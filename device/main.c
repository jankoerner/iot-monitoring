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
  args->poll_rate = 1000;
  args->algorithm = 0;
  args->duration = 300; // 5min

  if(argc > 1)
    parseArgs(argc, argv, args);

  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  struct timespec* start = malloc(sizeof(struct timespec));
  struct timespec* last_poll = malloc(sizeof(struct timespec));
  struct timespec* curr = malloc(sizeof(struct timespec));

  fp = fopen(args->filename, "r");
  if (fp == NULL)
    return -1;

  read = getline(&line, &len, fp);
  clock_gettime(CLOCK_REALTIME, start);
  while (read != -1 && args->duration > curr->tv_sec - start->tv_sec){
    clock_gettime(CLOCK_REALTIME, curr);
    // I hate this line with a passion
    if(last_poll == NULL || (((curr->tv_sec * 1000) + (curr->tv_nsec / 1000000)) - ((last_poll->tv_sec * 1000) + (last_poll->tv_nsec / 1000000))) > args->poll_rate){

      printf("%s", line); // Algo goes here

#if CONNECT_SERVER
      sendMessage(args->address, line);
#endif

      read = getline(&line, &len, fp);
      clock_gettime(CLOCK_REALTIME, last_poll);
    }
    else{
      usleep(100000); // 10ms
    }
  }

  if(line)
    free(line);

  fclose(fp);
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
    else if (strcmp(argv[i], "-d") == 0){
      args->duration = atoi(argv[++i]);
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

int sendMessage(char* address, char* message){
  int sockfd;
  const struct sockaddr_in servaddr = {
    .sin_family= AF_INET,
    .sin_addr.s_addr= inet_addr(address),
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

  close(sockfd);
  return 0;
}

