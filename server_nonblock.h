//
// Created by sps5394 on 10/24/18.
//

#ifndef P1_CSRF_SERVER_NONBLOCK_H
#define P1_CSRF_SERVER_NONBLOCK_H
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include "db.h"

#define THREAD_POOL_SIZE 1

#define INVALID -1
#define GET 0
#define PUT 1
#define INSERT 2
#define DELETE 3

/* When a SIGUSR1 signal arrives, set this variable. */ // ???

struct continuation {
  int request_type;
  char buffer[MAX_ENTRY_SIZE];
  int fd;
  char result[MAX_VALUE_SIZE];
  time_t start_time, finish_time;
} *temp;

struct pending_queue {
  struct continuation *cont;
  struct pending_queue *next;
} *pending_head, *pending_tail, *pending_node;

struct sockaddr_in address;
pthread_t io_thread[THREAD_POOL_SIZE];
int initial_server_fd;
pid_t my_pid;
pthread_t main_tid;
union sigval *v;
int pipe_fd[2];
#endif //P1_CSRF_SERVER_NONBLOCK_H
