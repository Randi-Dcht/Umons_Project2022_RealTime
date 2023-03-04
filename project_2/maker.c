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
#include <mqueue.h>
#include <semaphore.h>
#include "objects.h"
#include "parser.h"
#include "logger.h"

/**
 * "boolean" that says if it's the day or not
 * */
unsigned int isDay = FALSE;
/**
 * The list of product
 */
struct product* productList;
/**
 * The list of semaphore
 */
pthread_mutex_t* list_semaphore;
/***/
pthread_t* arrayThread;
/***/
int loop = 1;
/***/
int number_thread = 0;
/***/
mqd_t queue = -1;
/***/
pid_t pid_stock;
/***/
int wait_store = 1;
/***/
int number_maker;


/**
 * Open a data segment for the makers that will contain the pids of startProgramm
 * */
void openMemory(int initHere)
{
    struct startProgramm* startingMemory;
    int memoryPid = -1;

    int key = ftok("startMe", 4444);
    if (key == -1){printf("ERROR 1 ! (memory segment)\n");exit(0);}

    memoryPid = shmget(key, sizeof(struct startProgramm), IPC_CREAT | 0666);
    if (memoryPid == -1){printf("ERROR 2 ! (memory segment)\n");exit(0);}

    startingMemory = shmat(memoryPid, NULL, 0);
    if(startingMemory == (void*) -1){printf("ERROR 3 ! (memory segment)\n");exit(0);}

    if(initHere)
    {
        startingMemory->factory = -1;// important (init here)
        startingMemory->customer = -1;
        startingMemory->storage = -1;
        logs("I'm first to init value","maker.c", 76, 0);
    }

    startingMemory->factory = getpid();logs("stock my pid for stock","maker.c", 79, 0);
    pid_stock = -1;

    while (pid_stock == -1)
    {
        pid_stock = startingMemory->storage;
        sleep(1);
    }

    logs("stock pid of stock","maker.c", 88, 0);
    shmdt(startingMemory);
}


/**
 * Initiate the queue of orders from the maker
 * */
void initQueue()//TODO is delete
{
    queue = mq_open("/queueStock",  O_EXCL|O_CREAT|O_WRONLY|O_TRUNC,0644,NULL);
    if (queue == -1)
    {
        printf("ERROR Queue 1\n");
        exit(1);
    }
}


/**
 * Send a message to the stock
 * */
void sendMessage(char *msg)//TODO is delete
{
    ssize_t send_queue = mq_send(queue, msg, strlen(msg), 0);
    if (send_queue == -1)
    {
        printf("ERROR Queue 2\n");
        exit(1);
    }
}


/**
 * Thread
 */
void* launch_sorting(void *data)//TODO add the night and day
{
    number_thread++;
    struct product *my_product;my_product = data;char nm[10];int serial_counter= 0;
    snprintf(nm, 10, "mFacto%i", my_product->id);
    FILE* f = fopen(nm, "w+");fclose(f);
    printf("ID : %i ( %s )\n", my_product->id, my_product->info);

    logs("create segment memory","maker.c", 132, 0);

    int memoryPid_ = -1; struct finishMake* memoryFactory;
    int key_ = ftok(nm, 4884);
    if (key_ == -1){printf("ERROR 1 ! (memory segment)\n");exit(0);}
    memoryPid_ = shmget(key_, sizeof(struct finishMake), IPC_CREAT | 0666);
    if (memoryPid_ == -1){printf("151:ERROR 2 ! (memory segment)\n");exit(0);}
    memoryFactory = shmat(memoryPid_, NULL, 0);
    if(memoryFactory == (void*) -1){printf("ERROR 3 ! (memory segment)\n");exit(0);}

    memoryFactory->id_product = my_product->id;

    union sigval value;
    value.sival_int = key_;

    logs("start loop of product","maker.c", 148, 0);
    while (loop)
    {
        usleep(my_product->time_sec);
        while (isDay){} //TODO upgrade with semaphore or counter
        memoryFactory->serial_number = serial_counter;
        serial_counter++;
        sigqueue(pid_stock, SIG_FACTORY, value);
        pthread_mutex_lock(&list_semaphore[my_product->id]);
    }

    if(shmctl(memoryPid_, IPC_RMID, NULL) == -1)
        printf("ERROR ! (memory segment)\n");
    shmdt(memoryFactory);//detached

    logs("finish thread in maker","maker.c", 161, 0);
    number_thread--;
}


/**
 * Turn into day when receiving SIG_DAY
 * */
void header_day()
{
    isDay = FALSE;
    printf("Changed Timer! (DAY)\n");
    logs("signal to day","maker.c", 173, 0);
}


/**
 * Turn into night when receiving SIG_NIGHT
 * */
void header_night()
{
    isDay = TRUE;
    printf("Changed Timer! (NIGHT)\n");
    logs("signal to night","maker.c", 184, 0);
}


/***/
void stop_loop()
{
    loop = 0;
    for (int i = 0; i < number_maker; ++i)
    {
        pthread_mutex_unlock(&list_semaphore[i]);
        pthread_mutex_destroy(&list_semaphore[i]);
    }
    logs("stop all thread and destruct the semaphore","maker.c", 197, 0);
}


/**
 * Wait until the stock has a place for the product that the maker did
 * */
void confirm_maker(int signum ,siginfo_t *data, void* context)
{
    int number_of_factory = data->si_value.sival_int;
    pthread_mutex_unlock(&list_semaphore[number_of_factory]);
    logs("receive signal to confirm","maker.c", 208, 0);
    //printf("UNLOCK %i \n", number_of_factory);
}


/**
 * Initialize the makers
 * */
void store_active(int signum ,siginfo_t *data, void* context)
{
    int key_of_memory = data->si_value.sival_int;
    wait_store = 0;
    logs("receive the signal to open store","maker.c", 220, 0);
}


/**
 * Main of maker
 */
int main(int argc, char *argv[])
{
    int statusInit = 0;
    if (argc < 2)
        exit(1);
    if (argc == 3)
        statusInit = 1;
    /*SIGNAL HEADER*/
    struct sigaction descriptor;
    memset(&descriptor, 0, sizeof(struct sigaction));
    descriptor.sa_flags = SA_SIGINFO;
    descriptor.sa_sigaction = header_day;
    sigaction(SIG_DAY, &descriptor, NULL);
    descriptor.sa_flags = SA_SIGINFO;
    descriptor.sa_sigaction = header_night;
    sigaction(SIG_NIGHT, &descriptor, NULL);
    descriptor.sa_flags = SA_SIGINFO;
    descriptor.sa_sigaction = confirm_maker;
    sigaction(SIG_FACTORY, &descriptor, NULL);
    descriptor.sa_flags = SA_SIGINFO;
    descriptor.sa_sigaction = store_active;
    sigaction(SIG_STORE_ACTIVE, &descriptor, NULL);
    signal(SIGINT, stop_loop);
    /**/
    printf("MAKER : %i\n", getpid());
    openMemory(statusInit);
    /*INIT*/
    number_maker = readNumber(argv[1], ".maker");

    /*PRODUCT READ*/
    productList = (struct product*) malloc(number_maker * sizeof(*productList));
    list_semaphore = (pthread_mutex_t*) malloc(number_maker * sizeof(*list_semaphore));
    arrayThread = (pthread_t *) malloc(number_maker * sizeof(*arrayThread));
    readArticles(argv[1], productList);

    while (wait_store){}

    for (int i = 0; i < number_maker; ++i)
    {
        pthread_mutex_init(&list_semaphore[i], NULL);
        pthread_mutex_lock(&list_semaphore[i]);
        pthread_create(&arrayThread[i], NULL, launch_sorting, &productList[i]);
    }


    while (number_thread != 0){}

    free(productList);
    free(list_semaphore);
    free(arrayThread);
    printf("End of Makers\n");
    logs("free all dynamic memory","maker.c", 278, 0);

    return 0;
}