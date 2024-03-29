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

int t;	// Command Line argument t
int sim_counter = 0;
struct tm *time_info;
char time_char[64];

int phone_counter = 0;	// count of cell phone occurences
int is_police_sleep = 0;	// Police sleep boolean

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
int north_def_wait = 0;

FILE *car_fp;
struct tm *arr_info;
struct tm *leave_info;
char arr_time[64];
char leave_time[64];

FILE *police_fp;
struct tm *event_info;
char event_time[64];

struct tm *cur_info;
char cur_time[64];

char lane_char;

// Mutex, Conditional Variables, Semaphores, Locks
pthread_mutex_t police_checking;
pthread_mutex_t id_mutex;
pthread_cond_t new_turn;
pthread_mutex_t lane_count; // mutex for incrementing decided lane numbers
pthread_mutex_t queue_mutex; // only one thread can make an operation on the queue
pthread_mutex_t intersection; // only one car is allowed to pass the intersection
pthread_mutex_t north_def_wait_mutex; // used when checking or modifying wait times of lanes
pthread_mutex_t police_sleep_mutex; // used to keep police sleep counter intact
pthread_cond_t police_wakes_up; // used for 3 sec. sleep of police in case no car is around.
pthread_mutex_t sim_counter_mutex; // used for time intervals
pthread_mutex_t phone_counter_mutex;

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
void keep_car_log(struct Car *temp, int lane_num);
void keep_police_log(int event, char log_time[]);

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
    * -s $(total simulation time) -p $(probability) -t $(time interval)
    */

    if(argc != 7) {
        perror("Not valid number of argument to run the program\n");
        return -1;
    }

    for(int i=1; i<argc; i+=2) {
        if(strcmp("-s", argv[i]) == 0) {
            sim_time = atoi(argv[i+1]);
        } else if(strcmp("-p",argv[i]) == 0) {
            p = strtod(argv[i+1], NULL);
        } else if(strcmp("-t", argv[i]) == 0){
			t = atoi(argv[i+1]);
		} else {
            perror("You specified an undefined flag!\n");
            return -1;
        }
    }

    printf("Simulation Time:\t%d seconds\nProbability:\t%f\n", sim_time, p);
	printf("We will provide the snapshots of the intersection every %d seconds.\n", t);
	
	// Get the current time in seconds, decide start time and calculate finish time
    gettimeofday(&get_time, NULL);
    start_time = get_time.tv_sec;
	finish_time = start_time + sim_time;

	cur_info = localtime(&start_time);
	strftime(cur_time, sizeof cur_time, "%H:%M:%S", cur_info);
    printf("Start Time:\t%s\n", cur_time);

	cur_info = localtime(&finish_time);
	strftime(cur_time, sizeof cur_time, "%H:%M:%S", cur_info);
	printf("Finish Time:\t%s\n\n", cur_time);

	// Mutex initialization
	pthread_mutex_init(&police_checking, NULL);
	pthread_mutex_init(&id_mutex, NULL);
	pthread_cond_init(&new_turn, NULL);
	pthread_mutex_init(&lane_count, NULL);
	pthread_mutex_init(&queue_mutex, NULL);
	pthread_mutex_init(&intersection, NULL);
	pthread_mutex_init(&north_def_wait_mutex, NULL);
	pthread_cond_init(&police_wakes_up, NULL);
	pthread_mutex_init(&sim_counter_mutex, NULL);
	pthread_mutex_init(&phone_counter_mutex, NULL);

	return 0;
	
}


int main(int argc, char *argv[]) {

	program_init(argc, argv);
	srand(7); // initialize random number generator seed

	// Open the log files
	car_fp = fopen("car.log", "w");
	fprintf(car_fp, "CarID\t\tDirection\t\tArrival-Time\t\tCross-Time\t\tWait-Time\n");
	police_fp = fopen("police.log", "w");
	fprintf(police_fp, "Time\t\t\tEvent\n");

	// At the beginning, there are one vehicle at each lane, queue[0] means there is a car in N lane
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

	printf("\nSIMULATION IS DONE!!!\n");
	
	/* Destroy mutex */
	pthread_mutex_destroy(&police_checking);
	pthread_mutex_destroy(&id_mutex);
	pthread_cond_destroy(&new_turn);
	pthread_mutex_destroy(&lane_count);
	pthread_mutex_destroy(&queue_mutex);
	pthread_mutex_destroy(&intersection);
	pthread_mutex_destroy(&north_def_wait_mutex);
	pthread_cond_destroy(&police_wakes_up);
	pthread_mutex_destroy(&sim_counter_mutex);
	pthread_mutex_destroy(&phone_counter_mutex);
	
	fclose(car_fp);
	fclose(police_fp);

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
			//printf("Hi! This is Lane %d.\n", l_id);

			// E, S, W is produced with probability p.
			if(l_id != 0){
				if(is_occur(p)){
					printf("At Lane %d, Vehicle %d has arrived.\n", l_id, vehicleID);

					pthread_mutex_lock(&police_sleep_mutex);
						if(queue_count == 0){
							pthread_cond_signal(&police_wakes_up);
							printf("HONK!!\n");
							keep_police_log(1, cur_time);
							pthread_mutex_lock(&phone_counter_mutex);
								is_police_sleep = 1;
								phone_counter = 3;
							pthread_mutex_unlock(&phone_counter_mutex);
						}
					pthread_mutex_unlock(&police_sleep_mutex);

					enqueue(vehicleID, l_id, current_time);
				}
			// N is produces with probability 1-p.
			} else {
				if(1-is_occur(p)){
					pthread_mutex_lock(&north_def_wait_mutex);
						// If 20 sec. wait time is over, new car can arrive at North.
						if(north_def_wait == 0) {
							printf("At Lane %d, Vehicle %d has arrived.\n", l_id, vehicleID);

							pthread_mutex_lock(&police_sleep_mutex);
								if(queue_count == 0){
									pthread_cond_signal(&police_wakes_up);
									printf("HONK!!\n");
									keep_police_log(1, cur_time);
									pthread_mutex_lock(&phone_counter_mutex);
										is_police_sleep = 1;
										phone_counter = 3;
									pthread_mutex_unlock(&phone_counter_mutex);
								}
							pthread_mutex_unlock(&police_sleep_mutex);

							enqueue(vehicleID, l_id, current_time);
						}
					pthread_mutex_unlock(&north_def_wait_mutex);
				} else {
					// If no car has arrived to North for the first time since last arrival, 20 sec. definite wait for North
					pthread_mutex_lock(&north_def_wait_mutex);
						if(north_def_wait == 0) {
							north_def_wait = 20;
							printf("No Vehicle has arrived to North. 20 sec. definite wait at North.\n");
						}
					pthread_mutex_unlock(&north_def_wait_mutex);
				}
			}
		pthread_mutex_unlock(&queue_mutex);

		pthread_mutex_lock(&lane_count);
			lanes_decided++;
		pthread_mutex_unlock(&lane_count);

	}
	pthread_exit(0);
}


/* Police officer thread function */
void *police_func() {
	while (!is_finished()){

		// Print the current simulation time.
		gettimeofday(&get_time, NULL);
        current_time = get_time.tv_sec;
		cur_info = localtime(&current_time);
		strftime(cur_time, sizeof cur_time, "%H:%M:%S", cur_info);
        printf("Current Time:\t%s\n", cur_time);

		// After 1 second, we dequeued a car and we show it.
		pthread_mutex_lock(&queue_mutex);
			if(last_dequeued_lane != 4){
				printf("Vehicle %d from the lane %d finished their passing process in 1 second.\n", dequeuedVehicleID, last_dequeued_lane);
			}
		pthread_mutex_unlock(&queue_mutex);


		pthread_mutex_lock(&police_sleep_mutex);
			if(queue_count == 0){
				keep_police_log(0, cur_time);
			}
		pthread_mutex_unlock(&police_sleep_mutex);

		// If North has started to count down from 20 sec, keep going.
		pthread_mutex_lock(&north_def_wait_mutex);
			if(north_def_wait != 0) {
				north_def_wait--;
				printf("North's wait time: %d\n", north_def_wait);
			}
		pthread_mutex_unlock(&north_def_wait_mutex);

		// Notify the lanes that there is a police on the simulation.
		//printf("Hi! This is The Police.\n");
		pthread_mutex_lock(&police_checking);
			pthread_cond_broadcast(&new_turn);
		pthread_mutex_unlock(&police_checking);

		// Wait for all the lanes finish their operations. (Decide whether to put a vehicle to the lane or not)
		while(lanes_decided != LANES); // busy wait
		printf("All lanes have completed their actions.\n");

		pthread_mutex_lock(&sim_counter_mutex);
			if((sim_counter % t == 0 || sim_counter % t == 1 || sim_counter % t == 2) && sim_counter != 0 && sim_counter != 1 && sim_counter != 2){
				time_info = localtime(&current_time);
				strftime (time_char, sizeof time_char, "At %H:%M:%S:\n", time_info);
				printf("%s", time_char);

				printf("\t%d\n", north_count);
				printf("%d\t\t%d\n", west_count, east_count);
				printf("\t%d\n", south_count);
			}
		pthread_mutex_unlock(&sim_counter_mutex);

		// Visualize Queue Before A Vehicle is Let Off
		pthread_mutex_lock(&queue_mutex);
			printf("Queue Before (%d):\t", queue_count);
			struct Car *before = front;
			while(before != NULL){
				printf(" %d", before->lane);
				before = before->next;
			}
			printf("\n");
		pthread_mutex_unlock(&queue_mutex);

		pthread_mutex_lock(&police_sleep_mutex);
			if(is_police_sleep == 1){
				printf("Police was playing on his phone.\n");
				pthread_cond_wait(&police_wakes_up, &police_sleep_mutex);
			}
		pthread_mutex_unlock(&police_sleep_mutex);

		pthread_mutex_lock(&queue_mutex);
		pthread_mutex_lock(&phone_counter_mutex);
			if(phone_counter == 0 && queue_count != 0) {
				// Dequeue
				// Vehicles wait for the signal from the police officer to pass the intersection
				pthread_mutex_lock(&intersection);

					// A Vehicle is Let Off
					dequeue();
					is_police_sleep = 0;

				pthread_mutex_unlock(&intersection);
		} else {
			phone_counter--;
		}
		pthread_mutex_unlock(&phone_counter_mutex);
		pthread_mutex_unlock(&queue_mutex);

		// Reset Lane Decision Count
		pthread_mutex_lock(&lane_count);
			lanes_decided = 0;
		pthread_mutex_unlock(&lane_count);

		// Visualize Queue After A Car is Sent Off
		pthread_mutex_lock(&queue_mutex);
			printf("Queue After (%d):\t", queue_count);
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

		pthread_mutex_lock(&sim_counter_mutex);
			sim_counter++;
		pthread_mutex_unlock(&sim_counter_mutex);

	}
	
	pthread_exit(0);
}

/* Queue Operations */
void enqueue(int id, int lane, long a_time) {
	struct Car *newCar = malloc(sizeof(struct Car));
	newCar->id = id;
	newCar->arrive_time = a_time;
	newCar->lane = lane;
	newCar->waiting = -1;
	newCar->next = NULL;

	if(front == NULL){
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
			if(is_police_sleep == 1){
				waitCar->waiting = waitCar->waiting+4;
			} else {
				waitCar->waiting = waitCar->waiting+1;
			}
			
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
			temp->leave_time = temp->arrive_time + temp->waiting;

			keep_car_log(temp, lane_selected);
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
		temp->leave_time = temp->arrive_time + temp->waiting;

		keep_car_log(temp, lane_selected);
		free(temp);
		
		printf("Vehicle %d was sent off from Lane %d\n", dequeuedVehicleID, lane_selected);

		if(queue_count == 1) {
			front = rear;
		}

		if(queue_count == 0) {
			front = NULL;
			rear = NULL;
		}

	}

}

/* Method to write the car.log file */
void keep_car_log(struct Car *temp, int lane_num) {
	// Convert the arrive time to the desired hour:minute:second format
	arr_info = localtime(&temp->arrive_time);
	strftime (arr_time, sizeof arr_time, "%H:%M:%S", arr_info);
	// Convert the leave time to the desired hour:minute:second format
	leave_info = localtime(&temp->leave_time);
	strftime (leave_time, sizeof leave_time, "%H:%M:%S", leave_info);
	// Set Lane character
	if(lane_num == 0) lane_char = 'N';
	if(lane_num == 1) lane_char = 'E';
	if(lane_num == 2) lane_char = 'S';
	if(lane_num == 3) lane_char = 'W';
	// Write the car info
	fprintf(car_fp, "%d\t\t\t%c\t\t\t\t%s\t\t\t%s\t\t%d\n", dequeuedVehicleID, lane_char, arr_time, leave_time, temp->waiting);
}

/* Method to write the police.log file */
void keep_police_log(int event, char log_time[]) {
	if(event == 0) {
		// Police starts playing with his cell phone.
		fprintf(police_fp, "%s\t\tCell Phone\n", log_time);
	} else if(event == 1) {
		// A car honks at Police.
		fprintf(police_fp, "%s\t\tHonk\n", log_time);
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
	// printf("Lane 0 waited %d seconds after the last dequeue.\n", wait_times[0]);
	// printf("Lane 1 waited %d seconds after the last dequeue.\n", wait_times[1]);
	// printf("Lane 2 waited %d seconds after the last dequeue.\n", wait_times[2]);
	// printf("Lane 3 waited %d seconds after the last dequeue.\n", wait_times[3]);

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
