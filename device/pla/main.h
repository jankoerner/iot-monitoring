#ifndef MAIN_H
#define MAIN_H
#include <time.h>

struct args {
  char* address;
  char* filename;
  char* idpath;
  int   poll_rate;
  int   duration; // seconds
};

struct time_val {
  float value;
  long long time;
};

int parseArgs(int argc, char** argv, struct args* args);
int sendMessage(char* address, char* m1, char* m2);
void createMessage(int id, int algorithm, long long time, float value, char* message);

// PLA stuff

struct piece{
  double slope;
  double offset;
};
int pla(struct time_val* tv_in, struct time_val* tv_out);
int linreg(int n, const double x[], const double y[], double* m, double* b, double* r);


#endif // MAIN_H
