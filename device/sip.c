#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#include "main.h"

#define ROW_BUFFER_SIZE 100

//#define ID_FILE "/home/pi/iot-monitoring/data/id.txt"
#define ID_FILE "id.txt"

char* PATH_TO_DATA;          
char* address;                        // 127.0.0.1
char* PORT;                           // 12001
char* FREQUENCY_str;                  // 10
char* DURATION_str;                   // 60 seconds
char* SLIDING_WINDOW_SIZE_str;        // 20 samples
char* ERROR_THRESHOLD_str;            // 2 epsilon
char* EMWA_ALPHA_str;                 // 0.25

int FREQUENCY;
int DURATION;
int SLIDING_WINDOW_SIZE;
int ERROR_THRESHOLD;
double EMWA_ALPHA;

//Linear values:
long double m = 0.0;
long double k = 0.0;

// Current k and m on sink
long double sink_m = 0.0;
long double sink_k = 0.0;

long double* m_ptr = &m;
long double* k_ptr = &k;

double raw_value; 
long double emwa_m = 0.0;

double predicted_sink_value = 0.0;
double predicted_client_value = 0.0;

double local_sink = 0.0;

struct timeval tv;

double iteration_time;

char buffer[2048];

char device_id[10];

char* address;

char* message;

char* data;
FILE *fp;
FILE *fp_id;

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

void calculateLinearApproximation(unsigned long times[], double values[], int length) {
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

    PATH_TO_DATA         = argv[1];          
    address              = argv[2];               // 127.0.0.1
    PORT                 = argv[3];               // 12001
    FREQUENCY_str            = argv[4];               // 10
    DURATION_str             = argv[5];               // 60 seconds
    SLIDING_WINDOW_SIZE_str  = argv[6];               // 20 samples
    ERROR_THRESHOLD_str      = argv[7];               // 2 epsilon
    EMWA_ALPHA_str           = argv[8];               // 0.25

    FREQUENCY           = atoi(FREQUENCY_str);
    DURATION            = atoi(DURATION_str);
    SLIDING_WINDOW_SIZE = atoi(SLIDING_WINDOW_SIZE_str);
    ERROR_THRESHOLD     = atoi(ERROR_THRESHOLD_str);

    EMWA_ALPHA       =  atof(EMWA_ALPHA_str);

    iteration_time = 1.0 / FREQUENCY;

    char* row = (char*)malloc(ROW_BUFFER_SIZE * sizeof(char));

    fp = fopen(PATH_TO_DATA, "r");

    int count = 0;

    int firstRow = 0;

    // get device_id from file named id.txt
    /*
    fp_id = fopen(ID_FILE, "r");
    fgets(device_id, 10, fp_id);
    fclose(fp_id);  
    */

    //THIS IS DUMB. If EMWA_ALPHA is 1, then the alg_id is 8 else 7 
    char *FILTER_ENABLE = "1";

    if (strcmp(EMWA_ALPHA_str, "1") == 0 || strcmp(EMWA_ALPHA_str, "1.0") == 0) {
        *FILTER_ENABLE = "0";
    }

    //send(sockfd, data, 100, 0);
    double values[SLIDING_WINDOW_SIZE];
    unsigned long timestamps[SLIDING_WINDOW_SIZE];

    // poihter to first element in values
    double* values_ptr      = &values[0];
    unsigned long* timestamps_ptr  = &timestamps[0];

    gettimeofday(&tv, NULL);
    unsigned long startTime = tv.tv_usec;

    while(feof(fp) != 1 && DURATION*100000 > startTime - tv.tv_usec) {
        printf("Enter loop\n");
        //Retrieve next data set: (Query Sensors)
        fgets(row, ROW_BUFFER_SIZE, fp);
        printf("Row: %s ---------------------------------------------", row);

        printf("still alive\n");

        //First SLIDING_WINDOW_SIZE data points are init, will be sent to sink too
        if(count < SLIDING_WINDOW_SIZE) {
            //printf("Data before atof (Timestamp) %s\n", data);
            printf("VAD HÄNDER---------------------");
            timestamps[count] = 0;
            //printf("Timestamp: %f\n", timestamps[count]);
            printf("PEEKABOO-----------");
            data = strtok(row, "\n");
            printf("data:----- %s\n", data);
            if (data == NULL) {
                // Handle the case where strtok returns NULL
                continue; // Skip the rest of this iteration
            }
            raw_value = atof(data);
            emwa_m = EMWA_ALPHA * raw_value + (1 - EMWA_ALPHA) * emwa_m;
            values[count] = emwa_m;
            count++;
            gettimeofday(&tv, NULL);
            continue;
        }
        //Ignore measuring in beginning as that would highly skew time as first ones are not sent (Look above)

        //Estimate new state (client side)
        //printf("\nRow: %s", row);
        unsigned long current_timestamp = 0;
        double current_value = 0.0;

        // we want the current timestamp, not the one the csv, current_timestamp = atof(row);
        gettimeofday(&tv, NULL);
        current_timestamp = tv.tv_usec;

        //clock_gettime(CLOCK_REALTIME, start);

        //printf("Second Row: %s\n", row);
        data = strtok(row, "\n");
        //printf("Third row: %s\n", data);

        raw_value = atof(data);
        emwa_m = EMWA_ALPHA * raw_value + (1 - EMWA_ALPHA) * emwa_m;
        current_value = emwa_m;

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

        // inc pointer, if pointer is at SLIDING_WINDOW_SIZE, reset to 0
        if(values_ptr == &values[SLIDING_WINDOW_SIZE-1]) {
            values_ptr      = &values[0];
            timestamps_ptr  = &timestamps_ptr[0];
        } else {
            values_ptr      += sizeof(double);
            timestamps_ptr  += sizeof(unsigned long);
        }

        // write the value in current_timestamp to the address of timestamp_ptr
        *timestamps_ptr = current_timestamp;
        *values_ptr = current_value;

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
            //printf("Count: %d\n", count);

            // printf("Sending new K=%f and M=%f\n", client_predict_k, client_predict_m);
            // printf("\n");

            sprintf(buffer, "%s,%lu,%.15Lf,%.15Lf,%s", device_id, current_timestamp, client_predict_k, client_predict_m, FILTER_ENABLE);

            message = buffer;

            sendMessage(address, message);
            

            sink_k = client_predict_k;
            sink_m = client_predict_m;

        }

        usleep(iteration_time * 1000000);  // Sleep in microsecond
        count++; 
        gettimeofday(&tv, NULL);

    }
    free(row);
    fclose(fp);
  
    count = 0;

    //Here begins the genral code: 
    //Estimate new state

    printf("End of program...\n");
    exit(0);
  }