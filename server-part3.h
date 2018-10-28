#define _GNU_SOURCE

#include <stdio.h>
#include "server_nonblock.h"
#include "cache.h"
#include "db.h"

#define SIGUNUSED   31

#define _NSIG       65  /* Biggest signal number + 1
                   (including real-time signals).  */
#define SIGRTMIN        (__libc_current_sigrtmin ())
#define SIGRTMAX        (__libc_current_sigrtmax ())

#define PORT 8085

#define NEW_CONN "0"
#define EXISTING_CONN "1"
#define OUTGOING_DATA "2"


sigset_t block_rtsigs_set;
int incoming_events;
pthread_mutex_t lock;
int sig_handler_pipe[2];
int bytes_written_to_pipe;
int bytes_read_from_pipe;
// volatile sig_atomic_t usr_interrupt = 0;


/*function list*/

void io_thread_func_3(); // To be completed

void *setup_sigs_and_exec_thread();

static void incoming_connection_handler_3(int sig, siginfo_t *si, void *data); // To be completed

void incoming_connection_handler_3_helper_new();

void incoming_connection_handler_3_helper(int);

static void outgoing_data_handler_3(int sig, siginfo_t *si, void *data); // To be completed

static int make_socket_non_blocking_3(int sfd);

int server_func_3();

int run_server_3(void);

void event_loop_scheduler_3();
