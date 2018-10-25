#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#define PORT 8080

char *hello[] = {"INSERT Leslie Lamport"};
struct sockaddr_in *serv_addr;

void *client_func() {
  int sock = 0, valread;
  struct timespec start_time, end_time;
  char buffer[1024] = {0};

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
      printf("\n Socket creation error \n");
      exit(0);
  }

  if (connect(sock, (struct sockaddr *)serv_addr, sizeof(struct sockaddr_in)) < 0)
  {
      printf("\nConnection Failed \n");
      exit(0);
  }
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  send(sock , hello[0] , strlen(hello[0]) , 0 );
  printf("REQUEST SENT: %s\n", hello[0]);
  valread = read( sock , buffer, 1024);
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  printf("RESPONSE: %s\n",buffer );
  printf("Request took %lu ns\n", end_time.tv_nsec - start_time.tv_nsec);
  return NULL;
}

int main(int argc, char const *argv[])
{
    pthread_t client_thread;

    serv_addr = (struct sockaddr_in*) malloc (sizeof(struct sockaddr_in));

    memset(serv_addr, '0', sizeof(serv_addr));

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr->sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    pthread_create(&client_thread, NULL, client_func, NULL);

    pthread_join(client_thread, NULL);

    return 0;
}
