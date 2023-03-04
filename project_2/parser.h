/**
 * Author: Dchtrnd.be
 *         Louis Dascotte
 * Depot: Github (https://github.com/Randi-Dcht/ITR_TP_2022)
 * Encoding: UTF-8
 * Language: C 
 * Source(s):
 **/

#ifndef PARSER_H
#define PARSER_H
#include "objects.h"

int readNumber(char *file, char *name);
void readStock(char *file, struct stock *list);
void readArticles(char *file, struct product *list);
void readCustomer(char *file, struct customers *list);

#endif //PARSER_H
