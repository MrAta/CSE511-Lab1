#include "server-part3.h"

pthread_mutex_t lock;

void *io_thread_func_3() {
  char *req_str = NULL;
  char *req_key = NULL;
  char *req_val = NULL;
  char *tmp_line = NULL;
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
      strtok_r(req_str, " ");
      req_key = strtok_r(NULL, " ");

      switch (pending_head->cont->request_type) {
        case GET:
          db_get(req_key, &db_get_val, &val_size);
          strncpy(pending_head->cont->result, db_get_val, val_size + 1);
          break;
        case PUT:
          req_val = strtok_r(NULL, "\n");
          db_put(req_key, req_val, &db_get_val, &val_size);
          strncpy(pending_head->cont->result, db_get_val, val_size);
          break;
        case INSERT:
          req_val = strtok_r(NULL, "\n");
          db_insert(req_key, req_val, &db_get_val, &val_size);
          strncpy(pending_head->cont->result, db_get_val, val_size);
          break;
        case DELETE:
          db_delete(req_key, &db_get_val, &val_size);
          strncpy(pending_head->cont->result, db_get_val, val_size);
          break;
      }

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

void setup_request_type(char *s, char **tec) {
  if (strcmp(s, "GET") == 0) {
    temp->request_type = GET;
    memcpy(*tec, "NULL", 5);
  } else if (strcmp(s, "PUT") == 0) {
    temp->request_type = PUT;
    memcpy(*tec, "NULL", 5);
  } else if (strcmp(s, "INSERT") == 0) {
    temp->request_type = INSERT;
    memcpy(*tec, "-1", 3);
  } else if (strcmp(s, "DELETE") == 0) {
    temp->request_type = DELETE;
    memcpy(*tec, "-1", 3);
  } else {
    temp->request_type = INVALID;
    memcpy(*tec, "-1", 3);
  }
}

/*
  This func is only executed by the main thread, and appends a task to the back of the
  task queue
*/
void issue_io_req_3() {
  pthread_mutex_lock(&lock);
  pending_node = (struct pending_queue *) malloc(sizeof(struct pending_queue));
  pending_node->cont = temp;
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
  int valread, incoming;
  char *req_string;
  char *req_type = NULL;
  char *req_key = NULL;
  char *req_val = NULL;
  struct sockaddr_in in;
  socklen_t sz = sizeof(in);
  char *tmp_err_code = (char *) calloc(5, sizeof(char));

  incoming = accept(initial_server_fd, (struct sockaddr *) &in, &sz);

  temp = (struct continuation *) malloc(sizeof(struct continuation));
  temp->start_time = time(0);
  memset(temp->buffer, 0, MAX_ENTRY_SIZE);
  valread = read(incoming, temp->buffer, MAX_ENTRY_SIZE);

  req_string = (char *) malloc(MAX_ENTRY_SIZE * sizeof(char));
  strcpy(req_string, temp->buffer);

  memset(temp->result, 0, MAX_ENTRY_SIZE);
  temp->fd = incoming;

  if (( req_type = strtok_r(req_string, " ")) == NULL) { // will ensure strlen>0
    printf("%s\n", "bad client request: req_type");
    // TODO: update timings since we send directly here (and below)
    send(temp->fd, "NULL", strlen("NULL"), 0);
    free(tmp_err_code);
    free(req_string);
    return;
  }

  setup_request_type(req_type, &tmp_err_code);

  if (( req_key = strtok_r(NULL, " ")) == NULL) {
    printf("%s\n", "bad client request: req_key");
    // TODO: update timings
    send(temp->fd, tmp_err_code, strlen(tmp_err_code), 0);
    free(tmp_err_code);
    free(req_string);
    return;
  }

  if (temp->request_type == GET) {
    if (( temp_node = cache_get(req_key)) != NULL) { // check cache
      printf("\nGET result found in cache\n\n");
      strcpy(temp->result, temp_node->defn);
      // TODO: update timings
      send(temp->fd, temp->result, strlen(temp->result), 0);
    } else { // not in cache, check db
      issue_io_req_3();
    }
  } else if (temp->request_type == PUT) {
    if (( req_val = strtok_r(NULL, " ")) == NULL) {
      printf("%s\n", "bad client request: req_val");
      // TODO: update timings
      send(temp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }
    issue_io_req_3();
  } else if (temp->request_type == INSERT) {
    if (( req_val = strtok_r(NULL, " ")) == NULL) {
      printf("%s\n", "bad client request: req_val");
      // TODO: update timings
      send(temp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }
    if (( temp_node = cache_get(req_key)) !=
        NULL) { // check if req_key in cache; yes - error: duplicate req_key violation, no - check db
      printf("%s\n", "error: duplicate req_key violation");
      // TODO: update timings
      send(temp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp_err_code);
      free(req_string);
      return;
    }
    issue_io_req_3(); // if not in cache, still might be in db
  } else if (temp->request_type == DELETE) {
    issue_io_req_3(); // issue io request to find out if req_key is in db to delete
  } else {
    // TODO: update timings
    send(temp->fd, tmp_err_code, strlen(tmp_err_code), 0);
    free(tmp_err_code);
    free(req_string);
    return;
  }

  free(tmp_err_code);
  free(req_string);
}

static void outgoing_data_handler_3(int sig, siginfo_t *si, void *data) {

  // update time measurements and cache in this func

  struct continuation *req_cont = (struct continuation *) si->si_value.sival_ptr;
  char *req_string = (char *) calloc(MAX_KEY_SIZE, sizeof(char));
  char *req_type = NULL;
  char *req_key = NULL;
  char *val = NULL;

  strcpy(req_string, req_cont->buffer); // buffer includes null byte
  req_type = strtok_r(req_string, " ");
  req_key = strtok_r(NULL, " ");

  if (req_cont->request_type == GET) {
    // TODO: update time measurements
    if (strcmp(req_cont->result, "NOTFOUND") != 0) { // if result was NULL there was some kind of error
      cache_put(req_key, req_cont->result);
    }
  } else if (req_cont->request_type == PUT) {
    // TODO: update time measurements
    if (strcmp(req_cont->result, "NOTFOUND") != 0) {
      val = strtok_r(NULL, " ");
      cache_put(req_key, val);
    }

  } else if (req_cont->request_type == INSERT) {
    // TODO: update time measurements
    if (strcmp(req_cont->result, "DUPLICATE") != 0) {
      val = strtok_r(NULL, " ");
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
  struct sigaction act, react;
  my_pid = getpid();

  pending_head = pending_tail = NULL;
  head = tail = curr = temp_node = NULL;

//  file = fopen("names.txt", "r+");

  act.sa_sigaction = incoming_connection_handler_3;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGRTMIN + 3, &act, NULL);

  react.sa_sigaction = outgoing_data_handler_3;
  sigemptyset(&react.sa_mask);
  react.sa_flags = SA_SIGINFO;
  sigaction(SIGRTMIN
            + 4, &react, NULL);

  // Creating I/O thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&io_thread[i], NULL, io_thread_func_3, NULL);
  }

  event_loop_scheduler_3();

  return 0;
}
