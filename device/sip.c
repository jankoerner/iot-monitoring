#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <string.h>

#include "main.h"

#define ROW_BUFFER_SIZE 100

//#define ID_FILE "/home/pi/iot-monitoring/data/id.txt"
#define ID_FILE "id.txt"

char* PATH_TO_DATA;          
char* address;                        // 127.0.0.1
int PORT;                             // 12001
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
double m = 0.0;
double k = 0.0;

// Current k and m on sink
double sink_m = 0.0;
double sink_k = 0.0;

double* m_ptr = &m;
double* k_ptr = &k;

double raw_value; 
double emwa_m = 0.0;
double current_value = 0.0;

double predicted_sink_value = 0.0;
double predicted_client_value = 0.0;

double local_sink = 0.0;

double iteration_time;

double today;

char buffer[2048];

int device_id;

char* address;

char* message;

int window_index = 0;

char* data;
FILE *fp;
FILE *fp_id;

void sink(double prediction, int timeStamp) {
    local_sink = prediction;
    //printf("Sink received predicted data: %lf\n", local_sink);
    //printf("Timestamp: %d\n", timeStamp);
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
  else{
    //printf("Messsage to send: %s\n", message);
    send(sockfd, message, strlen(message), 0);
  }

  close(sockfd);
  return 0;
}

void calculateLinearApproximation(double x[], double y[], int n) {

  double sumx = 0.0;
  double sumx2 = 0.0;
  double sumxy = 0.0;
  double sumy = 0.0;
  double sumy2  = 0.0;
  for (int i=0;i<n;i++){
    sumx  += x[i];
    sumx2 += (x[i] * x[i]);
    sumxy += x[i] * y[i];
    sumy  += y[i];
    sumy2 += (y[i] * y[i]);
  }

  // Can't solve singular matrix
  double denom = (n * sumx2 - (sumx * sumx));

  //printf("s: (%f * %f - %f * %f) / %f\n", sumy/n, sumx2, sumx/n, sumxy, denom);
  //printf("Today: %lf\n", today);
  //printf("Average Value: %f\n", sumy/n);
  //printf("Average timestamp: %f\n", sumx/n);
  k = (n * sumxy  -  sumx * sumy) / denom;
  m = (sumy * sumx2  -  sumx * sumxy) / denom;
}

int main(int argc, char** argv) {

    fflush(stdout);

    PATH_TO_DATA         = argv[1];          
    address              = argv[2];               // 127.0.0.1
    PORT                 = atoi(argv[3]);               // 12001
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

    char row[ROW_BUFFER_SIZE];

    fp = fopen(PATH_TO_DATA, "r");

    int count = 0;

    // get todays date at 00:00:00.000000 in unix 
    // -----------------------------------------------
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    t = mktime(&tm);


    // to double with todays date at 00:00:00.000000 in unix time
    today = (double)t + 3600;  

    fp_id = fopen(ID_FILE, "r");
    char* idline = NULL;
    size_t idlen = 0;
    if(getline(&idline, &idlen, fp_id)){
        device_id = atoi(idline);
        device_id = 0;
    }
    printf("Device id: %d\n", device_id);
    fclose(fp_id);

    //THIS IS DUMB. If EMWA_ALPHA is 1, then the alg_id is 8 else 7 
    char *FILTER_ENABLE = "1";

    if (strcmp(EMWA_ALPHA_str, "1") == 0 || strcmp(EMWA_ALPHA_str, "1.0") == 0) {
        *FILTER_ENABLE = "0";
    }

    //send(sockfd, data, 100, 0);
    double values[SLIDING_WINDOW_SIZE];
    double timestamps[SLIDING_WINDOW_SIZE];

    // poihter to first element in values
    window_index = 0;

    struct timespec* tv = malloc(sizeof(struct timespec));

    clock_gettime(CLOCK_REALTIME, tv);
    __time_t startTime = tv->tv_sec;


    while(feof(fp) != 1 && DURATION > tv->tv_sec - startTime) {
        if(window_index >= SLIDING_WINDOW_SIZE) {
            window_index = 0;
        }

        fgets(row, ROW_BUFFER_SIZE, fp);

        if(count < SLIDING_WINDOW_SIZE) {
            timestamps[window_index] = 0;

            data = strtok(row, "\n");
            if (data == NULL) {
                // Handle the case where strtok returns NULL
                count++;
                continue; // Skip the rest of this iteration
            }

            raw_value = atof(data);
            emwa_m = EMWA_ALPHA * raw_value + (1 - EMWA_ALPHA) * emwa_m;
            //printf("Window index value: %d\n", window_index);
            values[window_index] = emwa_m;
            count++;
            clock_gettime(CLOCK_REALTIME, tv);
            window_index++;
            continue;
        }
        //Ignore measuring in beginning as that would highly skew time as first ones are not sent (Look above)

        //Estimate new state (client side)
        double current_timestamp = 0.0;
        current_value = 0.0;

        clock_gettime(CLOCK_REALTIME, tv);

        double usec = tv->tv_nsec/1000;

        current_timestamp = tv->tv_sec + (usec / 1000000);

        data = strtok(row, "\n");
        //printf("Data: %s\n", data);

        raw_value = atof(data);
        emwa_m = EMWA_ALPHA * raw_value + (1 - EMWA_ALPHA) * emwa_m;
        current_value = emwa_m;


        // write the value in current_timestamp
        values[window_index] = current_value ;
        timestamps[window_index] = current_timestamp - today;

        calculateLinearApproximation(timestamps, values, SLIDING_WINDOW_SIZE);
        //printf("value: %lf\n", (current_timestamp - today)*k + m);
            
        double client_predict_m = *m_ptr;
        double client_predict_k = *k_ptr;

        // Estimate next state (sink side)
        predicted_sink_value = sink_k * (current_timestamp - today) + sink_m;

        // Estimate next state (sink side)
        predicted_client_value = client_predict_k * (current_timestamp - today) + client_predict_m;

        //If error threshold met
        if(fabsl(predicted_sink_value - predicted_client_value) > ERROR_THRESHOLD || predicted_sink_value == 0.0) {
            sprintf(buffer, "%d,%lf,%lf,%lf,%s", device_id, current_timestamp, client_predict_k, client_predict_m, FILTER_ENABLE);

            message = buffer;

            sendMessage(address, message);
            printf("@%d I sent: %s\n", count, message);
            //printf("The count was: %d\n", count);
            //printf("-----------------------\n");
            

            sink_k = client_predict_k;
            sink_m = client_predict_m;

        }

        fflush(stdout);
        usleep(iteration_time * 1000000);  // Sleep in microsecond
        count++; 
        clock_gettime(CLOCK_REALTIME, tv);

        // fuckit
        
        window_index += 1;
        
    }
    fclose(fp);
  
    free(tv);
    count = 0;

    //Here begins the genral code: 
    //Estimate new state

    printf("End of program...\n");
    exit(0);
  }