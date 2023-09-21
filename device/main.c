#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include "main.h"

// Server
#define ADDR "127.0.0.1"
#define PORT 12000

int main(int argc, char** argv) {
  int sockfd;
  const struct sockaddr_in servaddr = {
    .sin_family= AF_INET,
    .sin_addr.s_addr= inet_addr(ADDR),
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

  // assign IP, PORT
  /* servaddr.sin_family = AF_INET; */
  /* servaddr.sin_addr.s_addr = inet_addr(ADDR); */
  /* servaddr.sin_port = htons(PORT); */

  // connect the client socket to server socket
  if (connect(sockfd, (const struct sockaddr_in*)&servaddr, sizeof(servaddr))
    != 0) {
    printf("connection with the server failed...\n");
    exit(0);
  }
  else
  printf("connected to the server..\n");

  close(sockfd);
  printf("Hello World\n");
  exit(0);
}
