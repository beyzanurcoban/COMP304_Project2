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
