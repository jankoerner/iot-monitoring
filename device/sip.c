#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include "main.h"

#define PATH_TO_DATA "../host/datasets/cpu-splitted/part_22.csv"

//TMP
#define FREQUENCY 12.549123873

#define ERROR_THRESHOLD 8.19

#define ROW_BUFFER_SIZE 100

#define SLIDING_WINDOW_SIZE 10

#define CSV_FILE_SIZE 36000

#define ADDRESS '127.0.0.1'

#define PORT 12001

//Linear values:
long double m = 0.0;
long double k = 0.0;

// Current k and m on sink
long double sink_m = 0.0;
long double sink_k = 0.0;

long double* m_ptr = &m;
long double* k_ptr = &k;

double predicted_sink_value = 0.0;
double predicted_client_value = 0.0;

double local_sink = 0.0;

struct timeval tv;

double iteration_time = 1.0 / FREQUENCY;

void sink(double prediction, int timeStamp) {
    local_sink = prediction;
    printf("Sink received predicted data: %f\n", local_sink);
    printf("Timestamp: %d\n", timeStamp);
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
    // printf("Socket successfully created..\n");
  }

  // connect the client socket to server socket
  if (connect(sockfd,(const struct sockaddr_in*)&servaddr, sizeof(servaddr)) != 0) {
    printf("connection with the server failed...\n");
    exit(0);
  }
  else
    // printf("Messsage to send: %s\n", message);
    send(sockfd, message, strlen(message), 0);

  close(sockfd);
  return 0;
}

void calculateLinearApproximation(double times[], double values[], int length) {
    double sum_x = 0.0;
    double sum_y = 0.0; 
    double sum_xy = 0.0;
    double sum_x_squared = 0.0;

    for(int i = 0; i < length; i++) {
        sum_x += times[i];
        sum_y += values[i];
        sum_xy += times[i] * values[i];
        sum_x_squared += times[i]*times[i];
    }

    if((length * sum_x_squared - (sum_x*sum_x)) != 0.0) {

        long double eq_a = length*sum_xy;
        long double eq_b = sum_x*sum_y;
        long double eq_c = length*sum_x_squared;
        long double eq_d = sum_x*sum_x;

        long double eq_part1 = eq_a - eq_b;
        long double eq_part2 = eq_c - eq_d;
        k = eq_part1 / eq_part2;

    } else {
        k = 0.0;
    }

    m = (sum_y - (k*sum_x)) / length;

}

int main(int argc, char** argv) {

    char* data;

    FILE *fp;

    char row[ROW_BUFFER_SIZE];

    fp = fopen(PATH_TO_DATA, "r");

    int count = 0;

    int firstRow = 0;

    //send(sockfd, data, 100, 0);
    double values[SLIDING_WINDOW_SIZE];
    double timestamps[SLIDING_WINDOW_SIZE];
    
    while(feof(fp) != 1 && count < CSV_FILE_SIZE) {        
        //Retrieve next data set: (Query Sensors)
        fgets(row, ROW_BUFFER_SIZE, fp);
        //Init stuff (Ignore first line (VALUES, TIMESTAMP))
        if(firstRow == 0) {
            data = strtok(row, "\n");
            firstRow++;
            // printf("-----------------------\n");
            continue;
        }

        //First SLIDING_WINDOW_SIZE data points are init, will be sent to sink too
        if(count < SLIDING_WINDOW_SIZE) {
            //printf("Data before atof (Timestamp) %s\n", data);
            timestamps[count] = 0.0;
            //printf("Timestamp: %f\n", timestamps[count]);
            data = strtok(row, "\n");
            values[count] = strtof(data, NULL);
            count++;
            continue;
        }

        //Estimate new state (client side)
        //printf("\nRow: %s", row);
        double current_timestamp = 0.0;
        double current_value = 0.0;

        // we want the current timestamp, not the one the csv, current_timestamp = atof(row);
        gettimeofday(&tv, NULL);
        current_timestamp = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;

        //printf("Second Row: %s\n", row);
        data = strtok(row, "\n");
        //printf("Third row: %s\n", data);
        current_value = atof(data);
        //printf("\n");

        //printf("Current timestamp %f\n", current_timestamp);
        //printf("Current value %f\n", current_value);
        //printf("\n");

        /*
        if (sscanf(row, "%f,%f", &current_timestamp, &current_value) == 2) {
            printf("current_timestamp = %.6f\n", current_timestamp);
            printf("current_value = %.6f\n", current_value);
        } else {
            printf("Failed to parse the input string.\n");
        }
        */

        //Update window (Quite messy but it works)
        for(int i = 0; i < SLIDING_WINDOW_SIZE-2; i++) {
            timestamps[i] = timestamps[i+1];
            values[i] = values[i+1];
        }

        //printf("HELLO correct value %f\n", current_value);
        timestamps[SLIDING_WINDOW_SIZE-1] = current_timestamp;
        values[SLIDING_WINDOW_SIZE-1] = current_value;

        calculateLinearApproximation(timestamps, values, SLIDING_WINDOW_SIZE);
            
        long double client_predict_m = *m_ptr;
        long double client_predict_k = *k_ptr;

        // Estimate next state (sink side)
        predicted_sink_value = sink_k * current_timestamp + sink_m;

        //printf("Predicted sink value %f where k=%Lf and m=%Lf\n", predicted_sink_value, sink_k, sink_m);

        // Estimate next state (sink side)
        predicted_client_value = client_predict_k * current_timestamp + client_predict_m;

        //printf("Predicted client value %f where k=%Lf and m=%Lf\n", predicted_client_value, client_predict_k, client_predict_m);

        //printf("Error value: %Lf\n", fabsl(predicted_sink_value - predicted_client_value));
        //If error threshold met
        if(fabsl(predicted_sink_value - predicted_client_value) > ERROR_THRESHOLD || predicted_sink_value == 0.0) {
            //printf("Error: %f\n", fabsl(predicted_sink_value - predicted_client_value));
            //printf("Client predict: %f\n", predicted_client_value);
            //printf("Sink   predict: %f\n", predicted_sink_value);
            printf("Count: %d\n", count);

            // printf("Sending new K=%f and M=%f\n", client_predict_k, client_predict_m);
            // printf("\n");

            char buffer[2048];

            char* address = "127.0.0.1";

            sprintf(buffer, "0,%f,%.15Lf,%.15Lf", current_timestamp, client_predict_k, client_predict_m);

            char* message = buffer;


            sendMessage(address, message);

            sink_k = client_predict_k;
            sink_m = client_predict_m;

        }

        usleep(iteration_time * 1000000);  // Sleep in microsecond
        count++; 
    }

    /*
    //Initial 100 data points (values)
    while(feof(fp1) != 1 && count < 100) {
      fgets(row, 10, fp1);
      data = strtok(row, ",");
      values[count] = atof(data);
      count++;
    }
    count = 0;

    //Initial 100 data points (timestamps)
    while(feof(fp2) != 1 && count < 100) {
      fgets(row, 10, fp2);
      data = strtok(row, ",");
      timestamps[count] = atof(data);
      count++;
    }
    */
  
    count = 0;

    printf("m final: %.32Lf\n ", *m_ptr);
    printf("k final: %.32Lf\n ", *k_ptr);

    //Here begins the genral code: 
    //Estimate new state



    /*
    while(feof(fp) != 1 && count < 1000) {
      fgets(row, 10, fp);
      data = strtok(row, ",");
      



      count++;
    }
    */

    printf("End of program...\n");
    exit(0);
  }