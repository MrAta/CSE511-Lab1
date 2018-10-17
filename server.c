#include "server.h"

int max (int a, int b) {
  return (a>b?a:b);
}

char *strdups(char *s) {/* make a duplicate of s */
    char *p;
    p = (char *) malloc(strlen(s)+1); /* +1 for ’\0’ */
    if (p != NULL)
       strcpy(p, s);
    return p;
}

struct node* get (char *s) {
  curr = head;
  while (curr != NULL) {
    if (strcmp(curr->name, s) == 0) {
      //found in cache, move node to head of list
      // and return value of key

      if(curr != head) {
        temp_node = curr->prev;
        if (curr == tail) {
          temp_node->next = NULL;
          tail = temp_node;
        } else {
          temp_node->next = curr->next;
          curr->next->prev = temp_node;
        }

        head->prev = curr;
        curr->next = head;
        curr->prev = NULL;
        head = curr;
      }
      return head;
    }
    curr = curr->next;
  }
  // Key not found in cache
  return NULL;
}

void put (char *name, char *defn) {
  struct node *cache_entry;
  if ((cache_entry = get(name)) == NULL) {
    // value not in cache
    temp_node = (struct node*) malloc (sizeof(struct node));
    temp_node->name = strdups(name);
    temp_node->defn = strdups(defn);
    temp_node->next = temp_node->prev = NULL;
    if(head == NULL) {
      head = tail = temp_node;
    } else {
      head->prev = temp_node;
      temp_node->next = head;
      temp_node->prev = NULL;
      head = temp_node;
    }
    if (global_cache_count >= CACHE_SIZE) {
      // cache is full, evict the last node
      // insert a new node in the list
      tail = tail->prev;
      tail->next = NULL;

      //TODO: Free the memory as you evict nodes
    } else {
      // Increase the count
      global_cache_count++;
    }
  } else {
    // update cache entry
    cache_entry->defn = strdups(defn);
  }
}

void *io_thread_func() {
  // This function should handle incoming I/O requests,
  // once the request is processed, the thread notifies
  // the event schuduler using a signal

  // Complete this function to implement I/O functionality
  // for incoming requests and handle proper synchronization
  // among other helper threads

  while(1) {
    if (pending_head != NULL) {
      if (pending_head->cont->request_type == 0) {
       strcpy(pending_head->cont->result, "SAMPLE RESPONSE");
      } else {
       strcpy(pending_head->cont->result, "1");
      }
      v = (union sigval*) malloc (sizeof(union sigval));
        v->sival_ptr = pending_head->cont;
        sigqueue(my_pid, SIGRTMIN+4, *v);
        pending_head = pending_head->next;
    }
  }
}

static void incoming_connection_handler(int sig, siginfo_t *si, void *data) {
    int fl, valread;
    struct sockaddr_in in;
    socklen_t sz = sizeof(in);
    char *request_string, *request_key, *request_value, *tokens;
    char *temp_string;
    incoming = (int *) malloc (sizeof(int));
    *incoming = accept(initial_server_fd,(struct sockaddr*)&in, &sz);

    temp = (struct continuation*) malloc (sizeof (struct continuation));
    temp->start_time = time(0);

    temp_string = (char *) malloc (1024 * sizeof(char));
    memset(temp->buffer, 0, 1024);
    valread = read( *incoming , temp->buffer, 1024);

    strcpy (temp_string, temp->buffer);
    tokens = strtok(temp_string, " ");

    if (strcmp(tokens, "GET") == 0) {
      temp->request_type = 0;
    } else {
      temp->request_type = 1;
    }

    memset(temp->result, 0, 1024);
    temp->fd = *incoming;

    // Servicing the request
    if (temp->request_type == 0) {

      // This is a GET request, check the cache first
      tokens = strtok(NULL, " ");
      temp_node = get(tokens);
      if (temp_node != NULL) {

        // Result found in cache
        printf("\nResult found in cache\n\n");
        strcpy(temp->result, temp_node->defn);
        send(temp->fd ,temp->result , strlen(temp->result) , 0);
      } else {

        // Not found in cache, issue request to I/O
        pending_node = (struct pending_queue*) malloc (sizeof(struct pending_queue));
        pending_node->cont = temp;
        pending_node->next = NULL;
        if (pending_head == NULL) {
          pending_head = pending_tail = pending_node;
        } else {
          pending_tail->next = pending_node;
          pending_tail = pending_node;
        }
      }
    } else if (temp->request_type == 1) {

      // This is a PUT request, complete the function
      // to service the request.

      pending_node = (struct pending_queue*) malloc (sizeof(struct pending_queue));
      pending_node->cont = temp;
      pending_node->next = NULL;
      if (pending_head == NULL) {
        pending_head = pending_tail = pending_node;
      } else {
        pending_tail->next = pending_node;
        pending_tail = pending_node;
      }
    }
}

static void outgoing_data_handler(int sig, siginfo_t *si, void *data) {
  // Function to be completed.

  struct continuation *temp_cont_to_send = (struct continuation*)si->si_value.sival_ptr;
  send(temp_cont_to_send->fd, temp_cont_to_send->result, strlen(temp_cont_to_send->result), 0);
  free(temp_cont_to_send);
}

static int make_socket_non_blocking (int sfd) {
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1) {
      perror ("fcntl");
      return -1;
  }
  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1) {
      perror ("fcntl");
      return -1;
  }

  return 0;
}

int server_func() {
  int server_fd;
  addrlen = sizeof(address);

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                &opt, sizeof(opt))) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( PORT );

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address,
                               sizeof(address))<0) {
      perror("bind failed");
      exit(EXIT_FAILURE);
  }

  return server_fd;
}

void event_loop_scheduler() {
     initial_server_fd = server_func();
     int fl;

     fl = fcntl(initial_server_fd, F_GETFL);
     fl |= O_ASYNC|O_NONBLOCK;     /* want a signal on fd ready */
     fcntl(initial_server_fd, F_SETFL, fl);
     fcntl(initial_server_fd, F_SETSIG, SIGRTMIN + 3);
     fcntl(initial_server_fd, F_SETOWN, getpid());

     if (listen(initial_server_fd, 5000) < 0) {
         perror("listen");
         exit(EXIT_FAILURE);
     }

     while (1) {
        printf("Waiting for a signal\n");
        pause();
        printf("Got some event on fd\n");
     }
}

int main (void)
{
  struct sigaction act, react;
  my_pid = getpid();

  pending_head = pending_tail = NULL;
  head = tail = curr = temp_node = NULL;

  file = fopen("names.txt", "r+");

  act.sa_sigaction = incoming_connection_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGRTMIN + 3, &act, NULL);

  react.sa_sigaction = outgoing_data_handler;
  sigemptyset(&react.sa_mask);
  react.sa_flags = SA_SIGINFO;
  sigaction(SIGRTMIN + 4, &react, NULL);

  //Creating I/O thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&io_thread[i], NULL, io_thread_func, NULL);
  }

  event_loop_scheduler();

  return 0;
}
