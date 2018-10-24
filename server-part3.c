#include "server.h"
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
  int found = 0;
  struct stat st;
  int fsz = 0;
  char *fbuf = NULL;
  int fbuf_bytes = 0;

  while(1) {
    pthread_mutex_lock(&lock);

    if (pending_head != NULL) {

      req_str = (char *)malloc(MAX_ENTRY_SIZE*sizeof(char));

      strcpy (req_str, pending_head->cont->buffer);

      // at this point request in buffer is valid string
      req_type = strtok(req_str, " ");
      req_key = strtok(NULL, " ");
      rewind(file);
    switch (pending_head->cont->request_type) {
      case GET:
        tmp_line = (char *)calloc(MAX_ENTRY_SIZE, sizeof(char));
        while(1){
          if(fgets(tmp_line, MAX_ENTRY_SIZE, file) != NULL){
            line_key = strtok(tmp_line, " ");
            if(strcmp(line_key, req_key) == 0){
              //ke in db
              line_val = strtok(NULL, " ");
              // printf("119 line valL %s\n",line_val );
              strncpy(pending_head->cont->result, line_val, strlen(line_val));
              strncpy(pending_head->cont->result+strlen(line_val), "\0", 1); // copy null byte too
              break;
            }else{
              continue;
            }
          }else{
            strcpy(pending_head->cont->result, "NULL");
            break;
          }
        }
        free(tmp_line);
        break;
      case PUT:
        req_val = strtok(NULL, "\n");
        found = 0;
        tmp_line = (char *)calloc(MAX_ENTRY_SIZE, sizeof(char));
        tmp_line_copy = (char *)calloc(MAX_ENTRY_SIZE, sizeof(char));
        stat("names.txt", &st);
        fsz = st.st_size;
        fbuf = (char *)calloc(fsz+MAX_ENTRY_SIZE, sizeof(char)); // need enough buffer space for the entire file plus an additional entry assuming the key isnt in the db
        fbuf_bytes = 0;
        while(1){
          if(fgets(tmp_line, MAX_ENTRY_SIZE, file) != NULL){
            memset(tmp_line_copy, 0, MAX_ENTRY_SIZE);
            memcpy(tmp_line_copy, tmp_line, strlen(tmp_line));
            line_key = strtok(tmp_line, " ");
            if(strcmp(line_key, req_key) == 0){
              //key in db
              found = 1;
              continue;
            }else{
              //go to next line
            memcpy(fbuf+fbuf_bytes, tmp_line_copy, strlen(tmp_line_copy));
            fbuf_bytes = fbuf_bytes + strlen(tmp_line_copy);
            continue;
            }
          }else{
            break;
          }
        }
        if(!found){
          strcpy(pending_head->cont->result, "NULL"); // key not found in db to do put, return NULL, dont update to file
        }else{
           // key found in db, add the new k/v to the end of file
           memcpy(fbuf+fbuf_bytes, req_key, strlen(req_key));
           memcpy(fbuf+fbuf_bytes+strlen(req_key), " ", 1);
           memcpy(fbuf+fbuf_bytes+strlen(req_key)+1, req_val, strlen(req_val));
           memcpy(fbuf+fbuf_bytes+strlen(req_key)+1+strlen(req_val), "\n", 1);

           rewind(file);
           fwrite(fbuf, sizeof(char), fbuf_bytes+strlen(req_key)+1+strlen(req_val)+1, file);
           fflush(file);
           ftruncate(fileno(file), fbuf_bytes+strlen(req_key)+1+strlen(req_val)+1);
           strncpy(pending_head->cont->result, "0", sizeof("0"));
        }
        free(tmp_line_copy);
        free(tmp_line);
        free(fbuf);
        break;
      case INSERT:
                 req_val = strtok(NULL, "\n");
                 found = 0;
                 tmp_line = (char *)calloc(MAX_ENTRY_SIZE, sizeof(char));
                 tmp_line_copy = (char *)calloc(MAX_ENTRY_SIZE, sizeof(char));
                 stat("names.txt", &st);
                 fsz = st.st_size;
                 fbuf = (char *)calloc(fsz+MAX_ENTRY_SIZE, sizeof(char));
                 fbuf_bytes = 0;
                 while (1) {
                   if(fgets(tmp_line, MAX_ENTRY_SIZE, file) !=  NULL) {
                     memset(tmp_line_copy, 0, MAX_ENTRY_SIZE);
                     memcpy(tmp_line_copy, tmp_line, strlen(tmp_line));
                     line_key = strtok(tmp_line, " ");
                     if (strcmp(line_key, req_key) == 0) { // found key in db; error
                       found = 1;
                       break;
                     } else { // line key doesnt match search key, go to next line
                       memcpy(fbuf+fbuf_bytes, tmp_line_copy, strlen(tmp_line_copy));
                       fbuf_bytes += strlen(tmp_line_copy);
                       continue;
                     }
                   } else { // end of file
                     break;
                   }
                 }
                 if (found) { // duplicate key error
                   strcpy(pending_head->cont->result, "NULL");
                 } else {
                   // key not found in db, add the new k/v to the end of file
                   memcpy(fbuf+fbuf_bytes, req_key, strlen(req_key));
                   memcpy(fbuf+fbuf_bytes+strlen(req_key), " ", 1);
                   memcpy(fbuf+fbuf_bytes+strlen(req_key)+1, req_val, strlen(req_val));
                   memcpy(fbuf+fbuf_bytes+strlen(req_key)+1+strlen(req_val), "\n", 1);

                   rewind(file);
                   fwrite(fbuf, sizeof(char), fbuf_bytes+strlen(req_key)+1+strlen(req_val)+1, file);
                   fflush(file);
                   ftruncate(fileno(file), fbuf_bytes+strlen(req_key)+1+strlen(req_val)+1);
                   strncpy(pending_head->cont->result, "0", sizeof("0"));
                 }
                 free(tmp_line_copy);
                 free(tmp_line);
                 free(fbuf);
                 break;
      case DELETE:
          req_val = strtok(NULL, "\n");
          found = 0;
          tmp_line = (char *)calloc(MAX_ENTRY_SIZE, sizeof(char));
          tmp_line_copy = (char *)calloc(MAX_ENTRY_SIZE, sizeof(char));
          stat("names.txt", &st);
          fsz = st.st_size;
          fbuf = (char *)calloc(fsz+MAX_ENTRY_SIZE, sizeof(char));
          fbuf_bytes = 0;
          while (1) {
            if(fgets(tmp_line, MAX_VALUE_SIZE, file) !=  NULL) {
              memset(tmp_line_copy, 0, MAX_VALUE_SIZE);
              memcpy(tmp_line_copy, tmp_line, strlen(tmp_line));
              line_key = strtok(tmp_line, " ");
              if (strcmp(line_key, req_key) == 0) { // found key in db; ignore this line for file rewrite
                found = 1;
                continue;
              } else { // line key doesnt match search key, copy to buffer and go to next line
                memcpy(fbuf+fbuf_bytes, tmp_line_copy, strlen(tmp_line_copy));
                fbuf_bytes += strlen(tmp_line_copy);
                continue;
              }
            } else { // must read to end of file
              break;
            }
          }
          if (!found) {
            strcpy(pending_head->cont->result, "-1"); // key not found in db to do delete, return -1, dont update to file
          } else { // key found in db, write fbuf to file without that entry
            rewind(file);
            fwrite(fbuf, sizeof(char), fbuf_bytes, file);
            fflush(file);
            ftruncate(fileno(file), fbuf_bytes);
            strncpy(pending_head->cont->result, "0", sizeof("0"));
          }
          free(tmp_line_copy);
          free(tmp_line);
          free(fbuf);
          break;
    }
    //TODO: notify main thread!
    //writing to pipe_fd
    int pipe_write_res;
    pipe_write_res = write(pipe_fd[1], pending_head->cont, sizeof(struct continuation));//
    if(pipe_write_res < 0){
      perror("write");
    }
    free(pending_head);
    pending_head = pending_head->next; // free the pending_node in task queue, the cont is freed in outgoing
  }
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
       put(req_key, req_cont->result);
     }
   } else if (req_cont->request_type == PUT) {
     // TODO: update time measurements
     if(strcmp(req_cont->result, "NULL") != 0) {
       val = strtok(NULL, " ");
       put(req_key, val);
     }

   } else if (req_cont->request_type == INSERT) {
     // TODO: update time measurements
     if(strcmp(req_cont->result, "NULL") != 0) {
       val = strtok(NULL, " ");
       put(req_key, val);
     }
   } else { // DELETE
     // TODO: update time measurements
     if(strcmp(req_cont->result, "NULL") != 0) {
       if ((temp_node = get(req_key)) != NULL) { // if in cache, delete it
         curr = head;
         head->next->prev = NULL;
         head = head->next;
         free(curr);
       }
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
                  free(req_string);
                  continue;
                }
                issue_io_req();
            }
            else if (temp->request_type == INSERT) {
                if ((req_val = strtok(NULL, " ")) == NULL) {
                  printf("%s\n", "bad client request: req_val");
                  free(req_string);
                  continue;
                }
                if ((temp_node = get(req_key)) != NULL) { // check if req_key in cache; yes - error: duplicate req_key violation, no - check db
                  printf("%s\n", "error: duplicate req_key violation");
                  free(req_string);
                  continue;
                }
                issue_io_req(); // if not in cache, still might be in db
                }
                else if (temp->request_type == DELETE) {
                issue_io_req(); // issue io request to find out if req_key is in db to delete
              } else {
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

int main (void)
{
  my_pid = getpid();

  pending_head = pending_tail = NULL;
  head = tail = curr = temp_node = NULL;

  file = fopen("names.txt", "r+");

  //initilize socket fds for client connections:
  for(int i=0; i< 5000; i++) sockfds[i] = -1;

  //Creating I/O thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&io_thread[i], NULL, io_thread_func, NULL);
  }

  event_loop_scheduler();

  return 0;
}
