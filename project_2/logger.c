/**
 * Author: Dchtrnd.be
 * Depot: Github (https://github.com/Randi-Dcht/ITR_TP_2022)
 * Encoding: UTF-8
 * Language: C 
 * Source(s):
 **/

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>

void logs(char* msg, char* file_name, int line, int isError)
{
    char cha[30]; char cha2[30];
    sprintf(cha, "logger_%s.log", file_name);
    sprintf(cha2, "log-%s-semaphore", file_name);

    //sem_t* sema = sem_open(cha2, O_CREAT, 0600, 1);

    //sem_wait(sema);
    FILE * file;
    file = fopen(cha, "a+");
    if (isError)
        fprintf(file, "! %s _ %i : %s\n", file_name, line, msg );
    else
        fprintf(file, "> %s _ %i : %s\n", file_name, line, msg );
    fclose(file);
    //sem_post(sema);

    //sem_unlink(cha2);
}