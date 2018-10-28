#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#define PORT 8080
#define N_KEY 1000 //number of unique keys
#define a 1.25 //parameter for zipf distribution
#define ratio 0.1 // get/put ratio
#define key_size 16
#define value_size 84
long int zeta; //zeta fixed for zipf
char * keys[N_KEY] = {NULL};
char * values[N_KEY] = {NULL};
double popularities[N_KEY];
double cdf[N_KEY];
int arr_rate = 100;
int arr_type = 1;

static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
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

long int calc_zeta(){
   double sum = 0.0;
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
double zipf(int x){
  return (1/(pow(x,a)))/zeta;
}
void generate_popularities(){
  for(int i=0; i < N_KEY; i++){
    popularities[i] = zipf(i+1);
  }
}
void calc_cdf(){
  cdf[0] = popularities[0];
  for(int i=1; i < N_KEY; i++){
      cdf[i] = cdf[i-1] + popularities[i];
  }
}

char * next_key(){
  double p = (rand() / (RAND_MAX+1.0));
  //TODO: optimize it with binary search
  for(int i=0; i< N_KEY; i++)
    if(p < cdf[i]){
      return keys[i];
    }
    return NULL;
}

double nextArrival(){
    if (arr_type == 1){//unifrom
      return 1/arr_rate;
    }
    else{//exponential
      return (-1 * log(1 - (rand()/(RAND_MAX+1.0)))/(arr_rate));
    }
    return 1/arr_rate; //default: unifrom :D
}

int nextReqType(){
  double p = (rand() / (RAND_MAX+1.0));
  //req types: 0 indicates get requst, 1 indicates put request
  if(p <= ratio) return 0
  return 1;

}
char *hello[] = {"GET Tammy"};//{"GET Atajoon", "GET Clifton", "GET Tammy", "INSERT Sami Pali", "DELETE Tammy", "GET Tammy"};


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
for(int i=0; i < 6; i++){
  clock_t t;
  t = clock();
  send(sock , hello[0] , strlen(hello[0]) , 0 );
  printf("REQUEST SENT: %s\n", hello[0]);
  valread = read( sock , buffer, 1024);
  t = clock() - t;
  double time_taken = ((double)t)/CLOCKS_PER_SEC;
  printf("RESPONSE: %s took %f\n",buffer,  time_taken);
}

}

int main(int argc, char const *argv[])
{
    generate_key_values();
    generate_popularities();
    calc_cdf();

    pthread_t client_thread[10];

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

    // for(int i=0; i<10; i++)
    client_func();
    // pthread_create(&client_thread[i], NULL, client_func, NULL);
// for(int i=0; i<10; i++)
    // pthread_join(client_thread[i], NULL);

    return 0;
}
