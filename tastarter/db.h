//
// Created by sps5394 on 10/18/18.
//

#ifndef P1_CSRF_DB_H
#define P1_CSRF_DB_H

#include <stdio.h>

// Database file
FILE *file;

char *filename = "names.txt";
/**
 * Retrieves a value from the database
 * @param key
 * @param ret_buf
 * @param ret_len
 * @return
 */
 inline void db_init() {
   file = fopen(filename, "r+");
 }
int db_get(char *key, char **ret_buf, int *ret_len);
int db_put(char *key, char *value, char **ret_buf, int *ret_len);

#endif //P1_CSRF_DB_H
