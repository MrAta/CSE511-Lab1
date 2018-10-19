//
// Created by sps5394 on 10/18/18.
//

#ifndef P1_CSRF_SERVER_PART1_H
#define P1_CSRF_SERVER_PART1_H
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include "cache.h"
#include "db.h"

#define PORT 8080
#define QUEUED_CONNECTIONS 10

#define CACHE_SIZE 101

struct sockaddr_in address;
int opt = 1;

/**
 * The loop for the server. Continuously loops
 * and listens on the server socket
 * @return 0 if success 1 if failure
 */
int loop_and_listen_1();

/**
 * Main program for the server
 * @return 0 if success 1 if failure
 */
int run_server_1();

/**
 * Thread handler for the server upon accepting a new connection
 */
void *server_handler(void *arg);

/**
 * Handles the PUT request on the server
 * @param key Key as parsed from the request
 * @param value Value to put as part of the key
 * @param ret_buffer char **, allocates and returns if any data is to be returned
 * @param ret_size int *, the size of the buffer returned, 0 otherwise
 * @return 0 is success 1 if failure
 */
int server_1_put_request(char *key, char *value, char **ret_buffer, int *ret_size);

/**
 * Handles the GET request on the server
 * @param key Key as parsed from the request
 * @param ret_buffer char **, allocates and returns the data read from the DB
 * @param ret_size int *, the size of the read data
 * @return 0 if success 1 if failure
 */
int server_1_get_request(char *key, char **ret_buffer, int *ret_size);

#endif //P1_CSRF_SERVER_PART1_H
