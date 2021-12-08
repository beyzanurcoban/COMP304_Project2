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


// Global Variables
int sim_time;
double p;
long start_time;
long current_time = 0;
long finish_time;
struct timeval get_time;

// Function Declarations
int program_init(int argc, char *argv[]);
int is_occur(double p);
int is_finished();
void *vehicle_func(void *id);

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
	printf("finish time in secs: %ld\n", finish_time);
}


int main(int argc, char *argv[]) {

	program_init(argc, argv);
	srand(7); // initialize random number generator seed

	/* Creating vehicle threads */
	pthread_t vehicles[100];
	int index = 0;

	while(!is_finished()) {
		gettimeofday(&get_time, NULL);
        	current_time = get_time.tv_sec;
        	printf("current time now: %ld\n", current_time);

		if(is_occur(p)) {
			pthread_create(&vehicles[index], NULL, vehicle_func, (void *)index);
			index++;
		}
       		sleep(1);
	}
	
	/* Join the worker threads to the master thread */
	for (int j=0; j<=index; j++) {
                pthread_join(vehicles[j], NULL);
        }


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
void *vehicle_func(void *id) {
	int vehic_id = (int) id;

	printf("This is vehicle %d.\n", vehic_id);
}


