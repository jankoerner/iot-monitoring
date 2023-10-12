#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "adaptive_sampling.h"

#define SAMPLE_RATE_DELTA 100
#define SAMPLE_DELTA_INC 1.01
#define SAMPLE_DELTA_DEC 0.99

float lastSample = 0.0;
int adaptiveSampling(float in, float* out, struct args* args){

  if (lastSample == 0.0){
    lastSample = in;
    *out = in;
    return 1;
  }

  float delta = fabs(in / lastSample);
  if (delta >= SAMPLE_DELTA_INC || delta <= SAMPLE_DELTA_DEC){
    lastSample = in;
    *out = in;

    if(args->poll_rate > SAMPLE_RATE_DELTA)
      args->poll_rate -= SAMPLE_RATE_DELTA;
    
    return 1;
  }

  args->poll_rate += SAMPLE_RATE_DELTA;
  return 0;
}
