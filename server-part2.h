#define _GNU_SOURCE
#include "server_nonblock.h"
#include "cache.h"
#define SIGUNUSED   31

#define _NSIG       65  /* Biggest signal number + 1
                   (including real-time signals).  */
#define SIGRTMIN        (__libc_current_sigrtmin ())
#define SIGRTMAX        (__libc_current_sigrtmax ())

#define PORT 8080

int initial_server_fd;
pid_t my_pid;
union sigval *v;

/*function list*/

void *io_thread_func_2(); // To be completed

static void incoming_connection_handler_2(int sig, siginfo_t *si, void *data); // To be completed

static void outgoing_data_handler_2(int sig, siginfo_t *si, void *data); // To be completed

static int make_socket_non_blocking_2(int sfd);

int server_func_2();
int run_server_2(void);
void event_loop_scheduler_2();
