/**
 * Author: Dchtrnd.be
 * Depot: https://github.com/Randi-Dcht/ITR_TP_2022
 * Encoding: UTF-8
 * Language: C
 * Source(s): Data structure of Veronique Bruyere (Umons).
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




/**
 * stop the loop in main
 */
int isFinish = 0;
/**
 * counter of thread
 */
int thread_work = 0;
/**
 * Max size of array
 */
#define MAX_SIZE 65536
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
/*
 * Number of thread who is launch
 */
int numberThread = 0;
/**
 * list of active thread
 */
pthread_t arrayThread;
/*
 * Signal from create to sorting (to sort the list)
 */
#define SIGNALNEW (SIGRTMIN+3)
/*
 * Signal from sorting to create (list is sorted)
 */
#define SIGNALSORT (SIGRTMIN+2)
/**
 * Memory segment with my pid
 */
int memoryPid = -1;



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
 * Algorithm of sorting (slide p5)
 * On average is O(n * log_2(n))
 */
void swapi(int *array, int i, int j)
{
    int buff = array[i];
    array[i] = array[j];
    array[j] = buff;
}
int partition(int *array, int deb, int end)
{
    int j = deb;
    for (int i = deb; i < end; ++i)
    {
        if (array[i] <= array[end])
        {
            swapi(array, i, j);
            j++;
        }
    }
    swapi(array, j, end);
    return j;
}
void quickSort(int *array, int deb, int end)
{
    int j = 0;
    if (deb < end)
    {
        j = partition(array, deb, end);
        quickSort(array, deb, j-1);
        quickSort(array, j+1, end);
    }
}


/**
 * Thread sorts the array by quicksort
 */
void* launch_sorting(void *data)//TODO modified here to pass the segment with array
{
    /*Memory segment structure*/
    int mSeg = -1;
    /*Memory segment*/
    struct dataMemory* sg;

    int *key;//TODO here problem pointer
    key = data;


    /*open segment*/
    mSeg = shmget(key, sizeof(struct dataMemory), IPC_CREAT | 0666); //TODO is modified
    if (mSeg == -1)
    {
        printf("ERROR 2 ! (memory segment)\n");
        isFinish = 1;
        exit(0);
    }
    /*use segment*/
    sg = shmat(mSeg, NULL, 0);
    if(sg == (void*) -1)
    {
        printf("ERROR 3 ! (memory segment)\n");
        isFinish = 1;
        exit(0);
    }

    quickSort(sg->array, 1, sg->array[0]-1);//Note : the array [1; size-1] !!!
    kill(sg->create, SIGNALSORT);//TODO launch signal (finish)

    if(shmctl(mSeg, IPC_RMID, NULL) == -1)
        printf("ERROR ! (memory segment)\n");
    shmdt(mSeg);//detached

    thread_work--;
}


/**
 * Redefine handler to stop to receive data and create the new thread
 */
void handler_stoping()
{
    if(shmctl(memoryPid, IPC_RMID, NULL) == -1)
        printf("ERROR ! (memory segment)\n");
    shmdt(memoryPid);//detached
    printf("Close to receive data !\n");
    isFinish = 1;
}


/**
 * Signal of real time to start the sort
 */
void handler_goToSort(int signum ,siginfo_t *data, void* context)//receive signal
{
    thread_work++;
    pthread_create(&arrayThread, NULL, launch_sorting, data->si_value.sival_int);
}

/**
 * Init the segment memory with my pid
 */
void initMsg()//execute with : -lrt
{
    /*
    char id[8] = "02536";
    mqd_t queue = mq_open("/msg_pid",  O_EXCL|O_CREAT|O_WRONLY|O_TRUNC,0644,NULL);
    if (queue == -1) { printf("ERROR 1\n"); }
    ssize_t send_queue = mq_send(queue, id, strlen(id), 0);
    if (send_queue == -1) { printf("ERROR 2\n"); }
    mq_close(queue);
    */

    FILE *fp;
    int pid = getpid();
    char spid[8];   // ex. 34567
    sprintf(spid, "%d", pid);


    fp = fopen("/tmp/myid", "w+");
    fputs(spid, fp);
    fclose(fp);

    int key = ftok("mypid", 2424);
    if (key == -1)
    {
        printf("ERROR 1 ! (memory segment)\n");
        exit(0);
    }
    struct dataPid* sgm;
    memoryPid = shmget(key, sizeof(struct dataPid), IPC_CREAT | 0666);
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
    sgm->sorte = getpid();
}


/**
 * Main of process
 * The main is stop when all threads finish.
 */
int main()
{
    signal(SIGINT, handler_stoping);
    printf("START : sorting process with number : %i\n", getpid());

    struct sigaction descriptor;
    descriptor.sa_flags = SA_SIGINFO;
    descriptor.sa_sigaction = handler_goToSort;
    sigaction(SIGNALNEW, &descriptor, NULL);

    initMsg();

    while (thread_work != 0 || isFinish != 1){} //active wait

    //mq_unlink("/msg_pid");

    return 0;
}