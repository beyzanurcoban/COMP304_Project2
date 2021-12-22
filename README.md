# COMP304_Project2

This is the second project of the COMP304 course (Fall'21) in Koç University.

## Contributors
Beyzanur Çoban\
Kerem Girenes

## Compiling

Use the following command to compile the program. You need -lpthread flag because the program contains threads.

```bash
gcc simulation.c -o simulation -lpthread
```
## Executing

Use the following command to execute the program. -s flag determines the simulation time, -p flag determines the\
probability for the vehicle creation. -t flag determines the time intervals in which we print snapshots of the\
current lane counts in a diamond shaped fashion.

```bash
./simulation -s 20 -p 0.2 -t 7
```

## Working Parts

We implemented all parts of the project. If you give the valid command line arguments, then it will produce an\
output. There is no error or segmentation fault in the code. 

## Implementation

In this project, we aimed to manage the cars without creating a deadlock and any car crash.\
In order to prevent the car collisions at the intersection point, we used a mutex.\
Also, we wanted the police function and thread function to work in an order. Thus, we used\
a conditional variable to implement a turn. In the enqueue and dequeue operations, we paid\
attention to using a mutex. Because, two lane threads could try to create and enqueue the car\ 
at the same time. Or while a lane enqueueing a car, the police could try to dequeue a car.\ 
Since we are using one linked list, we needed to prioritize the operations.\
In part II, we implemented a new rule for lane deciding such that a lane flows until the conditions\ 
specified in the project description holds. We created an array of four integers, each keeping track\ 
of the maximum time any car in that lane has waited.\
In part III, we included a police_wakes_up conditional variable to make the police wait for\ 
a Honk, and for a car to signal a Honk.