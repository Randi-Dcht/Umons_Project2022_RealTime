/**
 * Author: Dchtrnd.be
 *         Louis Dascotte
 *         Ingrid Fondja
 * Depot: Github (https://github.com/Randi-Dcht/ITR_TP_2022)
 * Encoding: UTF-8
 * Language: C 
 * Source(s):
 **/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <mqueue.h>
#include "objects.h"
#include "parser.h"
#include "logger.h"
#include <semaphore.h>


#define TIME_NIGHT 18 //18h00
#define TIME_DAY 8 //8h00


/**
 * the hour of the program
 * */
int now_hour = 12;
/**
 * The list of the stocks
 * */
struct stock* stockList;
/**
 * The storage
 */
struct storage* storage;
/***/
int* number_storage;
/***/
int loop = 1;
/***/
int pid_maker = -1;
/***/
int pid_customer = -1;
/**
 * Struct for the program starts
 * */
struct startProgramm* startingMemory;
/***/
int memoryPid = -1;
/***/
sem_t safeMemory;
/***/
struct waiter* list_waitCustomer;
/***/
struct waiter* list_waitMaker;
/***/
int number_stock;
/***/
int number_custo;


/**
 * Update the current time
 * */
void updateTime()
{
    now_hour = (now_hour + 1)%24;
    //printf("Hour:%i\n", now_hour);
    if(now_hour == TIME_NIGHT)
        kill(pid_maker, SIG_NIGHT);
    if (now_hour == TIME_DAY)
        kill(pid_maker, SIG_DAY);
    //logs("time update","stock.c", 81, 0);
}


/**
 * Stop the loop
 * */
void stop_loop()
{
    logs("stop program","stock.c", 90, 0);
    loop = 0;
}


/***/
void checkPlaceCustomer()//place this function in semaphore !
{
    for (int i = 0; i < number_custo; ++i)
    {
        if (list_waitCustomer[i].num_customer != -1)
        {
            sem_wait(&safeMemory);
            int memoryPid_ = -1; struct receive* memoryUser;
            memoryPid_ = shmget(list_waitCustomer[i].keyMemory, sizeof(struct receive), IPC_CREAT | 0666);
            if (memoryPid_ == -1){printf("ERROR 2 ! (memory segment)\n");exit(0);}
            memoryUser = shmat(memoryPid_, NULL, 0);
            if(memoryUser == (void*) -1){printf("ERROR 3 ! (memory segment)\n");exit(0);}

            printf("Remove [wait]: ");
            number_storage[list_waitCustomer[i].id_product] = number_storage[list_waitCustomer[i].id_product] - 1;
            memoryUser->serial_number = storage[list_waitCustomer[i].id_product*number_storage[list_waitCustomer[i].id_product]].serial_number;

            union sigval value;
            value.sival_int = list_waitCustomer[i].num_customer;
            sigqueue(pid_customer, SIG_ORDER, value);

            printf("SERIAL:%i\n", memoryUser->serial_number);
            shmdt(memoryUser);
            list_waitCustomer[i].num_customer = -1;
            sem_post(&safeMemory);logs("check if order is available now","stock.c", 120, 0);
        }
    }
}


/*
 * Alert when one of the makers is done making its product
 * **/
void alert_factory(int signum ,siginfo_t *data, void* context)
{
    int key_ = data->si_value.sival_int; int number_of_factory = -1; int serial = -1;

    int memoryPid_ = -1; struct finishMake* memoryFactory;
    memoryPid_ = shmget(key_, sizeof(struct finishMake), IPC_CREAT | 0666);
    if (memoryPid_ == -1){printf("ERROR 2 ! (memory segment)\n");exit(0);}
    memoryFactory = shmat(memoryPid_, NULL, 0);
    if(memoryFactory == (void*) -1){printf("ERROR 3 ! (memory segment)\n");exit(0);}

    number_of_factory = memoryFactory->id_product;
    serial = memoryFactory->serial_number;

    shmdt(memoryFactory);
    logs("receive from factory","stock.c", 142, 0);

    if (number_storage[number_of_factory] <= stockList[number_of_factory].max_in_stock)
    {
        sem_wait(&safeMemory);
          printf("Add: ");
          union sigval value;
          value.sival_int = number_of_factory;
          sigqueue(pid_maker, SIG_FACTORY, value);
          number_storage[number_of_factory] = number_storage[number_of_factory] + 1;
          storage[number_of_factory*number_storage[number_of_factory]].id_product = number_of_factory;
          storage[number_of_factory*number_storage[number_of_factory]].serial_number = serial;
          printf("SERIAL:%i-%i\n", number_of_factory, storage[number_of_factory*number_storage[number_of_factory]].serial_number);
          logs("add new article in stock","stock.c", 155, 0);
        sem_post(&safeMemory);
        checkPlaceCustomer();
    }
    else
    {
        printf("Wait\n");
        list_waitMaker[number_of_factory].serial_number = serial;
        list_waitMaker[number_of_factory].id_product = number_of_factory;
        logs("add the article in file wait","stock.c", 164, 0);
    }
}


/***/
void checkPlaceFactory()//place this function in semaphore !
{
    for (int i = 0; i < number_stock; ++i)
    {
        if (list_waitMaker[i].id_product != -1)
        {
            if (number_storage[list_waitMaker[i].id_product] <= stockList[list_waitMaker[i].id_product].max_in_stock)
            {
                sem_wait(&safeMemory);printf("Add [wait]: ");
                union sigval value;
                value.sival_int = list_waitMaker[i].id_product;
                sigqueue(pid_maker, SIG_FACTORY, value);
                number_storage[list_waitMaker[i].id_product] = number_storage[list_waitMaker[i].id_product] + 1;
                storage[list_waitMaker[i].id_product*number_storage[list_waitMaker[i].id_product]].id_product = list_waitMaker[i].id_product;
                storage[list_waitMaker[i].id_product*number_storage[list_waitMaker[i].id_product]].serial_number = list_waitMaker[i].serial_number;
                printf("SERIAL:%i\n", list_waitMaker[i].serial_number);
                list_waitMaker[i].id_product = -1;
                sem_post(&safeMemory);logs("check if place for an article in stock","stock.c", 188, 0);
            }
        }
    }
}


/**
 * Receive a request from a customer
 * */
void receive_order(int signum ,siginfo_t *data, void* context)
{
    int key_ = data->si_value.sival_int; int number_of_product = -1; int numUser = -1;

    int memoryPid_ = -1; struct receive* memoryUser;
    memoryPid_ = shmget(key_, sizeof(struct receive), IPC_CREAT | 0666);
    if (memoryPid_ == -1){printf("ERROR 2 ! (memory segment)\n");exit(0);}
    memoryUser = shmat(memoryPid_, NULL, 0);
    if(memoryUser == (void*) -1){printf("ERROR 3 ! (memory segment)\n");exit(0);}

    number_of_product = memoryUser->id_product;
    numUser = memoryUser->num_customer;

    logs("receive an order from customer","stock.c", 210, 0);

    if (number_storage[number_of_product] > 1)
    {
        sem_wait(&safeMemory);
        printf("Remove: ");

        number_storage[number_of_product] = number_storage[number_of_product] - 1;
        memoryUser->serial_number = storage[number_of_product*number_storage[number_of_product]].serial_number;

        union sigval value;
        value.sival_int = numUser;
        sigqueue(pid_customer, SIG_ORDER, value);

        printf("SERIAL:%i-%i\n", number_of_product, storage[number_of_product*number_storage[number_of_product]].serial_number);
        logs("remove article from stock and send article","stock.c", 226, 0);
        sem_post(&safeMemory);
        checkPlaceFactory();
    }
    else
    {
        printf("Wait\n");
        list_waitCustomer[numUser].id_product = number_of_product;
        list_waitCustomer[numUser].keyMemory = key_;
        list_waitCustomer[numUser].num_customer = numUser;
        logs("add order in file to wait","stock.c", 235, 0);
    }

    shmdt(memoryUser);
}


void openMemory(int initHere)
{
    int key = ftok("startMe", 4444);
    if (key == -1){printf("ERROR 1 ! (memory segment)\n");logs("error in key memory","stock.c", 245, 1);exit(0);}

    memoryPid = shmget(key, sizeof(struct startProgramm), IPC_CREAT | 0666);
    if (memoryPid == -1){printf("ERROR 2 ! (memory segment)\n");logs("error in shmget","stock.c", 248, 1);exit(0);}

    startingMemory = shmat(memoryPid, NULL, 0);
    if(startingMemory == (void*) -1){printf("ERROR 3 ! (memory segment)\n");logs("error in shmat","stock.c", 251, 1);exit(0);}


    if(initHere)// important (init here)
    {
        startingMemory->factory = -1;
        startingMemory->customer = -1;
        startingMemory->storage = -1;
    }

    startingMemory->storage = getpid();

    while (pid_customer == -1 || pid_maker == -1)
    {
        pid_customer = startingMemory->customer;
        pid_maker = startingMemory->factory;
        sleep(1);
    }
    logs("receive pid of customer and factory","stock.c", 269, 0);
}


/**
 * Main of stock
 */
int main(int argc, char *argv[])
{
    int statusInit = 0;
    if (argc < 2)
        exit(1);
    if (argc == 3)
        statusInit = 1;
    /*ALL SIGNAL*/
    struct sigaction desc;
    memset(&desc, 0, sizeof(struct sigaction));
    desc.sa_flags = SA_SIGINFO;
    desc.sa_sigaction = updateTime;
    sigaction(SIGALRM, &desc, NULL);
    desc.sa_flags = SA_SIGINFO;
    desc.sa_sigaction = alert_factory;
    sigaction(SIG_FACTORY, &desc, NULL);
    desc.sa_flags = SA_SIGINFO;
    desc.sa_sigaction = receive_order;
    sigaction(SIG_ORDER, &desc, NULL);
    signal(SIGINT, stop_loop);
    logs("init signal stock","stock.c",287, 0);
    /**/
    printf("STOCK : %i\n", getpid());
    openMemory(statusInit);
    sem_init(&safeMemory, 0, 0);
    /*READ FILE*/
    number_stock = readNumber(argv[1], ".maker");
    number_custo = readNumber(argv[1], ".customer");
    now_hour = readNumber(argv[1], ".hour");
    stockList = (struct stock*) malloc(number_stock * sizeof(*stockList));
    list_waitCustomer = (struct waiter*) malloc(number_custo * sizeof(*list_waitCustomer));
    list_waitMaker = (struct waiter*) malloc(number_stock * sizeof(*list_waitMaker));
    readStock(argv[1], stockList);
    logs("end to read file","stock.c",300, 0);

    /*START CUSTOMER*/
    int total_storage = 0;
    for (int i = 0; i < number_stock; ++i)
        total_storage += stockList[i].max_in_stock;

    /*CREATE STORAGE*/
    storage = (struct storage*) malloc(total_storage * sizeof(*storage));
    number_storage = (int*) malloc(number_stock* sizeof(int));
    logs("malloc","stock.c", 310, 0);

    for (int i = 0; i < number_stock; ++i)
    {
        number_storage[i] = 1;
        list_waitMaker[i].id_product = -1;
    }
    for (int i = 0; i < number_custo; ++i)
        list_waitCustomer[i].num_customer = -1;


    /*SET TIMER*/
    struct itimerval iti;
    iti.it_value.tv_sec = 0; iti.it_value.tv_usec = 88e4;
    iti.it_interval.tv_sec = 0; iti.it_interval.tv_usec = 88e4;
    setitimer(ITIMER_REAL, &iti, NULL);

    /*SEND OPEN STORE*/
    int custo, facto;
    union sigval value;
    facto = 0;//startCreate_Factory(number_stock);
    value.sival_int = facto;
    sigqueue(pid_maker, SIG_STORE_ACTIVE, value);
    custo = 0;//startCommand_Customer(number_customer);
    value.sival_int = custo;
    sigqueue(pid_customer, SIG_STORE_ACTIVE, value);

    while (loop){}

    if(shmctl(memoryPid, IPC_RMID, NULL) == -1)
        logs("error in close segment memory","stock.c", 349, 1);
    shmdt(startingMemory);//detached

    sem_destroy(&safeMemory);
    logs("destruct semaphore","stock.c", 353, 0);
    free(stockList);
    logs("No Segmentation fault (core dumped)","stock.c", 355, 0);
    free(storage);
    logs("No Segmentation fault (core dumped)","stock.c", 357, 0);
    free(number_storage);//TODO problem here with Segment fault
    logs("No Segmentation fault (core dumped)","stock.c", 359, 0);
    free(list_waitMaker);
    logs("No Segmentation fault (core dumped)","stock.c", 361, 0);
    free(list_waitCustomer);
    logs("No Segmentation fault (core dumped)","stock.c", 363, 0);
    logs("free all dynamic memory","stock.c", 359, 1);
    printf("End of Stock\n");

    return 0;
}