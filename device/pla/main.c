#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include "main.h"

// Server
#define PORT 12000

int main(int argc, char** argv) {
  int id = -1;
  struct args* args = malloc(sizeof(struct args));
  args->address = "127.0.0.1";
  args->poll_rate = 1000;
  args->duration = 300; // 5min
  args->filename = "../data/data.csv";
  args->idpath = "../data/id.txt";

  if(argc > 1)
    parseArgs(argc, argv, args);

  FILE* idpath;
  idpath = fopen(args->idpath, "r");
  if (idpath == NULL)
    return -1;
  char* idline = NULL;
  size_t idlen = 0;
  if(getline(&idline, &idlen, idpath) != -1){
    id = atoi(idline);
  }
  fclose(idpath);


  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  struct timespec* start = malloc(sizeof(struct timespec));
  struct timespec* curr = malloc(sizeof(struct timespec));

  fp = fopen(args->filename, "r");
  if (fp == NULL)
    return -1;

  struct time_val* tv_in = malloc(sizeof(struct time_val));
  struct time_val* tv_out = malloc(sizeof(struct time_val) * 2);
  char* end = malloc(sizeof(char));
  char* m1;
  char* m2;
  long long curr_us, last_ms = -1;
  read = getline(&line, &len, fp);
  clock_gettime(CLOCK_REALTIME, start);
  while (read != -1 && args->duration > curr->tv_sec - start->tv_sec){
    clock_gettime(CLOCK_REALTIME, curr);
    curr_us = (curr->tv_sec * 1000000LL) + (curr->tv_nsec / 1000);
    
    if(last_ms == -1 || ((curr_us/1000) - last_ms) > args->poll_rate){
      tv_in->value = strtod(line, &end);
      tv_in->time  = curr_us;
      if (pla(tv_in, tv_out)){
        m1 = malloc(sizeof(char) * 100);
        m2 = malloc(sizeof(char) * 100);
        createMessage(id, 6, tv_out[0].time, tv_out[0].value, m1);
        createMessage(id, 6, tv_out[1].time, tv_out[1].value, m2);
        printf("\no: %so: %s\n", m1, m2);
#ifdef CONNECT_SERVER
        sendMessage(args->address, m1, m2);
#endif
        free(m1);
        free(m2);
      }


      read = getline(&line, &len, fp);
      last_ms = curr_us/1000;
    }
    else{
      usleep(1000); // 1ms
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

int sendMessage(char* address, char* m1, char* m2){
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
  send(sockfd, m1, strlen(m1), 0);
  send(sockfd, m2, strlen(m2), 0);

  close(sockfd);
  return 0;
}

void createMessage(int id, int algorithm, long long time, float value, char* message){
  sprintf(message, "%d,%d,%lld,%f\n", id, algorithm, time, value);
}

