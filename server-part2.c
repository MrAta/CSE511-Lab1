#include "server-part2.h"

#define MAX_EVENTS 5000
int max_fd = 0;
int addrlen, opt = 1;
pthread_mutex_t lock;

void *io_thread_func_2() {


  // Complete this function to implement I/O functionality
  // for incoming requests and handle proper synchronization
  // among other helper threads
  char *req_str = NULL;
  char *req_type = NULL;
  char *req_key = NULL;
  char *req_val = NULL;
  char *tmp_line = NULL;
  char *tmp_line_copy = NULL;
  char *line_key = NULL;
  char *line_val = NULL;
  char *save_ptr;
  int found = 0, val_size = 0;
  struct stat st;
  int fsz = 0;
  char *fbuf = NULL;
  char *db_get_val = NULL;
  int fbuf_bytes = 0;

  while (1) {
    pthread_mutex_lock(&lock);
    db_connect();

    if (pending_head != NULL) {

      req_str = (char *) malloc(MAX_ENTRY_SIZE * sizeof(char));

      strcpy (req_str, pending_head->cont->buffer);

      // at this point request in buffer is valid string
      req_type = strtok_r(req_str, " ", &save_ptr);
      req_key = strtok_r(NULL, " ", &save_ptr);
      rewind(file);
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

      //writing to pipe_fd
      int pipe_write_res;
      pipe_write_res = write(pipe_fd[1], pending_head->cont, sizeof(struct continuation));//
      if (pipe_write_res < 0) {
        perror("write");
      }
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

/*
  This func is only executed by the main thread, and appends a task to the back of the
  task queue
*/
void issue_io_req_2() {
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

void on_read_from_pipe_2() {
  char *save_ptr;
  struct continuation *req_cont = (struct continuation *) malloc(sizeof(struct continuation));//TODO
  int read_pipe_res;
  read_pipe_res = read(pipe_fd[0], req_cont, sizeof(struct continuation));
  if (read_pipe_res < 0) {
    perror("read");
  }
  char *req_string = (char *) calloc(MAX_KEY_SIZE, sizeof(char));
  char *req_type = NULL;
  char *req_key = NULL;
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

static int make_socket_non_blocking_2(int sfd) {
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

int server_func_2() {
  int server_fd;
  addrlen = sizeof(address);

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

void event_loop_scheduler_2() {
  //create pipe fd for connecting threads in thread pool to main thread
  int pipe_res;
  pipe_res = pipe(pipe_fd);
  if (pipe_res < 0) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  //creating main thread fd
  initial_server_fd = server_func_2();
  make_socket_non_blocking_2(initial_server_fd);
  

  int retval;
  //our reading set for select
  struct epoll_event ev, events[MAX_EVENTS];
  int nfds, epollfd;
  if (listen(initial_server_fd, 5000) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  ev.events = EPOLLIN;
  ev.data.fd = initial_server_fd;

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, initial_server_fd, &ev) == -1) {
    perror("epoll_ctl: listen_sock");
    exit(EXIT_FAILURE);
  }

  ev.events = EPOLLIN;
  ev.data.fd = pipe_fd[0];

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pipe_fd[0], &ev) == -1) {
    perror("epoll_ctl: listen_sock");
    exit(EXIT_FAILURE);
  }
  while (1) {

    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == initial_server_fd) {
        struct sockaddr_in in;
        socklen_t sz = sizeof(in);
        //accept new connection and create its fd
        int new_fd = accept(initial_server_fd, (struct sockaddr_in *) &in, &sz);
        if (new_fd < 0) {
          perror("Could not accept connection");
          continue;
        }
        //setnonblocking(new_fd);
        make_socket_non_blocking_2(new_fd);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = new_fd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_fd,
                      &ev) == -1) {
          perror("epoll_ctl: conn_sock");
          exit(EXIT_FAILURE);
        }

      } else if (events[i].data.fd == pipe_fd[0]) {
        on_read_from_pipe_2();
      } else {
        //read from it!
        int valread;
        char *req_string;
        char *save_ptr;
        char *req_type = NULL;
        char *req_key = NULL;
        char *req_val = NULL;
        //create a new cont
        temp = (struct continuation *) malloc(sizeof(struct continuation));
        temp->start_time = time(0);
        memset(temp->buffer, 0, MAX_ENTRY_SIZE);

        valread = read(events[i].data.fd, temp->buffer, MAX_ENTRY_SIZE);
        if(valread < 0 ) continue;
        req_string = (char *) malloc(MAX_ENTRY_SIZE * sizeof(char));
        strcpy (req_string, temp->buffer);

        memset(temp->result, 0, MAX_ENTRY_SIZE);
        temp->fd = events[i].data.fd;

        if (( req_type = strtok_r(req_string, " ", &save_ptr)) == NULL) { // will ensure strlen>0
          // TODO: update timings since we send directly here (and below)
          free(req_string);
          continue;
        }
        if(req_type == NULL) continue;
        //checking reques type
        if (strcmp(req_type, "GET") == 0) {
          temp->request_type = GET;
        } else if (strcmp(req_type, "PUT") == 0) {
          temp->request_type = PUT;
        } else if (strcmp(req_type, "INSERT") == 0) {
          temp->request_type = INSERT;
        } else if (strcmp(req_type, "DELETE") == 0) {
          temp->request_type = DELETE;
        } else {
          temp->request_type = INVALID;
        }

        //set the key
        if (( req_key = strtok_r(NULL, " ", &save_ptr)) == NULL) {
          free(req_string);
          continue;
        }

        //perfom the request:

        if (temp->request_type == GET) {
          struct node * temp_node;
          if (( temp_node = cache_get(req_key)) != NULL) { // check cache
            printf("\nGET result found in cache\n\n");
            strcpy(temp->result, temp_node->defn);
            send(temp->fd, temp->result, strlen(temp->result), 0);

          } else { // not in cache, check db
            printf("\nnot in in cache\n\n");
            issue_io_req_2();
          }
        } else if (temp->request_type == PUT) {
          if (( req_val = strtok_r(NULL, " ", &save_ptr)) == NULL) {
            printf("%s\n", "bad client request: req_val");
            send(temp->fd, "BADVALUE", 8, 0);
            free(req_string);
            continue;
          }
          issue_io_req_2();
        } else if (temp->request_type == INSERT) {
          if (( req_val = strtok_r(NULL, " ", &save_ptr)) == NULL) {
            printf("%s\n", "bad client request: req_val");
            send(temp->fd, "BADVALUE", 8, 0);
            free(req_string);
            continue;
          }
          if (( temp_node = cache_get(req_key)) !=
              NULL) { // check if req_key in cache; yes - error: duplicate req_key violation, no - check db
            printf("%s\n", "error: duplicate req_key violation");
            send(temp->fd, "DUPLICATE", 9, 0);
            free(req_string);
            continue;
          }
          issue_io_req_2(); // if not in cache, still might be in db
        } else if (temp->request_type == DELETE) {
          issue_io_req_2(); // issue io request to find out if req_key is in db to delete
        } else {
          send(temp->fd, "BADCMD", 6, 0);
          free(req_string);
          continue;
        }
      }

    }
  } //end of while loop
  exit(EXIT_SUCCESS);

}

int run_server_2(void) {
  my_pid = getpid();

  pending_head = pending_tail = NULL;
  head = tail = temp_node = NULL;
  db_init();
  // file = fopen("names.txt", "r+");


  //Creating I/O thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&io_thread[i], NULL, io_thread_func_2, NULL);
  }

  event_loop_scheduler_2();

  return 0;
}
