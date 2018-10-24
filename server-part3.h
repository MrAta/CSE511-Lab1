#define _GNU_SOURCE

#include <stdio.h>
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
#include "cache.h"
#include "db.h"

#define SIGUNUSED   31

#define _NSIG       65  /* Biggest signal number + 1
                   (including real-time signals).  */
#define SIGRTMIN        (__libc_current_sigrtmin ())
#define SIGRTMAX        (__libc_current_sigrtmax ())

#define PORT 8080

#define THREAD_POOL_SIZE 1

#define INVALID -1
#define GET 0
#define PUT 1
#define INSERT 2
#define DELETE 3

/* When a SIGUSR1 signal arrives, set this variable. */ // ???
extern volatile sig_atomic_t usr_interrupt;

struct continuation {
  int request_type;
  char buffer[1024];
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
union sigval *v;

/*function list*/
int max(int a, int b); // Helper function

char *strdups(char *s); // Helper function

void *io_thread_func(); // To be completed

static void incoming_connection_handler(int sig, siginfo_t *si, void *data); // To be completed

static void outgoing_data_handler(int sig, siginfo_t *si, void *data); // To be completed

static int make_socket_non_blocking(int sfd);

int server_func();

int run_server_3(void);

void event_loop_scheduler();
