#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "adaptive_sampling.h"

#define SAMPLE_DELTA_INC 1.01
#define SAMPLE_DELTA_DEC 0.99

int sample_rate_delta = -1;
float lastSample = 0.0;
int adaptiveSampling(float in, float* out, struct args* args){

  if (sample_rate_delta == -1)
    sample_rate_delta = args->poll_rate/5;

  if (lastSample == 0.0){
    lastSample = in;
    *out = in;
    return 1;
  }

  float delta = fabs(in / lastSample);
  if (delta >= SAMPLE_DELTA_INC || delta <= SAMPLE_DELTA_DEC){
    lastSample = in;
    *out = in;

    if(args->poll_rate > sample_rate_delta)
      args->poll_rate -= sample_rate_delta;
    
    return 1;
  }

  args->poll_rate += sample_rate_delta;
  return 0;
}
