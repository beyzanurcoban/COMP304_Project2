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

int sim_time;
double p;
long start_time;
struct timeval get_time;

int main(int argc, char *argv[]) {

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

	gettimeofday(&get_time, NULL);
	start_time = get_time.tv_sec;

	printf("start time in secs: %ld\n", start_time);

}


