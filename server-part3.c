#include "server-part3.h"
int sockfds[5000];
int max_fd = 0;
pthread_mutex_t lock;
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

void *io_thread_func() {


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
  int found = 0, val_size = 0;
  struct stat st;
  int fsz = 0;
  char *fbuf = NULL;
   char *db_get_val = NULL;
  int fbuf_bytes = 0;

  while(1) {
    pthread_mutex_lock(&lock);
    db_connect();

    if (pending_head != NULL) {

      req_str = (char *)malloc(MAX_ENTRY_SIZE*sizeof(char));

      strcpy (req_str, pending_head->cont->buffer);

      // at this point request in buffer is valid string
      req_type = strtok(req_str, " ");
      req_key = strtok(NULL, " ");
      rewind(file);
    switch (pending_head->cont->request_type) {
      case GET:
        db_get(req_key, &db_get_val, &val_size);
        strncpy(pending_head->cont->result, db_get_val, val_size + 1);
        break;
      case PUT:
        req_val = strtok(NULL, "\n");
        db_put(req_key, req_val, &db_get_val, &val_size);
        strncpy(pending_head->cont->result, db_get_val, val_size);
        break;
      case INSERT:
        req_val = strtok(NULL, "\n");
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
    if(pipe_write_res < 0){
      perror("write");
    }
    node *dead_head = pending_head;
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
void issue_io_req() {
  pthread_mutex_lock(&lock);
  pending_node = (struct pending_queue*) malloc (sizeof(struct pending_queue));
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
void on_read_from_pip(){
  struct continuation *req_cont = (struct continuation *)malloc(sizeof(struct continuation));//TODO
  int read_pipe_res;
  read_pipe_res = read(pipe_fd[0],req_cont, sizeof(struct continuation));
  if(read_pipe_res < 0){
    perror("read");
  }
  char *req_string = (char *)calloc(MAX_KEY_SIZE, sizeof(char));
  char *req_type = NULL;
  char *req_key = NULL;
  char *val = NULL;
  strcpy(req_string, req_cont->buffer); // buffer includes null byte
  req_type = strtok(req_string, " ");
  req_key = strtok(NULL, " ");
    if (req_cont->request_type == GET) {
     // TODO: update time measurements
     if(strcmp(req_cont->result, "NULL") != 0) { // if result was NULL there was some kind of error
       cache_put(req_key, req_cont->result);
     }
   } else if (req_cont->request_type == PUT) {
     // TODO: update time measurements
     if(strcmp(req_cont->result, "NULL") != 0) {
       val = strtok(NULL, " ");
       cache_put(req_key, val);
     }

   } else if (req_cont->request_type == INSERT) {
     // TODO: update time measurements
     if(strcmp(req_cont->result, "NULL") != 0) {
       val = strtok(NULL, " ");
       cache_put(req_key, val);
     }
   } else { // DELETE
     // TODO: update time measurements
     if (strcmp(req_cont->result, "NULL") != 0) {
      cache_invalidate(req_key);
          }
        }

  send(req_cont->fd, req_cont->result, strlen(req_cont->result), 0);
  free(req_string);
  free(req_cont); // frees the cont that was just executed
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
    //create pipe fd for connecting threads in thread pool to main thread
    int pipe_res;
    pipe_res = pipe(pipe_fd);
    if(pipe_res < 0){
      perror("pipe");
      exit(EXIT_FAILURE);
    }
    if(pipe_fd[0] > max_fd){
      max_fd = pipe_fd[0];
    }
    //creating main thread fd
    initial_server_fd = server_func();
    if (initial_server_fd > max_fd){
      max_fd = initial_server_fd;
    }
     int retval;
     //according to manpage, timeout should be 0 for polling
     struct timeval tv;
     tv.tv_sec = 0;
     tv.tv_usec = 0;

     //our reading set for select
     fd_set rfds;

      if (listen(initial_server_fd, 5000) < 0) {
              perror("listen");
              exit(EXIT_FAILURE);
          }
while(1){
  //set read list to zero and add main fd and pipe fd to the reading list for selectss
  FD_ZERO(&rfds);
  FD_SET(initial_server_fd, &rfds);
  FD_SET(pipe_fd[0], &rfds);
  //add/set socket fds to the reading list for select
  for(int i=0; i < 5000; i++){
    if(sockfds[i] != -1){
      FD_SET(sockfds[i], &rfds);
    }
  }
  //see which fd is available for read
     retval = select(max_fd+1, &rfds, NULL, NULL, &tv);
     if (retval == -1)//couldnt check them
        perror("select()");

    else if (retval){ //some fds are available.
      //first check for new connection on main fd
      if(FD_ISSET(initial_server_fd, &rfds)){
        struct sockaddr_in in;
        socklen_t sz = sizeof(in);
        //accept new connection and create its fd
        int new_fd = accept(initial_server_fd, (struct sockaddr_in *)&in, &sz);
        if (new_fd < 0){
          perror("Could not accept connection");

          continue;
        }
        //for select we need the max fd
        if (new_fd > max_fd) max_fd = new_fd;
        ///add the fd to readling list for select
        FD_SET(new_fd, &rfds);
        //add it to our array of fds for sockets
        for(int i=0; i < 5000; i++){
          if (sockfds[i] ==-1) {
            sockfds[i] = new_fd;
            break;
          }
        }
      }
      //check for pipe fd if available:
      if(FD_ISSET(pipe_fd[0], &rfds)){
        //read from pipe
        //send to cont->fd
        on_read_from_pip();
      }
      //check for other connections requests
      for (int i=0; i< 5000; i++){
        if(sockfds[i] != -1){
          //check if fd is available for read or not
          if(FD_ISSET(sockfds[i], &rfds)){
            //read from it!
            int valread;
            char * req_string;
            char * req_type = NULL;
            char * req_key = NULL;
            char * req_val = NULL;
            //create a new cont
            temp = (struct continuation *)malloc(sizeof(struct continuation));
            temp->start_time = time(0);
            memset(temp->buffer, 0, MAX_ENTRY_SIZE);

            valread = read(sockfds[i], temp->buffer, MAX_ENTRY_SIZE);
            req_string = (char *)malloc(MAX_ENTRY_SIZE*sizeof(char));
            strcpy (req_string, temp->buffer);

            memset(temp->result, 0, MAX_ENTRY_SIZE);
            temp->fd = sockfds[i];

            if ((req_type = strtok(req_string, " ")) == NULL) { // will ensure strlen>0
              // TODO: update timings since we send directly here (and below)
              send(temp->fd, "NULL", strlen("NULL"), 0);
              free(req_string);
              continue;
            }

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
          if ((req_key = strtok(NULL, " ")) == NULL) {
             send(temp->fd, "NULL", strlen("NULL"), 0);
             free(req_string);
             continue;
          }

            //perfom the request:

              if (temp->request_type == GET) {
                if ((temp_node = get(req_key)) != NULL) { // check cache
                  printf("\nGET result found in cache\n\n");
                  strcpy(temp->result, temp_node->defn);
                  send(temp->fd, temp->result, strlen(temp->result), 0);

                } else { // not in cache, check db
                  issue_io_req();
                }
            }
            else if (temp->request_type == PUT) {
                if ((req_val = strtok(NULL, " ")) == NULL) {
                  printf("%s\n", "bad client request: req_val");
                  send(temp->fd, "NULL", strlen("NULL"), 0);
                  free(req_string);
                  continue;
                }
                issue_io_req();
            }
            else if (temp->request_type == INSERT) {
                if ((req_val = strtok(NULL, " ")) == NULL) {
                  printf("%s\n", "bad client request: req_val");
                  send(temp->fd, "NULL", strlen("NULL"), 0);
                  free(req_string);
                  continue;
                }
                if ((temp_node = get(req_key)) != NULL) { // check if req_key in cache; yes - error: duplicate req_key violation, no - check db
                  printf("%s\n", "error: duplicate req_key violation");
                  send(temp->fd, "NULL", strlen("NULL"), 0);
                  free(req_string);
                  continue;
                }
                issue_io_req(); // if not in cache, still might be in db
                }
                else if (temp->request_type == DELETE) {
                issue_io_req(); // issue io request to find out if req_key is in db to delete
              } else {
                send(temp->fd, "NULL", strlen("NULL"), 0);
                free(req_string);
                continue;
                }

          }
        }
      }

    }// end of if retavl is a good value!
    else{
      continue;
    }

} //end of while loop
    exit(EXIT_SUCCESS);


}

int run_server_3 (void)
{
  my_pid = getpid();

  pending_head = pending_tail = NULL;
  head = tail = curr = temp_node = NULL;

  // file = fopen("names.txt", "r+");

  //initilize socket fds for client connections:
  for(int i=0; i< 5000; i++) sockfds[i] = -1;

  //Creating I/O thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&io_thread[i], NULL, io_thread_func, NULL);
  }

  event_loop_scheduler();

  return 0;
}
