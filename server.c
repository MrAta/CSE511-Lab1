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
     if (initial_server_fd > max_fd)
          max_fd = initial_server_fd;
     int retval;
     //according to manpage, timeout should be 0 for polling
     struct timeval tv;
     tv.tv_sec = 0;
     tv.tv_usec = 0;

     //Watch fd to see when it has input TODO: should it be in while loop?
     fd_set rfds;

      if (listen(initial_server_fd, 5000) < 0) {
              perror("listen");
              exit(EXIT_FAILURE);
          }
while(1){
  FD_ZERO(&rfds);
  FD_SET(initial_server_fd, &rfds);
  for(int i=0; i < 5000; i++){
    if(sockfds[i] != -1){
      FD_SET(sockfds[i], &rfds);
    }
  }
     retval = select(max_fd+1, &rfds, NULL, NULL, &tv);
     if (retval == -1)
        perror("select()");

    else if (retval){
      if(FD_ISSET(initial_server_fd, &rfds)){
        struct sockaddr_in in;
        socketlen_t sz = sizeof(in);
        int new_fd = accept(initial_server_fd, (struct address *)&in, &sz);
        if (new_fd < 0){
          perror("Could not accept connection");
          continue;
        }

        if (new_fd > max_fd) max_fd = new_fd;

        FD_SET(new_fd, &rfds);
        for(int i=0; i < 5000; i++){
          if (sockfds[i] ==-1) {
            sockfds[i] = new_fds;
            break;
          }
        }
        printf("Data available\n" );
      }
      //check for others
      for (int i=0; i< 5000; i++){
        if(sockfds[i] != -1){
          if(FD_ISSET(sockfds[i], &rfds)){
            //read from it!
            int valread;
            char * req_string;
            char * req_type = NULL;
            char * req_key = NULL;
            char * req_val = NULL;

            temp = (struct continuation *)malloc(sizeof(struct continuation));
            temp->start_time = time(0);
            memset(temp->buffer, 0, MAX_ENTRY_SIZE);

            valread = read(sockfds[i], temp->buffer, MAX_ENTRY_SIZE);
            req_string = (char *)malloc(MAX_ENTRY_SIZE*sizeof(char));
            strcpy (req_string, temp->buffer);

            memset(temp->result, 0, MAX_ENTRY_SIZE);
            temp->fd = sockfds[i];

            if ((req_type = strtok(req_string, " ")) == NULL) { // will ensure strlen>0
              printf("%s\n", "bad client request: req_type");
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
             printf("%s\n", "bad client request: req_key");
             free(req_string);
             continue;
          }

            //perfom the request:

              if (temp->request_type == GET) {
                if ((temp_node = get(req_key)) != NULL) { // check cache
                  printf("\nGET result found in cache\n\n");
                  strcpy(temp->result, temp_node->defn);
                  //TODO: what should I do?

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
                //TODO: attaching req_val to temp node?
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


              //now what?

          }
        }
      }

    }
    else{
      printf("%d\n", retval );
      printf("No data available\n" );
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

  //initilize socket fds:
  for(int i=0; i< 5000) sockfds[i] = -1;

  //Creating I/O thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&io_thread[i], NULL, io_thread_func, NULL);
  }

  event_loop_scheduler();

  return 0;
}
