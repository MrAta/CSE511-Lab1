//
// Created by sps5394 on 10/18/18.
//
#include "server-part1.h"

struct sockaddr_in address;

int server_1_get_request(char *key, char **ret_buffer, int *ret_size) {
  //
  //
  // Local Variables
  //
  //
  struct node *cache_lookup;

  if (ret_buffer == NULL || ret_size == NULL || key == NULL) {
    return EXIT_FAILURE;
  }
  // Perform cache lookup
  cache_lookup = cache_get(key);
  if (cache_lookup != NULL) {
    // HIT
    *ret_buffer = calloc(1024, sizeof(char *));
    strcpy(*ret_buffer, cache_lookup->defn);
    *ret_size = (int) strlen(*ret_buffer);
    return 0;
  }
  // Perform IO
  if (db_get(key, ret_buffer, ret_size)) {
    return EXIT_FAILURE;
  }
  cache_put(key, *ret_buffer);
  return EXIT_SUCCESS;
}

int server_1_put_request(char *key, char *value, char **ret_buffer, int *ret_size) {
  // Always perform IO
  if (db_put(key, value, ret_buffer, ret_size)) {
    return EXIT_FAILURE;
  }
  cache_put(key, value);
  return EXIT_SUCCESS;
}

int server_1_insert_request(char *key, char *value, char **ret_buffer, int *ret_size) {
  // Always perform IO
  if (db_insert(key, value, ret_buffer, ret_size)) {
    return EXIT_FAILURE;
  }
  cache_put(key, value);
  return EXIT_SUCCESS;
}

int server_1_delete_request(char *key, char **ret_buffer, int *ret_size) {
  // Always perform IO
  if (db_delete(key, ret_buffer, ret_size)) {
    return EXIT_FAILURE;
  }
  cache_invalidate(key);
  return EXIT_SUCCESS;
}

void *server_handler(void *arg) {
  db_connect();
  int sockfd = *(int *) arg;
  char *input_line = (char *) calloc(1024, sizeof(char *));
  char *tokens, *response = NULL, *key, *value, *save_ptr;
  int response_size;
  read(sockfd, input_line, 1024);

  tokens = strtok_r(input_line, " ", &save_ptr);
  key = strtok_r(NULL, " ", &save_ptr);
  value = strtok_r(NULL, " ", &save_ptr);
  if(tokens == NULL || key == NULL || value == NULL) return NULL;
  if (strncmp(tokens, "GET", 3) == 0) {
    server_1_get_request(key, &response, &response_size);
    write(sockfd, response, (size_t) response_size);
  } else if (strncmp(tokens, "PUT", 3) == 0) {
    server_1_put_request(key, value, &response, &response_size);
    write(sockfd, response, (size_t) response_size);
  } else if (strncmp(tokens, "INSERT", 6) == 0) {
    server_1_insert_request(key, value, &response, &response_size);
    write(sockfd, response, (size_t) response_size);
  } else if (strncmp(tokens, "DELETE", 6) == 0) {
    server_1_delete_request(key, &response, &response_size);
    write(sockfd, response, (size_t) response_size);
  } else {
    write(sockfd, "ERROR", 6);
  }
  db_cleanup();
  if (response != NULL) {
    free(response);
  }
  return NULL;
}

int create_server_1() {
  int server_fd, opt;

  // Creating socket file descriptor
  if (( server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
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

int loop_and_listen_1() {
  int sock_fd = create_server_1();
  if (listen(sock_fd, QUEUED_CONNECTIONS) != 0) {
    perror("listen failed");
    return EXIT_FAILURE;
  }

  while (1) {
    socklen_t cli_addr_size = sizeof(address);
    int newsockfd = accept(sock_fd, (struct sockaddr *) &address, &cli_addr_size);
    if (newsockfd < 0) {
      perror("Could not accept connection");
      continue;
    }
    pthread_t *handler_thread = (pthread_t *) malloc(sizeof(pthread_t));
    if (pthread_create(handler_thread, NULL, server_handler, (void *) &newsockfd) != 0) {
      perror("Could not start handler");
      continue;
    }
    pthread_detach(*handler_thread);
  }
}

int run_server_1() {
  // Load database
  head = tail = curr = temp_node = NULL;

  if (loop_and_listen_1()) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
