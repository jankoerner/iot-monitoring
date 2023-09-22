#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include "main.h"

#define CONNECT_SERVER 0

// Server
#define PORT 12000

int main(int argc, char** argv) {
  struct args* args = malloc(sizeof(struct args));
  args->address = "127.0.0.1";
  args->poll_rate = 1;
  args->algorithm = 0;

  if(argc > 1)
    parseArgs(argc, argv, args);

#if CONNECT_SERVER
  int sockfd;
  const struct sockaddr_in servaddr = {
    .sin_family= AF_INET,
    .sin_addr.s_addr= inet_addr(args->address),
    .sin_port= htons(PORT)
  };

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1){
    printf("socket creation failed...\n");
    exit(0);
  }
  else{
    printf("Socket successfully created..\n");
  }

  // connect the client socket to server socket
  if (connect(sockfd, (const struct sockaddr_in*)&servaddr, sizeof(servaddr))
    != 0) {
    printf("connection with the server failed...\n");
    exit(0);
  }
  else
  printf("connected to the server..\n");
#endif



  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  struct timespec* start = malloc(sizeof(struct timespec));
  struct timespec* end = malloc(sizeof(struct timespec));

  fp = fopen(args->filename, "r");
  if (fp == NULL)
    return -1;

  read = getline(&line, &len, fp);
  while (read != -1) {
    clock_gettime(CLOCK_REALTIME, end);
    if(start == NULL || (end->tv_sec - start->tv_sec) > args->poll_rate){

      printf("%s", line); // Algo goes here

      read = getline(&line, &len, fp);
      clock_gettime(CLOCK_REALTIME, start);
    }
    else
      sleep(0.1);
  }

  if(line)
    free(line);

  fclose(fp);
#if CONNECT_SERVER
  close(sockfd);
#endif
  free(args);
  exit(0);
}

int parseArgs(int argc, char** argv, struct args* args){
  int i;
  for (i = 1; i<argc; i++){
    if (strcmp(argv[i], "-f") == 0){
      args->filename = argv[++i];
    }
    else if (strcmp(argv[i], "-t") == 0){
      args->poll_rate = atoi(argv[++i]);
    }
    else if (strcmp(argv[i], "-a") == 0){
      args->algorithm = atoi(argv[++i]);
    }
    else if (strcmp(argv[i], "-A") == 0){
      args->address = argv[++i];
    }
    else {
      printf("invalid argument: %s\n", argv[i]);
      return -1;
    }
  }

  return 0;
}
