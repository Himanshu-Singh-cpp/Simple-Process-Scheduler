***************************************  DESIGN DOCUMENT ******************************************

In this assignment we have created a process scheduler. This runs in the background of the Simple Shell.

We have implemented both Round Robin Scheduling and Priority Based Scheduling.

We have created 3 main files in our assignment:

a) SimpleScheduler.c
b) SimpleShell.c
c) dummy_main.h

a) In our SimpleScheduler, we have the following functions:

   1) struct program *pop(struct queue *q)
   2) void push(struct program *bottom, struct queue *q)
   3) bool isEmpty(struct queue *q)
   4) bool isFull(struct queue *q)
   5) struct program *top(struct queue *q)
   6) void create_process(int priority)
   7) void resume_process(struct program *p)
   8) int pause_process(struct program *p)
   9) void history()
   10) void intHandler(int dummy)
   11) void make_struct_and_push(char *command, int priority)
   12) void execute_queue(int j)
   
For Inter-Process Communication between the SimpleShell(Parent) and the SimpleScheduler(Child), 
we have made use of Shared Memory.

When the Shell is terminated, we are displaying the History which consists of the names of all the commands executed, 
their waiting times, and their execution times.

We have done the appropriate error-checking at the required places.

For Analysis of effect of priority on job-scheduling, we hard coded some commands into our shell program. The following
were the observations (With ncpu = 1)

        Command       |      TSLICE     |     Priority     |         Execution Time       |           Waiting Time          
----------------------*-----------------*------------------*------------------------------*---------------------------------                                                                                                           
      submit ./fib    |      1000 ms    |         1        |          4.253386 ms         |            2.127059 ms
      submit ./fib    |      1000 ms    |         2        |          6.881923 ms         |            4.879852 ms
      submit ./fib    |      1000 ms    |         3        |          7.883303 ms         |            5.880824 ms
      submit ./fib    |      1000 ms    |         4        |          8.258849 ms         |            6.130955 ms
----------------------*-----------------*------------------*------------------------------*---------------------------------
      submit ./fib    |      100 ms     |         1        |          7.712911 ms         |            5.678137 ms
      submit ./fib    |      100 ms     |         2        |          7.940893 ms         |            5.930352 ms
      submit ./fib    |      100 ms     |         3        |          8.067755 ms         |            6.044267 ms
      submit ./fib    |      100 ms     |         4        |          8.080604 ms         |            6.069267 ms
----------------------*-----------------*------------------*------------------------------*---------------------------------
      submit ./fib    |       10 ms     |         1        |          8.332961 ms         |            6.238117 ms
      submit ./fib    |       10 ms     |         2        |          8.368340 ms         |            6.265439 ms
      submit ./fib    |       10 ms     |         3        |          8.365438 ms         |            6.268000 ms
      submit ./fib    |       10 ms     |         4        |          8.307322 ms         |            6.238468 ms




Statistical Calculations Based on the Above Data :

1. TSLICE = 1000 ms:
   
   a) Mean Execution Time =          6.8193653 ms
   b) Variance in Execution Time =   3.2640800 ms
   c) Mean Waiting Time =            4.7546725 ms
   d) Variance in Waiting Time =     3.3607978 ms

2. TSLICE = 100 ms:

   a) Mean Execution Time =          7.9505408 ms
   b) Variance in Execution Time =   0.0290722 ms
   c) Mean Waiting Time =            5.9305058 ms
   d) Variance in Waiting Time =     0.0319621 ms

3. TSLICE = 10 ms:

   a) Mean Execution Time =          8.3435153 ms
   b) Variance in Execution Time =   0.0008394 ms
   c) Mean Waiting Time =            6.2525060 ms
   d) Variance in Waiting Time =     0.0002704 ms


Observations : 

1. Variance in Waiting Time is directly proportional to the value of TSLICE.
2. Variance in Execution Time is directly proportional to the value of TSLICE.
3. Mean Execution Time is inversely proportional to value of TSLICE.
4. Mean Waiting Time is inversely proportional to value of TSLICE.

Team Members:

Himanshu Singh    2022217
Vijval Ekbote     2022569

We have both contributed equally towards this assignment.



   

