#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include "main.h"

long long t0 = -1;
double* working_x;
double* working_y;
int n = 0;
struct piece pw;
int pla(struct time_val* tv_in, struct time_val* tv_out){
  n += 1;
  if (t0 == -1)
    t0 = tv_in->time;
  if (n == 1){
    working_x = malloc(sizeof(double));
    working_y = malloc(sizeof(double));
    working_x[0] = ((double)tv_in->time - t0)/1000000;
    working_y[0] = tv_in->value;

    return 0;
  }
  double slope = 0.0;
  double offset = 0.0;
  double error = 0.0;

  double* temp_x = malloc(sizeof(double)*n);
  double* temp_y = malloc(sizeof(double)*n);
  int i;
  for(i = 0; i < n-1; i++){
    temp_x[i] = working_x[i];
    temp_y[i] = working_y[i];
  }
  temp_x[n-1] = ((double)tv_in->time - t0)/1000000;
  temp_y[n-1] = tv_in->value;
  free(working_x);
  free(working_y);
  working_x = temp_x;
  working_y = temp_y;

  if (linreg(n, working_x, working_y, &slope, &offset, &error)){
    return 0;
  }

  if(error * error < .95){
    
    tv_out[0].time = ((long long)(working_x[0] * 1000000) + t0);
    tv_out[1].time = ((long long)(working_x[n-2] * 1000000) + t0);

    tv_out[0].value = working_x[0] * pw.slope + pw.offset;
    tv_out[1].value = working_x[n-2] * pw.slope + pw.offset;

    free(working_x);
    free(working_y);
    working_x = malloc(sizeof(double));
    working_y = malloc(sizeof(double));
    working_x[0] = ((double)tv_in->time - t0)/1000000;
    working_y[0] = tv_in->value;
    n = 1;

    return 1;
  }
  pw.slope = slope;
  pw.offset = offset;
  return 0;
}

int linreg(int n, const double x[], const double y[], double* slope, double* offset, double* error){
  double   sumx = 0.0;                      /* sum of x     */
  double   sumx2 = 0.0;                     /* sum of x**2  */
  double   sumxy = 0.0;                     /* sum of x * y */
  double   sumy = 0.0;                      /* sum of y     */
  double   sumy2 = 0.0;                     /* sum of y**2  */

  for (int i=0;i<n;i++){
    sumx  += x[i];
    sumx2 += (x[i] * x[i]);
    sumxy += x[i] * y[i];
    sumy  += y[i];
    sumy2 += (y[i] * y[i]);
  }

  // Can't solve singular matrix
  double denom = (n * sumx2 - (sumx * sumx));
  if (denom == 0) {
    *slope = 0;
    *offset = 0;
    if (error) *error = 0;
    return 1;
  }

  /* printf("s: (%f * %f - %f * %f) / %f\n", sumy, sumx2, sumx, sumxy, denom); */
  *slope = (n * sumxy  -  sumx * sumy) / denom;
  *offset = (sumy * sumx2  -  sumx * sumxy) / denom;
  if (error!=NULL) {
    *error = (sumxy - sumx * sumy / n) /
      sqrt((sumx2 - (sumx * sumx)/n) *
           (sumy2 - (sumy * sumy)/n));
  }

  return 0;
}
