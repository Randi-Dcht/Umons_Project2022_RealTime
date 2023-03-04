/**
 * Author: Dchtrnd.be
 *         Louis Dascotte
 * Depot: Github (https://github.com/Randi-Dcht/ITR_TP_2022)
 * Encoding: UTF-8
 * Language: C 
 * Source(s):
 **/

#include "parser.h"
#include "objects.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/**
 * Clear the string of the \n
 */
void clear(char *string)
{
    int ii = strlen(string);
    int i = 0;

    while (i < ii)
    {
        if(string[i] == '\n')
            string[i] = ' ';
        i++;
    }
}


/**
 * Read the lines with only one number in the file (in our example shop_1, customer, maker and volume is read)
 */
int readNumber(char *file, char *name)
{
    char chaine[300];
    int space;
    int buff=0;
    FILE* fichier;

    fichier = fopen(file, "r");

    if (fichier != NULL)
    {
        while (fgets(chaine, 100, fichier) != NULL)
        {
            space = 0;
            char *split = strtok(chaine, " ");
            if (strcmp(split, name) == 0)
            {
                while (split != NULL)
                {
                    space++;
                    clear(split);
                    if (space == 2)
                        buff = strtol(split, 0, 10);
                    split = strtok(NULL, " ");
                }
            }
        }
        fclose(fichier);
    }
    else
        printf("error parser \n");
    return buff;
}


/**
 * Read all the lines with "quantity" in the file
 */
void readStock(char *file, struct stock *list)
{
    char chaine[300];
    int space;
    int i = 0;
    FILE* fichier;

    fichier = fopen(file, "r");

    if (fichier != NULL)
    {
        while (fgets(chaine, 100, fichier) != NULL)
        {
            space = 0;
            char *split = strtok(chaine, " ");
            if (strcmp(split, ".quantity") == 0)
            {
                while (split != NULL)
                {
                    space++;
                    clear(split);
                    if (space == 2)
                        list[i].id = strtol(split, 0, 10); //printf("Id of article : %ld --", )
                    if (space == 3)
                        list[i].max_in_stock = strtol(split, 0, 10);
                    split = strtok(NULL, " ");
                }
                i++;
            }
        }
        fclose(fichier);
    }
    else
        printf("error parser \n");
}


/**
 * Read all the lines with "article" in the file
 */
void readArticles(char *file, struct product *list)
{
    char chaine[300];
    int space;
    int i = 0;
    FILE* fichier;

    fichier = fopen(file, "r");

    if (fichier != NULL)
    {
        while (fgets(chaine, 100, fichier) != NULL)
        {
            space = 0;
            char *split = strtok(chaine, " ");
            if (strcmp(split, ".article") == 0)
            {
                while (split != NULL)
                {
                    space++;
                    clear(split);
                    if (space == 2)
                        list[i].id = strtol(split, 0, 10);
                    if (space == 3)
                        list[i].volume = strtol(split, 0, 10);
                    if (space == 4)
                        list[i].time_sec = strtol(split, 0, 10);
                    if (space == 5)
                        strcpy(list[i].info, split);//list[i].info = split;
                    split = strtok(NULL, " ");
                }
                i++;
            }
        }
        fclose(fichier);
    }
    else
        printf("error parser \n");
}


/**
 * Read all the lines with "client" in the file
 */
void readCustomer(char *file, struct customers *list)
{
    char chaine[300];
    int space;
    int i = 0;
    FILE* fichier;

    fichier = fopen(file, "r");

    if (fichier != NULL)
    {
        while (fgets(chaine, 100, fichier) != NULL)
        {
            space = 0;
            char *split = strtok(chaine, " ");
            if (strcmp(split, ".client") == 0)
            {
                while (split != NULL)
                {
                    space++;
                    clear(split);
                    if (space == 2)
                        list[i].min_sec = strtol(split, 0, 10);
                    if (space == 3)
                        list[i].max_sec = strtol(split, 0, 10);
                    split = strtok(NULL, " ");
                }
                i++;
            }
        }
        fclose(fichier);
    }
    else
        printf("error parser \n");
}