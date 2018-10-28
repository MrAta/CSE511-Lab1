#include "server-part3.h"

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
      // pthread_mutex_lock(&lock);
      // db_connect();
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

      v = (union sigval *) malloc(sizeof(union sigval));
      v->sival_ptr = pending_head->cont;
      sigqueue(my_pid, SIGRTMIN + 4, *v); // send signal to main thread
      struct pending_queue *dead_head = pending_head;
      pending_head = pending_head->next; // free the pending_node in task queue, the cont is freed in outgoing
      free(dead_head);
      free(db_get_val);
      free(req_str);

      // db_cleanup();
      // pthread_mutex_unlock(&lock);
    }

    db_cleanup();
    pthread_mutex_unlock(&lock);
  }
}

void issue_io_req_3(struct continuation *tmp) {
  pthread_mutex_lock(&lock); // prevent races b/w main thread and helper threads; might not need these tho?
  struct pending_queue *tmp2 = NULL;
  tmp2 = (struct pending_queue *) malloc(sizeof(struct pending_queue));
  tmp2->cont = tmp;
  tmp2->next = NULL;
  if (pending_head == NULL) {
    // pthread_mutex_lock(&lock);
    pending_head = pending_tail = tmp2;
    // pthread_mutex_unlock(&lock);
  } else {
    // pthread_mutex_lock(&lock);
    pending_tail->next = tmp2;
    pending_tail = tmp2;
    // pthread_mutex_unlock(&lock);
  }
  pthread_mutex_unlock(&lock);
}

static void handle_sig_pipe(int sig, siginfo_t *si, void *data) {
  // printf("==============in sigpipe handler=================");
  fcntl(si->si_value.sival_int, F_SETSIG, SIGIO);
  close(si->si_value.sival_int);
}

static void incoming_connection_handler_3(int sig, siginfo_t *si, void *data) {
  incoming_events++;
  char fd_buf[4];
  if(si->si_value.sival_int == initial_server_fd) { // if signal on main socket
    write(sig_handler_pipe[1], NEW_CONN, 1); // write type
    bytes_written_to_pipe++;
  } else { // signal for some other socket
    write(sig_handler_pipe[1], EXISTING_CONN, 1); 
    write(sig_handler_pipe[1], &(si->si_value.sival_int), sizeof(int));
    printf("si->si_value.sival_int: %d\n", si->si_value.sival_int);
    bytes_written_to_pipe += 5;
  }
}

void incoming_connection_handler_3_helper_new() {
  int incoming;
  struct sockaddr_in in;
  socklen_t sz = sizeof(in);
  incoming = accept(initial_server_fd, (struct sockaddr *) &in, &sz);
  if (incoming == -1) { perror(strerror(errno)); }
  int fl;
  fl = fcntl(incoming, F_GETFL);
  fl |= O_ASYNC | O_NONBLOCK; // send a signal to main thread on fd ready
  fcntl(incoming, F_SETFL, fl);
  fcntl(incoming, F_SETSIG, SIGRTMIN + 3);
  fcntl(incoming, F_SETOWN, getpid());

  incoming_connection_handler_3_helper(incoming);
}

void incoming_connection_handler_3_helper(int sock) {
  int valread;
  char *req_string;
  char *req_type = NULL;
  char *req_key = NULL;
  char *req_val = NULL;
  char *save_ptr;
  char *tmp_err_code = "NULL";
  struct continuation *tmp = NULL;
  struct node *tmp_node = NULL;

  tmp = (struct continuation *) malloc(sizeof(struct continuation));
  tmp->start_time = time(0);
  memset(tmp->buffer, 0, MAX_ENTRY_SIZE);
  valread = read(sock, tmp->buffer, MAX_ENTRY_SIZE);

  if (valread <= 0) {
    close(sock);
    return;
  }

  req_string = (char *) malloc(MAX_ENTRY_SIZE * sizeof(char));
  strcpy(req_string, tmp->buffer);

  memset(tmp->result, 0, MAX_ENTRY_SIZE);
  tmp->fd = sock;

  if (( req_type = strtok_r(req_string, " ", &save_ptr)) == NULL) { // will ensure strlen>0
    printf("%s\n", "bad client request: req_type");
    send(tmp->fd, "NULL", strlen("NULL"), 0);
    free(tmp);
    free(req_string);
    return;
  }

  if (( req_key = strtok_r(NULL, " ", &save_ptr)) == NULL) {
    printf("%s\n", "bad client request: req_key");
    send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
    free(tmp);
    free(req_string);
    return;
  }

  if (strcmp(req_type, "GET") == 0) {
    tmp->request_type = GET;
    if (( tmp_node = cache_get(req_key)) != NULL) { // check cache
      printf("\nGET result found in cache\n\n");
      strcpy(tmp->result, tmp_node->defn);
      send(tmp->fd, tmp->result, strlen(tmp->result), 0);
      free(tmp);
    } else { // not in cache, check db
      issue_io_req_3(tmp);
    }
  } else if (strcmp(req_type, "PUT") == 0) {
    tmp->request_type = PUT;
    if (( req_val = strtok_r(NULL, " ", &save_ptr)) == NULL) {
      printf("%s\n", "bad client request: req_val");
      send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp);
      free(req_string);
      return;
    }
    issue_io_req_3(tmp);
  } else if (strcmp(req_type, "INSERT") == 0) {
    tmp->request_type = INSERT;
    if (( req_val = strtok_r(NULL, " ", &save_ptr)) == NULL) {
      printf("%s\n", "bad client request: req_val");
      send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp);
      free(req_string);
      return;
    }
    if (( tmp_node = cache_get(req_key)) !=
        NULL) { // check if req_key in cache; yes - error: duplicate req_key violation, no - check db
      printf("%s\n", "error: duplicate req_key violation");
      send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
      free(tmp);
      free(req_string);
      return;
    }
    issue_io_req_3(tmp); // if not in cache, still might be in db
  } else if (strcmp(req_type, "DELETE") == 0) {
    tmp->request_type = DELETE;
    issue_io_req_3(tmp); // issue io request to find out if req_key is in db to delete
  } else {
    send(tmp->fd, tmp_err_code, strlen(tmp_err_code), 0);
    free(tmp);
    free(req_string);
    return;
  }

  free(req_string);
}

static void outgoing_data_handler_3(int sig, siginfo_t *si, void *data) {
  incoming_events++;
  write(sig_handler_pipe[1], OUTGOING_DATA, 1);
  write(sig_handler_pipe[1], &(si->si_value.sival_ptr), sizeof(void *));
  bytes_written_to_pipe += 9;
}

void outgoing_data_handler_3_helper(struct continuation *req_cont) {
  pthread_mutex_lock(&lock); // prevent helper threads from messing with req_cont

  char *req_string = (char *) calloc(MAX_KEY_SIZE, sizeof(char));
  char *req_type = NULL;
  char *req_key = NULL;
  char *save_ptr;
  char *val = NULL;

  strcpy(req_string, req_cont->buffer);
  req_type = strtok_r(req_string, " ", &save_ptr);
  req_key = strtok_r(NULL, " ", &save_ptr);

  if (req_cont->request_type == GET) {
    if (strcmp(req_cont->result, "NOTFOUND") != 0) { // if result was NULL there was some kind of error
      cache_put(req_key, req_cont->result);
    }
  } else if (req_cont->request_type == PUT) {
    if (strcmp(req_cont->result, "NOTFOUND") != 0) {
      val = strtok_r(NULL, " ", &save_ptr);
      cache_put(req_key, val);
    }

  } else if (req_cont->request_type == INSERT) {
    if (strcmp(req_cont->result, "DUPLICATE") != 0) {
      val = strtok_r(NULL, " ", &save_ptr);
      cache_put(req_key, val);
    }
  } else { // DELETE
    if (strcmp(req_cont->result, "NOTFOUND") != 0) {
      cache_invalidate(req_key);
    }
  }

  if ((send(req_cont->fd, req_cont->result, strlen(req_cont->result), 0) != strlen(req_cont->result))) {
    printf("not fully sent");
  }
  free(req_string);
  free(req_cont); // frees the cont that was just executed

  pthread_mutex_unlock(&lock);
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
  // make_socket_non_blocking_3(initial_server_fd); // redundant
  int fl;
  fl = fcntl(initial_server_fd, F_GETFL);
  fl |= O_ASYNC | O_NONBLOCK;
  fcntl(initial_server_fd, F_SETFL, fl);
  fcntl(initial_server_fd, F_SETSIG, SIGRTMIN + 3);
  fcntl(initial_server_fd, F_SETOWN, getpid());

  if (listen(initial_server_fd, 5000) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // printf("Waiting for a signal\n");
    // pause();
    // printf("Got some event on fd\n");

    sigprocmask(SIG_BLOCK, &block_rtsigs_set, NULL); // blocks any signals while the event is serviced, new signals are queued up
    if (incoming_events > 0) { // service all events pushed on pipe, in order
      char type[1];
      // char fd_buf[sizeof(int)];
      int type_bytes_read;

      int bytes_ready = 0;
      ioctl(sig_handler_pipe[0], FIONREAD, &bytes_ready);
      // printf("bytes_ready before anything: %d\n", bytes_ready);

      type_bytes_read = read(sig_handler_pipe[0], type, 1);
      bytes_read_from_pipe++;
      int fd_bytes_read = -1;
      int req_cont_bytes_read = -1;
      // int sock = -1;
      int sock = 0;
      void **req_cont = NULL;

      if(strncmp(NEW_CONN, type, 1) == 0) { // if new connection
        incoming_connection_handler_3_helper_new();
      } else if(strncmp(EXISTING_CONN, type, 1) == 0) { // if existing connection
        // fd_bytes_read = read(sig_handler_pipe[0], fd_buf, sizeof(int));
        fd_bytes_read = read(sig_handler_pipe[0], &sock, sizeof(int));
        bytes_read_from_pipe += 4;
        // printf("sock before convert: %s\n", fd_buf);
        // sock = atoi(fd_buf);
        // sock = (int)fd_buf;
        printf("sock in event loop: %d\n", sock);

        // if (sock < 5) {
        //   if (sock < 3) {
        //     printf("closing: %d\n", sock);
        //     close(sock);
        //   }
        //   incoming_events--;
        //   sigprocmask(SIG_UNBLOCK, &block_rtsigs_set, NULL);
        //   continue;
        // }
        ioctl(sig_handler_pipe[0], FIONREAD, &bytes_ready);
        // printf("bytes_ready before helper: %d\n", bytes_ready);
        incoming_connection_handler_3_helper(sock);
      } else if(strncmp(OUTGOING_DATA, type, 1) == 0) { // if outgoing data
        req_cont = (void **)calloc(sizeof(void *), sizeof(char));
        req_cont_bytes_read = read(sig_handler_pipe[0], req_cont, sizeof(void *));
        bytes_read_from_pipe+=8;
        outgoing_data_handler_3_helper((struct continuation *)*req_cont);
        free(req_cont);
      } else {
        exit(1); // TODO
      }

      incoming_events--;
    }
    sigprocmask(SIG_UNBLOCK, &block_rtsigs_set, NULL);
  }
}

void *setup_sigs_and_exec_thread() {
  if (pthread_self() != main_tid) {
    sigset_t mask1;
    sigemptyset(&mask1);
    sigaddset(&mask1, SIGRTMIN+3); // block these always
    sigaddset(&mask1, SIGRTMIN+4);
    pthread_sigmask(SIG_BLOCK, &mask1, NULL);

    io_thread_func_3();
  } else { // if main thread
    struct sigaction act, react;
    act.sa_sigaction = incoming_connection_handler_3;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGRTMIN+3); // block these while incoming is running
    sigaddset(&act.sa_mask, SIGRTMIN+4);
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGRTMIN + 3, &act, NULL);

    react.sa_sigaction = outgoing_data_handler_3;
    sigemptyset(&react.sa_mask);
    sigaddset(&react.sa_mask, SIGRTMIN+3); // block these while outgoing is running
    sigaddset(&react.sa_mask, SIGRTMIN+4);
    react.sa_flags = SA_SIGINFO;
    sigaction(SIGRTMIN + 4, &react, NULL);

    struct sigaction act2;
    act2.sa_sigaction = handle_sig_pipe;
    sigemptyset(&act2.sa_mask);
    act2.sa_flags = SA_SIGINFO;
    sigaction(SIGPIPE, &act2, NULL);
  }
}

int run_server_3(void) {
  my_pid = getpid();
  main_tid = pthread_self();

  pending_head = pending_tail = NULL;
  head = tail = temp_node = NULL;
  incoming_events = 0;
  bytes_written_to_pipe = 0;
  bytes_read_from_pipe = 0;

  sigemptyset(&block_rtsigs_set);
  sigaddset(&block_rtsigs_set, SIGRTMIN + 3);
  sigaddset(&block_rtsigs_set, SIGRTMIN + 4);

  if (pipe(sig_handler_pipe) == -1) {
    perror("pipe error");
    exit(EXIT_FAILURE);
  }

  // make_socket_non_blocking_3(sig_handler_pipe[0]);
  // make_socket_non_blocking_3(sig_handler_pipe[1]);

  setup_sigs_and_exec_thread(); // for main thread

  db_init();

  // Creating I/O thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&io_thread[i], NULL, setup_sigs_and_exec_thread, NULL);
  }

  event_loop_scheduler_3();

  return 0;
}

