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
#include "parser.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <mqueue.h>
#include "objects.h"
#include "logger.h"


/**
 * The maximum of threads (aka the maximum of customers)
 */
int max_thread;
/**
 * The number of articles available
 */
int number_articles;
/***/
struct storage* listOfReceive;//TODO update the dynamic size with the malloc and free
/**
 * The list of product
 */
struct product* productList;
/**
 * The list of customer
 */
struct customers* customersList;
/**
 * The list of semaphore
 */
pthread_mutex_t* list_semaphore;
/***/
int isFinish = 1;
/***/
mqd_t queue = -1;
/**
 * The number of threads currently working
 * */
int threadWork = 0;
/***/
int pid_stock;
/***/
int wait_store = 1;


/**
 * Open a data segment for the customers that will contain the pids of startProgramm
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
        logs("Init the memory to start","customer.c",82, 0);
    }

    startingMemory->customer = getpid();
    pid_stock = -1;

    while (pid_stock == -1)
    {
        pid_stock = startingMemory->storage;
        sleep(1);
    }

    logs("Read the pid of stock","customer.c",94, 0);
    shmdt(startingMemory);
}


/**
 * Generate a random number between mn et mx
 */
int randomNumber(int mn, int mx)
{
    return mn + (rand() % mx);
}


/**
 * Initiate the queue of orders from the customers
 */
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
 * Send a message to the stock to ask for a product
 */
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
 * Choose a random product
 */
int choose_article()
{
    int choose = randomNumber(0, number_articles);
    return productList[choose].id;
}


/**
 * Thread
 */
void* launch_sorting(void *data)
{
    logs("Start thread user","customer.c",149, 0);

    struct storage* myListBuy;int myListMax = 50; int myListSize = 0;
    myListBuy = (struct storage*) malloc(myListMax * sizeof(struct storage));

    struct customers* me;me = data;
    int myNum; myNum=me->id; char nm[10];
    snprintf(nm, 10, "mCusto%i", myNum);
    FILE* f = fopen(nm, "w+");fclose(f);

    struct receive* memoryCustomer; int memoryPid_ = -1;int chooseArticle =0;
    int key_ = ftok(nm, 4774); //TODO
    if (key_ == -1){printf("ERROR 1 ! (memory segment)\n");exit(0);}
    memoryPid_ = shmget(key_, sizeof(struct receive), IPC_CREAT | 0666);
    if (memoryPid_ == -1){printf("170:ERROR 2 ! (memory segment)\n");exit(0);}
    memoryCustomer = shmat(memoryPid_, NULL, 0);
    if(memoryCustomer == (void*) -1){printf("ERROR 3 ! (memory segment)\n");exit(0);}

    memoryCustomer->num_customer = myNum;

    union sigval value;
    usleep(me->min_sec);// delay to start
    logs("Start loop in thread","customer.c",172, 0);
    while (isFinish)//TODO update the random time !
    {
        chooseArticle = choose_article();
        value.sival_int = key_;
        memoryCustomer->id_product= chooseArticle;
        printf(">>>(%i)choose: %i\n", myNum, chooseArticle);
        sigqueue(pid_stock, SIG_ORDER, value);
        logs("Send an article to buy","customer.c",180, 0);
        pthread_mutex_lock(&list_semaphore[myNum]);
        printf("Receive an article : %i-%i!\n", memoryCustomer->id_product, memoryCustomer->serial_number);
        logs("Receive an article","customer.c",183, 0);

        if (myListSize+1 >= myListMax)
        {
            free(myListBuy);
            myListMax *= 2;
            myListBuy = (struct storage*) malloc(myListMax * sizeof(struct storage));
            logs("Update the mew size array in thread","customer.c",190, 0);
        }
        myListBuy[myListSize].serial_number = memoryCustomer->serial_number;
        myListBuy[myListSize].id_product    = memoryCustomer->id_product;
        myListSize++;

        usleep(me->max_sec);
    }

    if(shmctl(memoryPid_, IPC_RMID, NULL) == -1)
        printf("ERROR ! (memory segment)\n");
    shmdt(memoryCustomer);//detached

    for (int i = 0; i < myListSize; ++i)
        printf("%i-%i ; ", myListBuy[i].id_product, myListBuy[i].serial_number);
    printf("\n");

    free(myListBuy);
    logs("Quit the thread and free dynamic memory","customer.c",208, 0);

    threadWork--;
}


/***/
void stop_loop()
{
    isFinish = 0;
    for (int i = 0; i < max_thread; ++i)
    {
        pthread_mutex_unlock(&list_semaphore[i]);
        pthread_mutex_destroy(&list_semaphore[i]);
    }
    logs("Stop loop and free mutex in thread","customer.c",233, 0);
    printf("Stop all thread in customer\n");
}


/**
 * Wait until the stock has a place for the product that the maker did
 * */
void confirm_receive(int signum ,siginfo_t *data, void* context)
{
    int number_of_custo = data->si_value.sival_int;
    pthread_mutex_unlock(&list_semaphore[number_of_custo]);
    //printf("UNLOCK %i \n", number_of_custo);
}


/**
 * Says that the store is active
 * */
void store_active(int signum ,siginfo_t *data, void* context)
{
    int key_of_memory = data->si_value.sival_int;
    wait_store = 0;
    logs("Receive the start signal","customer.c",246, 0);
}


/**
 * Main of customer
 */
int main(int argc, char *argv[])
{
    int statusInit = 0;
    if (argc < 2)
        exit(1);
    if (argc == 3)
        statusInit = 1;
    /*ALL SIGNAL*/
    struct sigaction descriptor;
    memset(&descriptor, 0, sizeof(struct sigaction));
    descriptor.sa_flags = SA_SIGINFO;
    descriptor.sa_sigaction = store_active;
    sigaction(SIG_STORE_ACTIVE, &descriptor, NULL);
    descriptor.sa_flags = SA_SIGINFO;
    descriptor.sa_sigaction = confirm_receive;
    sigaction(SIG_ORDER, &descriptor, NULL);
    signal(SIGINT, stop_loop);
    /**/
    printf("CUSTOMER : %i\n", getpid());
    openMemory(statusInit);
    /*INIT NUMBER*/
    max_thread = readNumber(argv[1], ".customer");
    number_articles = readNumber(argv[1], ".maker");
    printf("Process (%i) with %i customer(s) and %i articles\n", getpid(), max_thread, number_articles);
    /*PRODUCT READ*/
    productList = (struct product*) malloc(number_articles * sizeof(*productList));
    readArticles(argv[1], productList);
    /*CUSTOMER READ*/
    customersList = (struct customers*) malloc(max_thread * sizeof(*customersList));
    readCustomer(argv[1], customersList);
    list_semaphore = (pthread_mutex_t*) malloc(max_thread * sizeof(*list_semaphore));

    while (wait_store){}

    for (int i = 0; i < number_articles; ++i)
        printf("ID %i : %s\n", productList[i].id ,productList[i].info);
    for (int i = 0; i < max_thread; ++i)
        printf("Min : %i et Max : %i\n", customersList[i].min_sec, customersList[i].max_sec);

    logs("Init and start thread","customer.c",292, 0);
    pthread_t arrayThread;
    for (int i = 0; i < max_thread; ++i)
    {
        pthread_mutex_init(&list_semaphore[i], NULL);
        pthread_mutex_lock(&list_semaphore[i]);
        customersList[i].id = i;
        threadWork++;
        pthread_create(&arrayThread, NULL, launch_sorting, &customersList[i]);
    }



    while (threadWork != 0){}

    /*CLEAN*/
    free(customersList);
    free(productList);
    if (queue != -1)
        mq_close(queue);

    printf("End of customers\n");
    logs("Close the process","customer.c",313, 0);

    return 0;
}