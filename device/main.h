#ifndef MAIN_H
#define MAIN_H

struct args {
  char* address;
  char* filename;
  int   poll_rate;
  int   algorithm;
};

int parseArgs(int argc, char** argv, struct args* args);

#endif // MAIN_H
