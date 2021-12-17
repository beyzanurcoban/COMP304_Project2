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
#define MAX 2048

// There is a unique id for each direction. Whenever you see lane 0, it means N lane.
// N -> 0
// E -> 1
// S -> 2
// W -> 3

// Global Variables
int sim_time;
double p;
time_t start_time;
time_t current_time;
time_t finish_time;
struct timeval get_time;
int lanes_decided = 0;
int id = 0;

int north_count = 0;
int east_count = 0;
int south_count = 0;
int west_count = 0;

int queue_count = 0;

int last_dequeued_lane = 4;
int vehicleID = 0;
int dequeuedVehicleID;

struct Car *front = NULL;
struct Car *rear = NULL;

int wait_times[LANES];


// Mutex, Conditional Variables, Semaphores, Locks
pthread_mutex_t police_checking;
pthread_mutex_t id_mutex;
pthread_cond_t new_turn;
pthread_mutex_t lane_count; // mutex for incrementing decided lane numbers
pthread_mutex_t queue_mutex; // only one thread can make an operation on the queue
pthread_mutex_t intersection; // only one car is allowed to pass the intersection

// Function Declarations
int program_init(int argc, char *argv[]);
int is_occur(double p);
int is_finished();
void *lane_func();
void *police_func();
void enqueue(int id, int lane, long a_time);
void dequeue();
int from_diff_lane();
int dequeue_decision();
void decide_wait_times();

struct Car {
	int id;
	long arrive_time;
	long leave_time;
	int waiting;
	int lane;
	struct Car *next;
};

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

	return 0;
	
}


int main(int argc, char *argv[]) {

	program_init(argc, argv);
	srand(7); // initialize random number generator seed

	// At the beginnin, there are one vehicle at each lane, queue[0] means there is a car in N lane
	enqueue(vehicleID, 0, start_time);
	enqueue(vehicleID, 1, start_time);
	enqueue(vehicleID, 2, start_time);
	enqueue(vehicleID, 3, start_time);
	
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
	
	/* Destroy mutex */
	pthread_mutex_destroy(&police_checking);
	pthread_mutex_destroy(&id_mutex);
	pthread_cond_destroy(&new_turn);
	pthread_mutex_destroy(&lane_count);
	pthread_mutex_destroy(&queue_mutex);
	pthread_mutex_destroy(&intersection);

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
			printf("Hi! This is Lane %d.\n", l_id);

			// E, S, W is produced with probability p.
			if(l_id != 0){
				if(is_occur(p)){
				printf("At Lane %d, Vehicle %d has arrived.\n", l_id, vehicleID);
				enqueue(vehicleID, l_id, current_time);
			}
			// N is produces with probability 1-p.
			} else {
				if(1-is_occur(p)){
					printf("At Lane %d, Vehicle %d has arrived.\n", l_id, vehicleID);
					enqueue(vehicleID, l_id, current_time);
				}
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

		// Print the current simulation time.
		gettimeofday(&get_time, NULL);
        current_time = get_time.tv_sec;
        printf("Current Time:\t%ld\n", current_time);

		// After 1 second, we dequeued a car and we show it.
		if(last_dequeued_lane != 4){
			printf("Vehicle % d from the lane %d finished their passing process in 1 second.\n", dequeuedVehicleID, last_dequeued_lane);
		}

		// Notify the lanes that there is a police on the simulation.
		printf("Hi! This is The Police.\n");
		pthread_mutex_lock(&police_checking);
			pthread_cond_broadcast(&new_turn);
		pthread_mutex_unlock(&police_checking);

		// Wait for all the lanes finish their operations. (Decide whether to put a vehicle to the lane or not)
		while(lanes_decided != LANES); // busy wait
		printf("All lanes have completed their actions.\n");

		// Dequeue
		// Vehicles wait for the signal from the police officer to pass the intersection
		while(lanes_decided != LANES);
		pthread_mutex_lock(&intersection);

			// Visualize Queue Before A Vehicle is Let Off
			pthread_mutex_lock(&queue_mutex);
				printf("Queue Before:\t");
				struct Car *before = front;
				while(before != NULL){
					printf(" %d", before->lane);
					before = before->next;
				}
				printf("\n");
			pthread_mutex_unlock(&queue_mutex);

			pthread_mutex_lock(&queue_mutex);
				dequeue();
			pthread_mutex_unlock(&queue_mutex);

		pthread_mutex_unlock(&intersection);

		// Reset Lane Decision Count
		pthread_mutex_lock(&lane_count);
			lanes_decided = 0;
		pthread_mutex_unlock(&lane_count);
		
		// Visualize Queue After A Car is Sent Off
		pthread_mutex_lock(&queue_mutex);
			printf("Queue After:\t");
			struct Car *after = front;
			while(after != NULL){
				printf(" %d", after->lane);
				after = after->next;
			}
			printf("\n");
		pthread_mutex_unlock(&queue_mutex);

		// Car from decided lane passes
		printf("\n");
		pthread_sleep(1);

	}
	
	pthread_exit(0);
}

/* Queue Operations */
void enqueue(int id, int lane, long a_time) {
	struct Car *newCar = malloc(sizeof(struct Car));
	newCar->id = id;
	newCar->arrive_time = a_time;
	newCar->lane = lane;
	newCar->waiting = 0;
	newCar->next = NULL;

	if(front == NULL && rear == NULL){
		front = rear = newCar;
	} else {
		rear->next = newCar;
		rear = newCar;
	}

	if(lane==0) north_count++;
	if(lane==1) east_count++;
	if(lane==2) south_count++;
	if(lane==3) west_count++;

	queue_count++;
	vehicleID++;
}

void dequeue() {
	int decision = 4;
	int lane_selected = 0;

	if (front == NULL) {
		printf("Queue is empty!\n");
	} else {
		
		decision = dequeue_decision();

		if(decision == 4){
			lane_selected = from_diff_lane();
		} else {
			lane_selected = decision;
		}
		// Keep the last dequeued lane to decide the next
		last_dequeued_lane = lane_selected;

		printf("Lane %d has been selected by police.\n", lane_selected);

		if(lane_selected == 0) { north_count--; }
		else if(lane_selected == 1) { east_count--; }
		else if(lane_selected == 2) { south_count--; }
		else if(lane_selected == 3) { west_count--; }

		queue_count--;

		// Increase the waiting times of the cars in the list
		struct Car *waitCar = front;
		while(waitCar != NULL){
			waitCar->waiting = waitCar->waiting+1;
			waitCar = waitCar->next;
		}

		// Deleting from the linked list
		struct Car *temp;
		temp = front;

		struct Car *prev;

		// If the lane will be dequeued is in the first position in the list
		if(temp->lane == lane_selected){
			front = temp->next;
			dequeuedVehicleID = temp->id;
			free(temp);
			return;
		}

		// Else, find the first occurence of that lane in the list
		while(temp != NULL && temp->lane != lane_selected){
			prev = temp;
			temp = temp->next;
		}
		dequeuedVehicleID = temp->id;

		// If we cannot find, we return
		if(temp == NULL){
			return;
		}

		// If we delete the last element in the list, rear should be updated
		if(temp == rear){
			rear = prev;
		}

		prev->next = temp->next;
		free(temp);

		printf("Vehicle %d was sent off from Lane %d\n", dequeuedVehicleID, lane_selected);

	}

}

/* If we decide to dequeue from a different lane, this method decides which lane it will be */
int from_diff_lane(){
	int lane_selected = 0;

	// Select line to be dequeued
	if(west_count > south_count && west_count > east_count && west_count > north_count) { lane_selected = 3; }
	else if(south_count > east_count && south_count > north_count) { lane_selected = 2; }
	else if(east_count > north_count) { lane_selected = 1; }
	else { lane_selected = 0; }

	return lane_selected;
}

/* Method to decide whether we should continue from the same lane or not */
int dequeue_decision() {
	int decision = 4;

	/* If there is at least one car in the current lane, and other lanes do not have at least 5 cars yet:
	 * You should dequeue from the same line */
	if(last_dequeued_lane == 0){
		if(north_count > 0 && east_count < 5 && south_count < 5 && west_count < 5){
			decision = 0;
		}
	} else if(last_dequeued_lane == 1){
		if(east_count > 0 && north_count < 5 && south_count < 5 && west_count < 5){
			decision = 1;
		}
	} else if(last_dequeued_lane == 2){
		if(south_count > 0 && north_count < 5 && east_count < 5 && west_count < 5){
			decision = 2;
		}
	} else if(last_dequeued_lane == 3){
		if(west_count > 0 && north_count < 5 && east_count < 5 && south_count < 5){
			decision = 3;
		}
	}

	// PART II - Start
	/* We check the waiting times 
	 * We check part (c) after checking the (b) part
	 * because we want a priority of (c), this will overwrite the decision even if (b) holds */
	decide_wait_times();
	printf("Lane 0 waited %d seconds after the last dequeue.\n", wait_times[0]);
	printf("Lane 1 waited %d seconds after the last dequeue.\n", wait_times[1]);
	printf("Lane 2 waited %d seconds after the last dequeue.\n", wait_times[2]);
	printf("Lane 3 waited %d seconds after the last dequeue.\n", wait_times[3]);

	if(wait_times[0] >= 20){
		decision = 0;
	} else if(wait_times[1] >= 20){
		decision = 1;
	} else if(wait_times[2] >= 20){
		decision = 2;
	} else if(wait_times[3] >= 20){
		decision = 3;
	}
	//  PART II - Finish

	/* If there is no car in the current lane, or there are at least 5 cars in the other lanes:
	 * You should switch the lane, we return 4 for this change  */
	return decision;
}

/* Method to find the maximum waiting time in each line */
void decide_wait_times() {
	struct Car *node = front;

	// Decide the waiting time of the first car in Lane North
	if(north_count==0) {
		wait_times[0] = 0;
	} else {
		while(node != NULL){
			if(node->lane == 0){
				wait_times[0] = node->waiting;
				break;
			}
			node = node->next;
		}
	}

	// Decide the waiting time of the first car in Lane East
	node = front;
	if(east_count==0) {
		wait_times[1] = 0;
	} else {
		while(node != NULL){
			if(node->lane == 1){
				wait_times[1] = node->waiting;
				break;
			}
			node = node->next;
		}
	}

	// Decide the waiting time of the first car in Lane South
	node = front;
	if(south_count==0) {
		wait_times[2] = 0;
	} else {
		while(node != NULL){
			if(node->lane == 2){
				wait_times[2] = node->waiting;
				break;
			}
			node = node->next;
		}
	}

	// Decide the waiting time of the first car in Lane West
	node = front;
	if(west_count==0) {
		wait_times[3] = 0;
	} else {
		while(node != NULL){
			if(node->lane == 3){
				wait_times[3] = node->waiting;
				break;
			}
			node = node->next;
		}
	}

}
