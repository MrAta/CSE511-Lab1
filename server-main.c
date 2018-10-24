//
// Created by sps5394 on 10/18/18.
//

#include <stdio.h>
#include <stdlib.h>
#include "server-part1.h"
#include "server-part2.h"
#include "server-part3.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Must provide value for what sort of server to run\n");
    return  1;
  }
  switch(atoi(argv[1])) {
    case 1:
      return run_server_1();
    case 2:
      return run_server_2();
    case 3:
      return run_server_3();
    default:
      printf("Illegal argument value. Exiting\n");
      return 1;
  }
}
