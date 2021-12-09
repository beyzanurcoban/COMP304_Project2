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

#define SIZE 4
#define MAX 256


// Global Variables
int sim_time;
double p;
long start_time;
long current_time = 0;
long finish_time;
struct timeval get_time;
int vehicle_count = 0;
int W_count = 1;
int W_queue[MAX];
int W_front = 0;
int W_rear = 0;

// Mutex, Conditional Variables, Semaphores, Locks
pthread_mutex_t vehicle_count_mutex;
pthread_mutex_t W_count_mutex;
pthread_mutex_t cross_mutex; // allows only one vehicle to pass the intersection
pthread_mutex_t queue_mutex;
pthread_mutex_t coming;
pthread_mutex_t leaving;
pthread_cond_t vTurn_cv;
pthread_cond_t pTurn_cv;

// Function Declarations
int program_init(int argc, char *argv[]);
int is_occur(double p);
int is_finished();
void *W_func(void* arg);
void *police_func(void* arg);
int enqueue(int data, int front, int rear, int queue[]);
int dequeue(int front, int rear, int queue[]);

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
	pthread_mutex_init(&W_count_mutex, NULL);
	pthread_mutex_init(&cross_mutex, NULL);
	pthread_mutex_init(&queue_mutex, NULL);
	pthread_cond_init(&vTurn_cv, NULL);
	pthread_cond_init(&pTurn_cv, NULL);
	pthread_mutex_init(&coming, NULL);
        pthread_mutex_init(&leaving, NULL);
}


int main(int argc, char *argv[]) {

	program_init(argc, argv);
	srand(7); // initialize random number generator seed

	W_queue[0] = 0;

	/* Creating vehicle threads */
	pthread_t lane_threads[SIZE];
	pthread_t police;

	pthread_create(&police, NULL, police_func, NULL);
	pthread_create(&lane_threads[0], NULL, W_func, NULL);
	//pthread_create(&police, NULL, police_func, NULL);

	while(!is_finished()) {
		gettimeofday(&get_time, NULL);
        	current_time = get_time.tv_sec;
        	printf("current time now: %ld\n", current_time);

		pthread_sleep(1);
	}
	
	/* Join the worker threads to the master thread */
	for (int j=0; j<1; j++) {
                pthread_join(lane_threads[j], NULL);
        }

	pthread_join(police, NULL);
	
	/* Destroy mutex */
	pthread_mutex_destroy(&vehicle_count_mutex);
	pthread_mutex_destroy(&W_count_mutex);
	pthread_mutex_destroy(&cross_mutex);
	pthread_mutex_destroy(&queue_mutex);
	pthread_cond_destroy(&vTurn_cv);
        pthread_cond_destroy(&pTurn_cv);
	pthread_mutex_destroy(&coming);
        pthread_mutex_destroy(&leaving);

	return 0;

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

/* West lane thread function */
void *W_func(void* arg) {
	while(!is_finished()) {
		pthread_mutex_lock(&coming);
		pthread_cond_wait(&pTurn_cv, &coming);
		pthread_mutex_unlock(&coming);
		printf("West func\n");
		pthread_cond_signal(&vTurn_cv);
	}

	//pthread_exit(0);
}


/* Pollice officer thread function */
void *police_func(void* arg) {
	while(!is_finished()) {
		pthread_cond_signal(&pTurn_cv);
		pthread_sleep(0.01);
		pthread_mutex_lock(&leaving);
                pthread_cond_wait(&vTurn_cv, &leaving);
		pthread_mutex_unlock(&leaving);
                printf("Police func\n");
        }
	//pthread_exit(0);


}

/* Queue Operations */
int enqueue(int data, int front, int rear, int queue[]) {
	if(rear == MAX -1) {
		perror("Queue is full!\n");
		return -1;
	} else {
		if (front == -1 && rear == -1){
			front = 0;
			rear = 0;
		} else {
			rear++;
		}

		queue[rear] = data;

	}

	return rear;
}

int dequeue(int front, int rear, int queue[]) {
	int data;

	if (front == -1) {
		perror("Queue is empty!\n");
		return -1;
	} else {
		data = queue[front];

		if (front == 0 && rear == 0) {
			front = -1;
			rear = -1;
		} else {
			front++;
		}
	}

	return data;
}
