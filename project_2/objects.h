/**
 * Author: Dchtrnd.be
 *         Louis Dascotte
 * Depot: Github (https://github.com/Randi-Dcht/ITR_TP_2022)
 * Encoding: UTF-8
 * Language: C 
 * Source(s):
 **/

#ifndef ITR_TP_2022_PRODUCTS_H
#define ITR_TP_2022_PRODUCTS_H


#include <signal.h>



/**
 * Boolean variable
 */
#define TRUE 1
/**
 * Boolean variable
 */
#define FALSE 0
/**
 * The signal that signifies the start of the day
 * */
#define SIG_DAY (SIGRTMIN+2)
/**
 * The signal that signifies the start of the night
 * */
#define SIG_NIGHT (SIGRTMIN+1)
/**
 * The signal that sends an order from the customer to the stock
 * */
#define SIG_ORDER (SIGRTMIN+3)
/**
 * The signal that confirms that the maker is done making the product to the stock
 * */
#define SIG_FACTORY (SIGRTMIN+4)
/**
 * The signal that launches everyone at the start of the program
 * */
#define SIG_STORE_ACTIVE (SIGRTMIN+0)
/**
 * Struct of product with :
 *   - id of product type (integer)
 *   - information about product (char of 256)
 *   - volume of product (integer)
 *   - serial number (integer)
 */
struct product
{
    int id;
    char info[256];
    int volume;
    int time_sec;
};
/**
 * Struct of stock with :
 * - id of the product stocked
 * - max_in_stock the maximum stock available for the product
 * */
struct stock
{
    int id;
    int max_in_stock;
};
/**
 * Struct of a customer with :
 * min_sec the fastest time for the customer to ask for a product (after they received a product or at the start)
 * max_sec the slowest time for the customer to ask for a product (after they received a product or at the start)
 * */
struct customers
{
    int min_sec;
    int max_sec;
    int id;
};
/**
 * Struct of an order with :
 * id_product the id of the product that the customer wants
 * num_customer the id of the customer
 * pid_customer the processus number of the customer
 * */
struct finishMake
{
    int id_product;
    int serial_number;
};
/**
 * Struct of a file containing the orders
 * */
struct waiter
{
    int id_product;//factory and customer
    int serial_number;//factory and customer
    int num_customer;//only customer
    int keyMemory;//only customer
};
/**
 * Struct of a receive with :
 * id_product the id of the product that will be sent
 * serial_number the serial number of the product
 * num_customer the customer that will receive the product
 * */
struct receive
{
    int id_product;
    int serial_number;
    int num_customer;
};
/**
 * Struct of a storage with :
 * id_product the id of the product
 * serial_number the serial number of the product
 * */
struct storage
{
    int id_product;
    int serial_number;
};

/**
 * Struct containing the pids for the start of the program
 */
struct startProgramm
{
    pid_t storage;
    pid_t factory;
    pid_t customer;
};
#endif //ITR_TP_2022_PRODUCTS_H
