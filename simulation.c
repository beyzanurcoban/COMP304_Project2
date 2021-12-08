#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/time.h>
#include "pthread_sleep.c"

#define SIZE 1024


// Global Variables
int sim_time;
double p;
long start_time;
long current_time = 0;
long finish_time;
struct timeval get_time;
int vehicle_count = 0;
int front = -1;
int rear = -1;

// Mutex, Conditional Variables, Semaphores, Locks
pthread_mutex_t vehicle_count_mutex;
pthread_mutex_t cross_mutex; // allows only one vehicle to pass the intersection

// Function Declarations
int program_init(int argc, char *argv[]);
int is_occur(double p);
int is_finished();
void *vehicle_func();
void *police_func();

int program_init(int argc, char *argv[]) {
	/* Taking input arguments from the user:
         * -s $(total simulation time) -p $(probability)
         */

        if(argc != 5) {
                perror("Not valid number of argument to run the program\n");
                return -1;
        }

        for(int i=1; i<argc; i+=2) {
                if(strcmp("-s", argv[i]) == 0) {
                        sim_time = atoi(argv[i+1]);
                } else if (strcmp("-p",argv[i]) == 0) {
                        p = strtod(argv[i+1], NULL);
                } else {
                        perror("You specified an undefined flag!\n");
                        return -1;
                }
        }

        printf("sim_time: %d\np: %f\n", sim_time, p);
	
	// Get the current time in seconds, decide start time and calculate finish time
        gettimeofday(&get_time, NULL);
        start_time = get_time.tv_sec;
	finish_time = start_time + sim_time;

        printf("start time in secs: %ld\n", start_time);
	printf("finish time in secs: %ld\n\n", finish_time);

	// Mutex initialization
	pthread_mutex_init(&vehicle_count_mutex, NULL);
	pthread_mutex_init(&cross_mutex, NULL);
}


int main(int argc, char *argv[]) {

	program_init(argc, argv);
	srand(7); // initialize random number generator seed

	/* Creating vehicle threads */
	pthread_t vehicles[SIZE];
	pthread_t police;

	while(!is_finished()) {
		gettimeofday(&get_time, NULL);
        	current_time = get_time.tv_sec;
        	printf("current time now: %ld\n", current_time);

		if(is_occur(p)) {
			pthread_create(&vehicles[vehicle_count], NULL, vehicle_func, NULL);
		}

		pthread_create(&police, NULL, police_func, NULL);

		sleep(1);
	}
	
	/* Join the worker threads to the master thread */
	for (int j=0; j<=vehicle_count; j++) {
                pthread_join(vehicles[j], NULL);
        }

	pthread_join(police, NULL);
	
	/* Destroy mutex */
	pthread_mutex_destroy(&vehicle_count_mutex);
	pthread_mutex_destroy(&cross_mutex);

}

/* Method to decide whether a vehicle is created or not based on the probability p  */
int is_occur(double p) {
	double rnd_num = (double) rand()/RAND_MAX; //produce random number between 0 and 1

	if(rnd_num <= p) {
		return 1;
	} else {
		return 0;
	}	

}

/* Method to decide whether the simulation is finished based on the given simulation time (finished time) */
int is_finished() {

        if (current_time >= finish_time-1) {
                return 1;
        } else {
                return 0;
        }
}

/* Vehicle thread function. */
void *vehicle_func() {
	
	pthread_mutex_lock(&vehicle_count_mutex);
	printf("This is vehicle %d.\n", vehicle_count);
	vehicle_count++;
	printf("Vehicle count after vehicle thread: %d\n", vehicle_count);
	pthread_mutex_unlock(&vehicle_count_mutex);
}

void *police_func() {
	pthread_mutex_lock(&vehicle_count_mutex);
	if (vehicle_count > 0) {
		vehicle_count--;
	}
	printf("Vehicle count after police thread: %d\n", vehicle_count);
	pthread_mutex_unlock(&vehicle_count_mutex);
}

