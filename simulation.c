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

#define LANES 4
#define MAX 256


// Global Variables
int sim_time;
double p;
long start_time;
long current_time = 0;
long finish_time;
struct timeval get_time;
int lanes_decided = 0;
int id = 0;
int queue[MAX];
int queue_count = 0;
int front = -1;
int rear = -1;

// Mutex, Conditional Variables, Semaphores, Locks
pthread_mutex_t police_checking;
pthread_mutex_t id_mutex;
pthread_cond_t new_turn;
pthread_mutex_t lane_count;
pthread_mutex_t queue_mutex;

// Function Declarations
int program_init(int argc, char *argv[]);
int is_occur(double p);
int is_finished();
void *lane_func();
void *police_func();
int enqueue(int data);
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
	pthread_mutex_init(&police_checking, NULL);
	pthread_mutex_init(&id_mutex, NULL);
	pthread_cond_init(&new_turn, NULL);
	pthread_mutex_init(&lane_count, NULL);
	pthread_mutex_init(&queue_mutex, NULL);

	return 0;
	
}


int main(int argc, char *argv[]) {

	program_init(argc, argv);
	srand(7); // initialize random number generator seed
	queue[0] = 0;
	queue[1] = 1;
	queue[2] = 2;
	queue[3] = 3;
	queue_count = 4;
	front = 0;
	rear = 3;

	/* Creating vehicle threads */
	pthread_t lane_threads[LANES];
	pthread_t police;

	for(int i=0; i<LANES; i++) {
		pthread_create(&lane_threads[i], NULL, lane_func, NULL);
	}
	pthread_create(&police, NULL, police_func, NULL);

	/*while(!is_finished()) {
		gettimeofday(&get_time, NULL);
        current_time = get_time.tv_sec;
        printf("current time now: %ld\n", current_time);

		pthread_sleep(1);
	}*/
	
	/* Join the worker threads to the master thread */
	for (int j=0; j<LANES; j++) {
        pthread_join(lane_threads[j], NULL);
    }

	pthread_join(police, NULL);
	
	/* Destroy mutex */
	pthread_mutex_destroy(&police_checking);
	pthread_mutex_destroy(&id_mutex);
	pthread_cond_destroy(&new_turn);
	pthread_mutex_destroy(&lane_count);
	pthread_mutex_destroy(&queue_mutex);

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

    if(current_time >= finish_time-1) {
        return 1;
    } else {
        return 0;
    }
}

/* Lane thread function */
void *lane_func() {
	pthread_mutex_lock(&id_mutex);
		int l_id = id;
		id++;
	pthread_mutex_unlock(&id_mutex);

	while(!is_finished()){
		pthread_mutex_lock(&police_checking);
			pthread_cond_wait(&new_turn, &police_checking);
		pthread_mutex_unlock(&police_checking);

		pthread_mutex_lock(&queue_mutex);
		printf("This is lane %d function\n", l_id);

		if(is_occur(p)){
			int ind = enqueue(l_id);
			printf("Vehicle from %d is produced and added at %d index\n", l_id, ind);
		}
		pthread_mutex_unlock(&queue_mutex);

		pthread_mutex_lock(&lane_count);
		lanes_decided++;
		pthread_mutex_unlock(&lane_count);
	}
	pthread_exit(0);
}


/* Pollice officer thread function */
void *police_func() {
	while (!is_finished()){

		gettimeofday(&get_time, NULL);
        current_time = get_time.tv_sec;
        printf("current time now: %ld\n", current_time);

		printf("This is police function\n");
		pthread_mutex_lock(&police_checking);
			pthread_cond_broadcast(&new_turn);
		pthread_mutex_unlock(&police_checking);

		while(lanes_decided != LANES);
		pthread_mutex_lock(&lane_count);
		printf("All lanes completed their actions\n");
		lanes_decided = 0;
		pthread_mutex_unlock(&lane_count);

		pthread_sleep(1);
	}
	
	pthread_exit(0);
}

/* Queue Operations */
int enqueue(int data) {
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
	queue_count++;
	return rear;
}

int dequeue(int front, int rear, int queue[]) {
	int data;
	pthread_mutex_lock(&queue_mutex);

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
	queue_count--;
	pthread_mutex_unlock(&queue_mutex);

	return data;
}
