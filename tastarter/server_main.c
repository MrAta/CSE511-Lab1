//
// Created by sps5394 on 10/18/18.
//

#include <stdio.h>
#include <stdlib.h>
#include "server-part1.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Must provide value for what sort of server to run");
    return  1;
  }
  switch(atoi(*argv)) {
    case 1:
      return run_server_1();
    default:
      printf("Illegal argument value. Exiting");
      return 1;
  }
}
