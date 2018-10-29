//
// Created by sps5394 on 10/18/18.
//

#ifndef P1_CSRF_DB_H
#define P1_CSRF_DB_H

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// need to check over these
#define MAX_KEY_SIZE 1024
#define MAX_VALUE_SIZE 11264
#define MAX_ENTRY_SIZE 11264

// Database file
FILE *file;

extern char *filename;

void db_init();
void db_connect();
void db_cleanup();
/**
 * Retrieves a value from the database
 * @param key The key of the object
 * @param ret_buf A buffer that will be allocated by the function and the data returned in
 * @param ret_len int * The length of the returned data not inclusive of the null terminator
 * @return 0 if success 1 if failure
 */
int db_get(char *key, char **ret_buf, int *ret_len);

/**
 * Replaces a value in the database. If the key does not exists, it will TODO: FILL HERE
 * @param key The key of the object
 * @param value The value to associate with the key
 * @param ret_buf A buffer that will be allocated by the function and the data returned in
 * @param ret_len int * The length of the returned data
 * @return 0 if success 1 if failure
 */
int db_put(char *key, char *value, char **ret_buf, int *ret_len);

/**
 * Adds a value to the database. If the key exists, it will do nothing.
 * @param key The key of the object
 * @param value The value to associate with the key
 * @param ret_buf A buffer that will be allocated by the function and the data returned in
 * @param ret_len int * The length of the returned data
 * @return 0 if success 1 if failure
 */
int db_insert(char *key, char *value, char **ret_buf, int *ret_len);

/**
 * Deletes a value from the database. If the key does not exist, it will do nothing.
 * @param key The key of the object
 * @param ret_buf A buffer that will be allocated by the function and the data returned in
 * @param ret_len int * The length of the returned data
 * @return 0 if success 1 if failure
 */
int db_delete(char *key, char **ret_buf, int *ret_len);

/**
 * Searches the database if the key exists. Also fills up the fbuf char **
 * with the data of the file.
 * @param key The key of the object
 * @param fbuf A buffer that will be allocated by the function and the data returned in
 * @param fbuf_bytes int * The length of the returned data
 * @return 0 if not found, 1 if found
 */
int db_search(const char *key, char **fbuf, int *fbuf_bytes);

#endif //P1_CSRF_DB_H
