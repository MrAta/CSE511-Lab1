//
// Created by sps5394 on 10/18/18.
//

#ifndef P1_CSRF_DB_H
#define P1_CSRF_DB_H

#include <stdio.h>

// Database file
FILE *file;

char *filename = "names.txt";
 inline void db_init() {
   file = fopen(filename, "r+");
 }
/**
 * Retrieves a value from the database
 * @param key The key of the object
 * @param ret_buf A buffer that will be allocated by the function and the data returned in
 * @param ret_len int * The length of the returned data
 * @return 0 if success 1 if failure
 */
int db_get(char *key, char **ret_buf, int *ret_len);

/**
 * Adds a value to the database. If the key exists, it will replace it
 * @param key The key of the object
 * @param value The value to associate with the key
 * @param ret_buf A buffer that will be allocated by the function and the data returned in
 * @param ret_len int * The length of the returned data
 * @return 0 if success 1 if failure
 */
int db_put(char *key, char *value, char **ret_buf, int *ret_len);

#endif //P1_CSRF_DB_H
