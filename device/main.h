#ifndef MAIN_H
#define MAIN_H
#include <time.h>

struct args {
  char* address;
  char* filename;
  int   poll_rate;
  int   algorithm;
  int   duration; // seconds
};

int parseArgs(int argc, char** argv, struct args* args);
int sendMessage(char* address, char* message);
char* createMessage(int id, int algorithm, __time_t time, float* value);

#endif // MAIN_H
