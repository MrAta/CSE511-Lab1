#include "server-part3.h"

pthread_mutex_t lock;

void io_thread_func_3() {
  char *req_str = NULL;
  char *req_key = NULL;
  char *req_val = NULL;
  char *tmp_line = NULL;
  char *save_ptr;
  char *tmp_line_copy = NULL;
  char *line_key = NULL;
  int found = 0, val_size = 0;

  int fsz = 0;
  char *fbuf = NULL;
  char *db_get_val = NULL;
  int fbuf_bytes = 0;

  while (1) {
    // we are either appending to queue below or executing a queue here; to maintain consistency can never do both so we
    // use a single lock to do writes/reads to files
    pthread_mutex_lock(&lock);
    db_connect();

    if (pending_head != NULL) {

      req_str = (char *) malloc(MAX_ENTRY_SIZE * sizeof(char));
      strcpy(req_str, pending_head->cont->buffer);

      // at this point request in buffer is valid string
      strtok_r(req_str, " ", &save_ptr);
      req_key = strtok_r(NULL, " ", &save_ptr);

      switch (pending_head->cont->request_type) {
        case GET:
          db_get(req_key, &db_get_val, &val_size);
          strncpy(pending_head->cont->result, db_get_val, val_size + 1);
          break;
        case PUT:
          req_val = strtok_r(NULL, "\n", &save_ptr);
          db_put(req_key, req_val, &db_get_val, &val_size);
          strncpy(pending_head->cont->result, db_get_val, val_size);
          break;
        case INSERT:
          req_val = strtok_r(NULL, "\n", &save_ptr);
          db_insert(req_key, req_val, &db_get_val, &val_size);
          strncpy(pending_head->cont->result, db_get_val, val_size);
          break;
        case DELETE:
          db_delete(req_key, &db_get_val, &val_size);
          strncpy(pending_head->cont->result, db_get_val, val_size);
          break;
      }
//TODO: write to pipe
      v = (union sigval *) malloc(sizeof(union sigval));
      v->sival_ptr = pending_head->cont;
      sigqueue(my_pid, SIGRTMIN + 4, *v); // send signal to main thread
      struct pending_queue *dead_head = pending_head;
      pending_head = pending_head->next; // free the pending_node in task queue, the cont is freed in outgoing
      free(dead_head);
      free(db_get_val);
      free(req_str);
    }
    db_cleanup();
    pthread_mutex_unlock(&lock);
  }
}

void setup_request_type(char *s, char **tec, struct continuation *tmp) {
  if (strcmp(s, "GET") == 0) {
    tmp->request_type = GET;
    memcpy(*tec, "NULL", 5);
  } else if (strcmp(s, "PUT") == 0) {
    tmp->request_type = PUT;
    memcpy(*tec, "NULL", 5);
  } else if (strcmp(s, "INSERT") == 0) {
    tmp->request_type = INSERT;
    memcpy(*tec, "-1", 3);
  } else if (strcmp(s, "DELETE") == 0) {
    tmp->request_type = DELETE;
    memcpy(*tec, "-1", 3);
  } else {
    tmp->request_type = INVALID;
    memcpy(*tec, "-1", 3);
  }
}

/*
  This func is only executed by the main thread, and appends a task to the back of the
  task queue
*/
void issue_io_req_3(struct continuation *tmp) {
  pthread_mutex_lock(&lock);
  pending_node = (struct pending_queue *) malloc(sizeof(struct pending_queue));
  pending_node->cont = tmp;
  pending_node->next = NULL;
  if (pending_head == NULL) {
    pending_head = pending_tail = pending_node;
  } else {
    pending_tail->next = pending_node;
    pending_tail = pending_node;
  }
  pthread_mutex_unlock(&lock);
}

static void incoming_connection_handler_3(int sig, siginfo_t *si, void *data) {

//if its main socket accept and read it
//else just read
// si->si_valu.sival_int == fd
  if(si->si_value.sival_int == initial_server_fd){
    printf("ATA: Main thread\n" );
    int valread, incoming;
    char *req_string;
    char *req_type = NULL;
    char *req_key = NULL;
    char *req_val = NULL;
    char *save_ptr;
    struct sockaddr_in in;
    socklen_t sz = sizeof(in);
    char *tmp_err_code = (char *) calloc(5, sizeof(char));
    struct continuation *tmp = NULL;
    struct node *tmp_node = NULL;

    incoming = accept(initial_server_fd, (struct sockaddr *) &in, &sz);
    // make_socket_non_blocking_3(incoming);

    tmp = (struct continuation *) malloc(sizeof(struct continuation));
    tmp->start_time = time(0);
    memset(tmp->buffer, 0, MAX_ENTRY_SIZE);
    valread = read(incoming, tmp->buffer, MAX_ENTRY_SIZE);
    //make incomming non setnonblockin
    //attach a signal
    int fl;
    fl = fcntl(incoming, F_GETFL);
    fl |= O_ASYNC | O_NONBLOCK; // want a signal on fd ready
    fcntl(incoming, F_SETFL, fl);
    fcntl(incoming, F_SETSIG, SIGRTMIN + 3);
    fcntl(incoming, F_SETOWN, getpid());

    req_string = (char *) malloc(MAX_ENTRY_SIZE * sizeof(char));
    strcpy(req_string, tmp->buffer);

    memset(tmp->result, 0, MAX_ENTRY_SIZE);
    tmp->fd = incoming;

    if (( req_type = strtok_r(req_string, " ", &save_ptr)) == NULL) { // will ensure strlen>0
      printf("%s\n", "bad client request: req_type");
      // TODO: update timings since we send directly here (and below)
      send(tmp->fd, "NULL", strlen("NULL"), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }

    setup_request_type(req_type, &tmp_err_code, tmp);

    if (( req_key = strtok_r(NULL, " ", &save_ptr)) == NULL) {
      printf("%s\n", "bad client request: req_key");
      // TODO: update timings
      send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }

    if (tmp->request_type == GET) {
      if (( tmp_node = cache_get(req_key)) != NULL) { // check cache
        printf("\nGET result found in cache\n\n");
        strcpy(tmp->result, tmp_node->defn);
        // TODO: update timings
        send(tmp->fd, tmp->result, strlen(tmp->result), 0);
      } else { // not in cache, check db
        issue_io_req_3(tmp);
      }
    } else if (tmp->request_type == PUT) {
      if (( req_val = strtok_r(NULL, " ", &save_ptr)) == NULL) {
        printf("%s\n", "bad client request: req_val");
        // TODO: update timings
        send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
        free(tmp_err_code);
        free(req_string);
        return;
      }
      issue_io_req_3(tmp);
    } else if (tmp->request_type == INSERT) {
      if (( req_val = strtok_r(NULL, " ", &save_ptr)) == NULL) {
        printf("%s\n", "bad client request: req_val");
        // TODO: update timings
        send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
        free(tmp_err_code);
        free(req_string);
        return;
      }
      if (( tmp_node = cache_get(req_key)) !=
          NULL) { // check if req_key in cache; yes - error: duplicate req_key violation, no - check db
        printf("%s\n", "error: duplicate req_key violation");
        // TODO: update timings
        send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
        free(tmp_err_code);
        free(req_string);
        return;
      }
      issue_io_req_3(tmp); // if not in cache, still might be in db
    } else if (tmp->request_type == DELETE) {
      issue_io_req_3(tmp); // issue io request to find out if req_key is in db to delete
    } else {
      // TODO: update timings
      send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }

    free(tmp_err_code);
    free(req_string);
  }
  else{
    printf("ATA: conns\n" );
    int valread; //, incoming;
    char *req_string;
    char *req_type = NULL;
    char *req_key = NULL;
    char *req_val = NULL;
    char *save_ptr;
    struct sockaddr_in in;
    socklen_t sz = sizeof(in);
    char *tmp_err_code = (char *) calloc(5, sizeof(char));
    struct continuation *tmp = NULL;
    struct node *tmp_node = NULL;

    // incoming = accept(initial_server_fd, (struct sockaddr *) &in, &sz);

    tmp = (struct continuation *) malloc(sizeof(struct continuation));
    tmp->start_time = time(0);
    memset(tmp->buffer, 0, MAX_ENTRY_SIZE);
    valread = read(si->si_value.sival_int, tmp->buffer, MAX_ENTRY_SIZE);
    //make incomming non setnonblockin
    //attach a signal
    // fl = fcntl(incomming, F_GETFL);
    // fl |= O_ASYNC | O_NONBLOCK; // want a signal on fd ready
    // fcntl(incomming, F_SETFL, fl);
    // fcntl(incomming, F_SETSIG, SIGRTMIN + 3);
    // fcntl(incomming, F_SETOWN, getpid());

    req_string = (char *) malloc(MAX_ENTRY_SIZE * sizeof(char));
    strcpy(req_string, tmp->buffer);

    memset(tmp->result, 0, MAX_ENTRY_SIZE);
    // tmp->fd = incoming;
    tmp->fd = si->si_value.sival_int;

    if (( req_type = strtok_r(req_string, " ", &save_ptr)) == NULL) { // will ensure strlen>0
      printf("%s\n", "bad client request: req_type");
      // TODO: update timings since we send directly here (and below)
      send(tmp->fd, "NULL", strlen("NULL"), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }

    setup_request_type(req_type, &tmp_err_code, tmp);

    if (( req_key = strtok_r(NULL, " ", &save_ptr)) == NULL) {
      printf("%s\n", "bad client request: req_key");
      // TODO: update timings
      send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }

    if (tmp->request_type == GET) {
      if (( tmp_node = cache_get(req_key)) != NULL) { // check cache
        printf("\nGET result found in cache\n\n");
        strcpy(tmp->result, tmp_node->defn);
        // TODO: update timings
        send(tmp->fd, tmp->result, strlen(tmp->result), 0);
      } else { // not in cache, check db
        issue_io_req_3(tmp);
      }
    } else if (tmp->request_type == PUT) {
      if (( req_val = strtok_r(NULL, " ", &save_ptr)) == NULL) {
        printf("%s\n", "bad client request: req_val");
        // TODO: update timings
        send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
        free(tmp_err_code);
        free(req_string);
        return;
      }
      issue_io_req_3(tmp);
    } else if (tmp->request_type == INSERT) {
      if (( req_val = strtok_r(NULL, " ", &save_ptr)) == NULL) {
        printf("%s\n", "bad client request: req_val");
        // TODO: update timings
        send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
        free(tmp_err_code);
        free(req_string);
        return;
      }
      if (( tmp_node = cache_get(req_key)) !=
          NULL) { // check if req_key in cache; yes - error: duplicate req_key violation, no - check db
        printf("%s\n", "error: duplicate req_key violation");
        // TODO: update timings
        send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
        free(tmp_err_code);
        free(req_string);
        return;
      }
      issue_io_req_3(tmp); // if not in cache, still might be in db
    } else if (tmp->request_type == DELETE) {
      issue_io_req_3(tmp); // issue io request to find out if req_key is in db to delete
    } else {
      // TODO: update timings
      send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }

    free(tmp_err_code);
    free(req_string);
  }

}

static void outgoing_data_handler_3(int sig, siginfo_t *si, void *data) {

  // update time measurements and cache in this func
  //TODO: read from the pipe
  struct continuation *req_cont = (struct continuation *) si->si_value.sival_ptr;
  char *req_string = (char *) calloc(MAX_KEY_SIZE, sizeof(char));
  char *req_type = NULL;
  char *req_key = NULL;
  char *save_ptr;
  char *val = NULL;

  strcpy(req_string, req_cont->buffer); // buffer includes null byte
  req_type = strtok_r(req_string, " ", &save_ptr);
  req_key = strtok_r(NULL, " ", &save_ptr);

  if (req_cont->request_type == GET) {
    // TODO: update time measurements
    if (strcmp(req_cont->result, "NOTFOUND") != 0) { // if result was NULL there was some kind of error
      cache_put(req_key, req_cont->result);
    }
  } else if (req_cont->request_type == PUT) {
    // TODO: update time measurements
    if (strcmp(req_cont->result, "NOTFOUND") != 0) {
      val = strtok_r(NULL, " ", &save_ptr);
      cache_put(req_key, val);
    }

  } else if (req_cont->request_type == INSERT) {
    // TODO: update time measurements
    if (strcmp(req_cont->result, "DUPLICATE") != 0) {
      val = strtok_r(NULL, " ", &save_ptr);
      cache_put(req_key, val);
    }
  } else { // DELETE
    // TODO: update time measurements
    if (strcmp(req_cont->result, "NOTFOUND") != 0) {
      cache_invalidate(req_key);
    }
  }

  send(req_cont->fd, req_cont->result, strlen(req_cont->result), 0);
  free(req_string);
  free(req_cont); // frees the cont that was just executed
}

static int make_socket_non_blocking_3(int sfd) {
  int flags, s;

  flags = fcntl(sfd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl");
    return -1;
  }
  flags |= O_NONBLOCK;
  s = fcntl(sfd, F_SETFL, flags);
  if (s == -1) {
    perror("fcntl");
    return -1;
  }

  return 0;
}

void *setup_sigs_and_exec_thread() {
  if (pthread_self() != main_tid) { // if not main thread, block SIGRTMIN signal on this thread
    sigset_t mask1;
    sigemptyset(&mask1);
    sigaddset(&mask1, SIGRTMIN+3);
    pthread_sigmask(SIG_BLOCK, &mask1, NULL);

    sigset_t mask2;
    sigemptyset(&mask2);
    sigaddset(&mask2, SIGRTMIN+4);
    pthread_sigmask(SIG_BLOCK, &mask2, NULL);

    io_thread_func_3();
  } else { // if main thread
    struct sigaction act, react;
    act.sa_sigaction = incoming_connection_handler_3;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGRTMIN + 3, &act, NULL);

    react.sa_sigaction = outgoing_data_handler_3;
    sigemptyset(&react.sa_mask);
    react.sa_flags = SA_SIGINFO;
    sigaction(SIGRTMIN + 4, &react, NULL);

    event_loop_scheduler_3();
  }
}

int server_func_3() {
  int server_fd, opt;

  // Creating socket file descriptor
  if (( server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == 0) {
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
  address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *) &address,
           sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  return server_fd;
}

void event_loop_scheduler_3() {
  initial_server_fd = server_func_3();
  int fl;

  fl = fcntl(initial_server_fd, F_GETFL);
  fl |= O_ASYNC | O_NONBLOCK; // want a signal on fd ready
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

int run_server_3(void) {
  // struct sigaction act, react;
  my_pid = getpid();
  main_tid = pthread_self();

  pending_head = pending_tail = NULL;
  head = tail = curr = temp_node = NULL;

  // Creating I/O thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&io_thread[i], NULL, setup_sigs_and_exec_thread, NULL);
  }

  // event_loop_scheduler_3();
  setup_sigs_and_exec_thread(); // for main thread

  return 0;
}
