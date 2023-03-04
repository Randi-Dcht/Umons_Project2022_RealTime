/**
 * Author: Dchtrnd.be
 * Depot: https://github.com/Randi-Dcht/ITR_TP_2022
 * Encoding: UTF-8
 * Language: C 
 * Source(s):
 **/


#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/ipc.h>



/**
 * Max size of array
 */
#define MAX_SIZE 65536
/**
 * Define true to 1
 */
#define TRUE 1
/*
 * Define false to 0
 */
#define FALSE 0
/**
 * Struct with list of number to sorted and creator's pid
 */
struct dataMemory
{
    pid_t create;
    int array[MAX_SIZE];
};
/**
 * Struct with my pid in memory segment
 */
struct dataPid
{
    pid_t sorte;
};
/**
 * Finish the main loop
 */
int finish = 0;
/*
 * Memory segment structure
 */
int mSeg = -1;
/**
 * Memory segment
 */
struct dataMemory* sg;
/*
 * Signal from create to sorting (to sort the list)
 */
#define SIGNALNEW (SIGRTMIN+3)
/*
 * Signal from sorting to create (list is sorted)
 */
#define SIGNALSORT (SIGRTMIN+2)
/**
 * Stock the pid of sorting process
 */
pid_t pidSorting = 0;
/**
 * Key of my memory segment with all number
 */
key_t mykey;
/**
 * Time for the random
 */
time_t timeRand;




/*
 * Print the list
 */
void print_myList(int *input)
{
    printf("[Elements: %i { ", input[0]-1);
    for (int i = 1; i < input[0]; ++i)
        printf("%i; ", input[i]);
    printf("%i}]\n", input[input[0]-1]);
}


/**
 * Create an array with random number
 */
void creatorArray(int* buff)
{
    srand((unsigned) time(&timeRand));
    for (int i = 1; i < buff[0]; ++i)//start to 1; the first position is the size of list
        buff[i] = (-102 - rand() % 32) + time(NULL) % 111; //TODO upgrade
}


/**
 * Create an array in segment memory
 */
void create_array()
{
    srand((unsigned) time(&timeRand));
    int si =  5 + rand() % MAX_SIZE;
    sg->array[0] = si;
    creatorArray(sg->array);
    union sigval value;
    value.sival_int = mykey;
    sigqueue(pidSorting, SIGNALNEW, value);
}


/**
 * Redefine handler to stop to send data in sorting when receive INT
 */
void handler_stoping()//stop to send
{
    finish = 1;
    if (mSeg != -1)
    {
        if(shmctl(mSeg, IPC_RMID, NULL) == -1)//destroy segment
            printf("ERROR ! (memory segment)\n");
        shmdt(mSeg);//detached
    }
    printf("Close this process !\n");
}


/**
 * Thread to print the result and check the order of integer
 */
void* launch_verif(int* dt)
{
    int ok = TRUE; int minList = dt[1];//init
    for (int i = 1; i < dt[0]-1; ++i)//WARNING : the first element at position 1 !
    {
        /*return the minimum value*/
        if(dt[i] <= minList)
            minList = dt[i];
        if(dt[i+1] <= minList)
            minList = dt[i+1];
        /*check the order*/
        if(dt[i] > dt[i+1])
            ok = FALSE;
    }
    if(ok == 1)
        printf("GOOD (list is sorted) with %i elements and min is %i\n", dt[0]-1, minList);
    else
    {
        printf("ERROR ! (list isn't sorted) ==> ");print_myList(dt);
    }

    handler_stoping();
}


/**
 * Redefine the signal to check
 */
void handler_checkSort()
{
    launch_verif(sg->array);
}


/*
 * Initialise the memory segment
 */
void init_memory(char* nameMem)
{
    /*key of segment*/
    mykey = ftok(nameMem, 4488);
    if (mykey == -1)
    {
        printf("ERROR 1 ! (memory segment)\n");
        finish = 1;
        exit(0);
    }
    /*open segment*/
    mSeg = shmget(mykey, sizeof(struct dataMemory), IPC_CREAT | 0666);
    if (mSeg == -1)
    {
        printf("ERROR 2 ! (memory segment)\n");
        finish = 1;
        exit(0);
    }
    /*use segment*/
    sg = shmat(mSeg, NULL, 0);
    if(sg == (void*) -1)
    {
        printf("ERROR 3 ! (memory segment)\n");
        finish = 1;
        exit(0);
    }

    sg->create = getpid();
}


/**
 * Check the pid of sorting in memory segment
 */
void getSortingPid()
{
    /*
    char id[8];
    mqd_t queue = mq_open("/msg_pid", O_RDONLY);
    struct mq_attr mqa;
    mq_getattr(queue,&mqa);
    printf("La file contient: %ld\n",mqa.mq_curmsgs);
    if (queue == -1) { printf("ERROR 1\n"); }
    ssize_t receive = mq_receive(queue, id, mqa.mq_msgsize, 0);
    if (receive == -1) { printf("ERROR 2\n"); }
    mq_close(queue);
    printf("NUMBER : %s \n", id);
     */

    int memoryPid = -1;
    int key = ftok("mypid", 2424);
    if (key == -1)
    {
        printf("ERROR 1 ! (memory segment)\n");
        exit(0);
    }
    struct dataPid* sgm;
    memoryPid = shmget(key, sizeof(struct dataPid), IPC_CREAT | 0666); //TODO is modified
    if (memoryPid == -1)
    {
        printf("ERROR 2 ! (memory segment)\n");
        exit(0);
    }
    /*use segment*/
    sgm = shmat(memoryPid, NULL, 0);
    if(sgm == (void*) -1)
    {
        printf("ERROR 3 ! (memory segment)\n");
        exit(0);
    }
    pidSorting = sgm->sorte;
    shmdt(sgm);//detached
}


/**
 * The main loop of process
 */
int main(int argc, char *argv[])
{
    if (argc != 2)
        exit(1);

    getSortingPid();
    printf("START : creator array process with number : %i\n", getpid());

    struct sigaction descriptor;
    descriptor.sa_flags = SA_SIGINFO;
    descriptor.sa_sigaction = handler_checkSort;
    sigaction(SIGNALSORT, &descriptor, NULL);

    init_memory(argv[1]);//segment memory
    create_array();

    while (finish != 1){}//active wait

    return 0;
}