#ifndef MAIN_H
#define MAIN_H

struct args {
  char* address;
  char* filename;
  int   poll_rate;
  int   algorithm;
  int   duration; // seconds
};

int parseArgs(int argc, char** argv, struct args* args);
int sendMessage(char* address, char* message);

#endif // MAIN_H
