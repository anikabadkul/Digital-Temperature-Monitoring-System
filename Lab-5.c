#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <time.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 10
#define AIN_DEV "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"

struct sensor_type {
    struct timespec timestamp;
    double celcius_temperature;
};

struct sensor_type buffer[BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int buffer_index = 0;

// Function to convert Celsius to Fahrenheit
double CtoF(double c) {
    return (c * 9.0 / 5.0) + 32.0;
}

// Function to extract temperature from TMP36 data
double temperature(int value) {
    // Convert the digital value to millivolts.
    double millivolts = (value / 4096.0) * 1800;
    // Convert the millivolts to Celsius temperature.
    // Celcius conversion from the TMP36 Datasheet
    double temperature = (millivolts - 500.0) / 10.0;
    return temperature;
}

void *inputThread(void *arg) {
    int gpio_number = 8; // P8_8 gpio pin

    // Configure interrupt settings using epoll
    char InterruptPath[40];
    sprintf(InterruptPath, "/sys/class/gpio/gpio%d/value", gpio_number);

    int epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = open(InterruptPath, O_RDONLY | O_NONBLOCK);
    if (ev.data.fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    struct timespec tm;

    while (buffer_index < BUFFER_SIZE) {
        int num_events = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].events & EPOLLIN) {
                clock_gettime(CLOCK_MONOTONIC, &tm);
                buffer[buffer_index].timestamp = tm;

                FILE *f = fopen(AIN_DEV, "r");
                if (f == NULL) {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }
                int a2dReading;
                fscanf(f, "%d", &a2dReading);
                fclose(f);

                buffer[buffer_index].celcius_temperature = temperature(a2dReading);

                pthread_mutex_lock(&mutex);
                buffer_index++;
                pthread_mutex_unlock(&mutex);
            }
        }
    }

    close(ev.data.fd);
    close(epfd);
    return NULL;
}

void *outputThread(void *arg) {
    FILE *file = fopen("Anika_Badkul_sensordata.txt", "a");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    
    while (1) {
        pthread_mutex_lock(&mutex);
        if (buffer_index > 0) {
            struct sensor_type data = buffer[buffer_index - 1];
            long long int time_stamp = data.timestamp.tv_sec * 1000000000LL + data.timestamp.tv_nsec; //tine in nanoseconds
            // Print timestamp and tempm to txt file
            fprintf(file, "Timestamp:%lld Temp:%f\n", time_stamp, data.celcius_temperature);
            // display in terminal 
            printf("Timestamp:%lld Temp:%f\n", time_stamp, data.celcius_temperature);
            buffer_index--;
        }
        pthread_mutex_unlock(&mutex);
        if (buffer_index == 0)
            break; // Break the loop once all data points are printed and stored
    }

    fclose(file);
    return NULL;
}

int main() {
    pthread_t input_thread, output_thread;
    int i = 0;

    while (i < 5) {
        //buffer_index = 0;
        for (int j = 0; j < 10; j++) {
            if (pthread_create(&input_thread, NULL, inputThread, NULL) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
            usleep(100000); 
        }

        if (pthread_create(&output_thread, NULL, outputThread, NULL) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        pthread_join(output_thread, NULL);

        i++;
    }

    return 0;
}

