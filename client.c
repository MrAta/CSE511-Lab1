#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#define PORT 8080
#define N_KEY 1000 //number of unique keys
#define a 1.25 //parameter for zipf distribution
#define key_size 16
#define value_size 100
long int zeta; //zeta fixed for zipf
char * keys[N_KEY] = {NULL};
char * values[N_KEY] = {NULL};
float popularities[N_KEY];
float cdf[N_KEY];
long int calc_zeta(){
  long float sum = 0.0;
  for(int i=1; i < N_KEY+1; i++)
    sum += 1/(pow(i,a));

  return (long int)sum;
}

void generate_key_values(){
  for(int i=0; i< N_KEY; i++){
    char * _key = rand_string_alloc(key_size);
    keys[i] = _key;
    char * _value = rand_string_alloc(value_size);
    values[i] = _value;
    //TODO: do we need to write them in a file?
  }
}
float zipf(int x, int a, int N){
  return (1/(pow(x,a)))/zeta;
}
void generate_popularities(){
  for(int i=0; i < N_KEY; i++){
    popularities[i] = zipf(i+1, a, N);
  }
}
void calc_cdf(){
  cdf[0] = popularities[0];
  for(int i=1; i < N_KEY; i++){
      cdf[i] = cdf[i-1] + popularities[i];
  }
}
static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK0123456789";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}
char* rand_string_alloc(size_t size)
{
     char *s = malloc(size + 1);
     if (s) {
         rand_string(s, size);
     }
     return s;
}
char *hello[] = {"GET Atajoon"};
char *mello[] = {"GET Atajoon"};

struct sockaddr_in *serv_addr;

void *client_func() {
  int sock = 0, valread;

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

  send(sock , hello[0] , strlen(hello[0]) , 0 );
  printf("REQUEST SENT: %s\n", hello[0]);
  valread = read( sock , buffer, 1024);
  printf("RESPONSE: %s\n",buffer );

  send(sock , mello[0] , strlen(mello[0]) , 0 );
  printf("REQUEST SENT: %s\n", mello[0]);
  valread = read( sock , buffer, 1024);
  printf("RESPONSE: %s\n",buffer );

}

int main(int argc, char const *argv[])
{
    generate_key_values();
    generate_popularities();
    calc_cdf();
    
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
