#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define PATH_TO_DATA "../../host/datasets/cpu-splitted/part_1.csv"


//TMP
#define ERROR_THRESHOLD 0.001

#define ROW_BUFFER_SIZE 100

#define SLIDING_WINDOW_SIZE 50

#define CSV_FILE_SIZE 1000

//Linear values:
float m = 0.0;
float k = 0.0;

// Current k and m on sink
float sink_m = 0.0;
float sink_k = 0.0;

float* m_ptr = &m;
float* k_ptr = &k;

float predicted_sink_value = 0.0;
float predicted_client_value = 0.0;

float local_sink = 0.0;

void sink(float prediction, int timeStamp) {
    local_sink = prediction;
    printf("Sink received predicted data: %f\n", local_sink);
    printf("Timestamp: %d\n", timeStamp);
}

void calculateLinearApproximation(float times[], float values[], int length) {
  float sum_x = 0;
  float sum_y = 0; 
  float sum_xy = 0;
  float sum_x_squared = 0;

  for(int i = 0; i < length; i++) {
    sum_x += times[i];
    sum_y += values[i];
    sum_xy += times[i] * values[i];
    sum_x_squared += times[i]*times[i];
  }

  if((length * sum_x_squared - (sum_x*sum_x)) != 0) {
    k = ((length*sum_xy) - (sum_x*sum_y)) / (length*sum_x_squared - (sum_x*sum_x));
  } else {
    k = 0.0;
  }

  m = (sum_y - (k*sum_x)) / length;
}

int main(int argc, char** argv) {

    char* data;

    FILE *fp;

    char row[ROW_BUFFER_SIZE];

    fp = fopen("values.csv", "r");

    int count = 0;

    int firstRow = 0;

    //send(sockfd, data, 100, 0);
    float values[SLIDING_WINDOW_SIZE];
    float timestamps[SLIDING_WINDOW_SIZE];

    while(feof(fp) != 1 && count < CSV_FILE_SIZE) {        
        //Retrieve next data set: (Query Sensors)
        fgets(row, ROW_BUFFER_SIZE, fp);

        //Init stuff (Ignore first line (VALUES, TIMESTAMP))
        if(firstRow == 0) {
            data = strtok(row, ",");
            data = strtok(NULL, "\n");
            firstRow++;
            printf("-----------------------\n");
            continue;
        }

        //First SLIDING_WINDOW_SIZE data points are init, will be sent to sink too
        if(count < SLIDING_WINDOW_SIZE) {
            data = strtok(row, ",");
            //printf("Data before atof (Timestamp) %s\n", data);
            timestamps[count] = strtof(data, NULL);
            //printf("Timestamp: %f\n", timestamps[count]);
            data = strtok(NULL, "\n");
            values[count] = strtof(data, NULL);
            count++;
            continue;
        }

        //Estimate new state (client side)
        printf("\nRow: %s", row);
        float current_timestamp = 0.0;
        float current_value = 0.0;

        if (sscanf(row, "%f,%f", &current_timestamp, &current_value) == 2) {
            printf("current_timestamp = %.6f\n", current_timestamp);
            printf("current_value = %.6f\n", current_value);
        } else {
            printf("Failed to parse the input string.\n");
        }



        //Update window (Quite messy but it works)
        for(int i = 0; i < SLIDING_WINDOW_SIZE-1; i++) {
            timestamps[i] = timestamps[i+1];
            values[i] = values[i+1];
        }

        timestamps[SLIDING_WINDOW_SIZE] = current_timestamp;
        values[SLIDING_WINDOW_SIZE] = current_value;

        calculateLinearApproximation(timestamps, values, SLIDING_WINDOW_SIZE);
            
        float client_predict_m = *m_ptr;
        float client_predict_k = *k_ptr;

        // Estimate next state (sink side)
        predicted_sink_value = sink_k * current_timestamp + sink_m;

        // Estimate next state (sink side)
        predicted_client_value = client_predict_k * current_timestamp + client_predict_m;

        //If error threshold met
        if(abs(predicted_sink_value - predicted_client_value) > ERROR_THRESHOLD || predicted_sink_value == 0.0) {
            printf("Error: %d\n", abs(predicted_sink_value - predicted_client_value));
            printf("Client predict: %f\n", predicted_client_value);
            printf("Sink   predict: %f\n", predicted_sink_value);
            //printf("Count: %d\n", count);

            printf("Sending new K=%d and M=%d\n", client_predict_k, client_predict_m);
            printf("--------------------------\n");

            sink_k = client_predict_m;
            sink_m = client_predict_k;

        }

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

    printf("m final: %f\n ", *m_ptr);
    printf("k final: %f\n ", *k_ptr);

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