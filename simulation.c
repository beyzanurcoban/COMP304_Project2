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

// There is a unique id for each direction, whenever you see lane 0, it means N lane.
// N -> 0
// E -> 1
// S -> 2
// W -> 3

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
pthread_mutex_t intersection;
pthread_cond_t passing;

// Function Declarations
int program_init(int argc, char *argv[]);
int is_occur(double p);
int is_finished();
void *lane_func();
void *police_func();
int enqueue(int data);
int dequeue();

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
	pthread_mutex_init(&intersection, NULL);
	pthread_cond_init(&passing, NULL);

	return 0;
	
}


int main(int argc, char *argv[]) {

	program_init(argc, argv);
	srand(7); // initialize random number generator seed

	// At the beginnin, there are one vehicle at each lane, queue[0] means there is a car in N lane
	queue[0] = 0;
	queue[1] = 1;
	queue[2] = 2;
	queue[3] = 3;
	queue_count = 4;
	// Queue variables
	front = 0;
	rear = 3;

	/* Creating vehicle threads */
	pthread_t lane_threads[LANES];
	pthread_t police;

	// Thread creation (in total there are 5 threads)
	for(int i=0; i<LANES; i++) {
		pthread_create(&lane_threads[i], NULL, lane_func, NULL);
	}
	pthread_create(&police, NULL, police_func, NULL);
	
	/* Join the worker threads to the master thread */
	for (int j=0; j<LANES; j++) {
        pthread_join(lane_threads[j], NULL);
    }

	pthread_join(police, NULL);

	// for(int i=0; i<queue_count; i++){
	// 	printf("Queue: %d\n", queue[i]);
	// }
	
	/* Destroy mutex */
	pthread_mutex_destroy(&police_checking);
	pthread_mutex_destroy(&id_mutex);
	pthread_cond_destroy(&new_turn);
	pthread_mutex_destroy(&lane_count);
	pthread_mutex_destroy(&queue_mutex);
	pthread_mutex_destroy(&intersection);
	pthread_cond_destroy(&passing);

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
	// Keep the id lock, and gave a unique id for each lane
	pthread_mutex_lock(&id_mutex);
		int l_id = id;
		id++;
	pthread_mutex_unlock(&id_mutex);

	// Simulation
	while(!is_finished()){
		// If there is a police, simulation can start. All lanes are notified.
		pthread_mutex_lock(&police_checking);
			pthread_cond_wait(&new_turn, &police_checking);
		pthread_mutex_unlock(&police_checking);

		// Vehicles started to come for each lane according to their probability.
		// Keep the queue mutex, so there will not be two different operation at the same time on the queue.
		// Police cannot dequeue while the cars are enqueued.
		pthread_mutex_lock(&queue_mutex);
			printf("This is lane %d function\n", l_id);

			// E, S, W is produced with probability p.
			if(l_id != 0){
				if(is_occur(p)){
				int ind = enqueue(l_id);
				printf("Vehicle from %d is produced and added at %d index\n", l_id, ind);
				}
			// N is produces with probability 1-p.
			} else {
				if(1-is_occur(p)){
					int ind = enqueue(l_id);
					printf("Vehicle from %d is produced and added at %d index\n", l_id, ind);
				}
			}
		pthread_mutex_unlock(&queue_mutex);

		pthread_mutex_lock(&lane_count);
			lanes_decided++;
		pthread_mutex_unlock(&lane_count);

		// Dequeue
		// Vehicles wait for the signal from the police officer to pass the intersection
		// while(lanes_decided != LANES);
		// printf("lanes decided: %d\n", lanes_decided);
		// pthread_mutex_lock(&intersection);
		// 	pthread_cond_wait(&passing, &intersection); // ikinci turda burada takiliyor, cunku line 261 sinyal veremiyor cunku line 252 busy waitte
		// 	printf("signal is received\n");

		// 	pthread_mutex_lock(&queue_mutex);
		// 		printf("lock is acquired\n");
		// 		int ind = dequeue();
		// 		printf("We have removed from lane %d\n", ind);
		// 	pthread_mutex_unlock(&queue_mutex);

		// pthread_mutex_unlock(&intersection);

	}
	pthread_exit(0);
}


/* Pollice officer thread function */
void *police_func() {
	while (!is_finished()){

		// Print the current simulation time.
		gettimeofday(&get_time, NULL);
        current_time = get_time.tv_sec;
        printf("current time now: %ld\n", current_time);

		// Notify the lanes that there is a police on the simulation.
		printf("This is police function\n");
		pthread_mutex_lock(&police_checking);
			pthread_cond_broadcast(&new_turn);
		pthread_mutex_unlock(&police_checking);

		// Wait for all the lanes finish their operations. (Decide whether to put a vehicle to the lane or not)
		while(lanes_decided != LANES); // busy wait
		pthread_mutex_lock(&lane_count);
			printf("All lanes completed their actions\n");
			lanes_decided = 0;
		pthread_mutex_unlock(&lane_count);

		// Signal for dequeue
		// pthread_mutex_lock(&intersection);
		// if(queue_count > 0){
		// 		pthread_cond_signal(&passing);
		// 		printf("signal is sent\n");
		// }
		// pthread_mutex_unlock(&intersection);


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

int dequeue() {
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
	queue_count--;

	return data;
}
