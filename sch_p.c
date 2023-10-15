#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>

int count = 0;
int buffer = 0;

int NCPU;
int TSLICE;

struct program
{
    pid_t pid;
    bool created;
    double start;
    double end;
    double time;
    int cycles;
    char *cmd;
    int priority;
};

struct queue
{
    struct program *arr[100];
    int start;
    int end;
};

int num_commands = 0;
struct program *history_arr[100];
struct queue *q_arr[4];
int capacity = 100;
static volatile int keepRunning = 1;

// removes a front most process from the queue
struct program *pop(struct queue *q)
{
    if (q->end != q->start)
    {
        if (q->end == 0)
            q->end = 99;
        else
            q->end--;
        return q->arr[q->end];
    }
    else
    {
        printf("Stack is empty\n");
        return NULL;
    }
}

// push a process at start of the queue
void push(struct program *bottom, struct queue *q)
{
    if ((q->end + 1) % capacity != q->start)
    {
        if (q->start == 0)
            q->start = 99;
        else
            q->start--;
        q->arr[q->start] = bottom;
    }
    else
        printf("Array size reached\n");
}

// retuns true if queue empty else return false
bool isEmpty(struct queue *q)
{
    if (q->end - q->start == 0)
        return true;
    return false;
}

// returns true if queue if full else returns false
bool isFull(struct queue *q)
{
    if ((q->end + 1) % capacity == q->start)
        return true;
    return false;
}

// returns the top most process of queue
struct program *top(struct queue *q)
{
    if (q->end == 0)
        return q->arr[99];
    return q->arr[q->end - 1];
}

// execv a process
void create_process(int priority)
{
    pid_t pid;
    int status;

    pid = fork();

    if (pid < 0)
    {
        printf("Fork failed\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // Child process
        char *args[2];
        args[0] = top(q_arr[priority-1])->cmd;
        args[1] = NULL;
        execvp(args[0], args);
        perror("Execution failed\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        top(q_arr[priority-1])->pid = pid;
    }
}

// resumes a process by sending it signal
void resume_process(struct program *p)
{
    kill(p->pid, SIGCONT);
}

// pause a process by sending it signal
int pause_process(struct program *p)
{
    return kill(p->pid, SIGSTOP);
}

// prints history and relevant info 
void history()
{
    printf("Printing the history...\n");
    printf("no of commands %d\n", num_commands);
    for (int i = 0; i < num_commands; i++)
    {
        // double execution_time = history_arr[i]->cycles * TSLICE;
        double total_time = (double)(history_arr[i]->end - history_arr[i]->start);
        printf("\ncommand %s\n", history_arr[i]->cmd);
        printf("pid is %d\n", history_arr[i]->pid);
        printf("priority %d",history_arr[i]->priority);
        printf("Execution time %f\n", total_time);
        // printf("Waiting Time %f\n", total_time - history_arr[i]->cycles * TSLICE / 1000.0);
        printf("waiting time %f",history_arr[i]->time);
    }
}

// parameter of while loop is set to false and while loop waits for natural termination
void intHandler(int dummy)
{
    keepRunning = 0;
}

void make_struct_and_push(char *command, int priority)
{
    // sets the start time, make array for execv and push process in the queue
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    struct program *p1 = (struct program *)malloc(sizeof(struct program));
    p1->cmd = command;
    p1->created = false;
    p1->start = (double)start_time.tv_sec + start_time.tv_usec / 1000000.0;
    p1->priority=priority;
    push(p1, q_arr[priority-1]);
}

int main()
{

    // signal handler for control c to end program after natural termination
    signal(SIGINT, intHandler);

    for (int i = 0; i < 4; i++) {
        q_arr[i] = (struct queue*)malloc(sizeof(struct queue));
        if (q_arr[i] == NULL) {
            printf("Memory allocation failed for q_arr[%d]\n", i);
        }
        q_arr[i]->start = 99;
        q_arr[i]->end = 99;
    }
    // printf("alloacted memory\n");

    // shared memory string to store all commands
    const char *name = "shared_memory";
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 1024);
    char *shm = (char *)mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // shared memory int to store how many commands recieved during the sleep time
    const char *num_command = "num_command";
    int fd_int = shm_open(num_command, O_CREAT | O_RDWR, 0666);
    ftruncate(fd_int, 24);
    int *shm_count = (int *)mmap(NULL, 24, PROT_READ | PROT_WRITE, MAP_SHARED, fd_int, 0);

    // recieving value of NCPU and TSLICE from shell
    NCPU = *(shm_count + 1);
    TSLICE = *(shm_count + 2);

    const char* name_p="name_p";
    int fd_p = shm_open(name_p, O_CREAT | O_RDWR, 0666);
    ftruncate(fd_p,400);
    int* shm_p=(int*)mmap(NULL,400,PROT_READ| PROT_WRITE,MAP_SHARED,fd_p,0);

    struct program *temp_arr[NCPU];
    // while loop which is executed at end of each time slice
    while ((keepRunning == 1) || q_arr[0]->start!=q_arr[0]->end || q_arr[1]->start!=q_arr[1]->end ||q_arr[2]->start!=q_arr[2]->end ||q_arr[3]->start!=q_arr[3]->end || (count < *shm_count))
    {
        if (shm != NULL)
        {
            // printf("count of commands is %d\n",count);
            // checking if new command it sent from shell to scheduler
            while (count < *shm_count)
            {
                // retrieve the priority
                make_struct_and_push(shm + buffer, *(shm_p+count));
                buffer += strlen(shm + buffer) + 1;
                count++;
            }
        }
        printf("Prioritizing queus\n");
        for (int j = 3; j >= 0; j--)
        {
            printf("entered the loop\n");
            printf("%dth start %d end %d\n",j, q_arr[j]->start, q_arr[j]->end);

            // counts the number of process in queue
            int count_current = 0;
            // run the processes equal to NCPU
            for (int i = 0; i < NCPU; i++)
            {
                if (!isEmpty(q_arr[j]))
                {
                    count_current++;

                    if (top(q_arr[j])->created)
                    {
                        struct timeval middle;
                        gettimeofday(&middle, NULL);
                        top(q_arr[j])->time = middle.tv_sec + middle.tv_usec / 1000000.0 - top(q_arr[j])->end;
                        printf("Resumed process\n");
                        resume_process(top(q_arr[j]));
                    }
                    else
                    {
                        struct timeval middle;
                        gettimeofday(&middle, NULL);
                        top(q_arr[j])->time = middle.tv_sec + middle.tv_usec / 1000000.0 - top(q_arr[j])->start;
                        top(q_arr[j])->created = true;
                        create_process(top(q_arr[j])->priority);
                    }
                    top(q_arr[j])->cycles++;

                    temp_arr[i] = pop(q_arr[j]);
                }
            }
            // scheduler goes to sleep for TSLICE time
            // printf("gone to sleep\n");
            if(j==0 || (count_current>0)){
                usleep(TSLICE * 1000);
            }
            // printf("exited the sleep\n");
            // Sends stop signal to processes, if the process not ended process is added to back of the queue
            for (int i = 0; i < NCPU; i++)
            {
                if (i < count_current)
                {
                    if (kill(temp_arr[i]->pid, SIGSTOP) != -1)
                    {
                        int status;
                        pid_t wpid = waitpid(temp_arr[i]->pid, &status, WNOHANG);
                        if (wpid == 0)
                        {
                            printf("\nprocess continued\n");
                            struct timeval end_time;
                            gettimeofday(&end_time, NULL);
                            temp_arr[i]->end = (double)end_time.tv_sec + end_time.tv_usec / 1000000.0;
                            if(temp_arr[i]->priority>1){
                                temp_arr[i]->priority=temp_arr[i]->priority-1;
                            }
                            push(temp_arr[i],q_arr[temp_arr[i]->priority-1]);
                        }
                        else
                        {
                            struct timeval end_time;
                            gettimeofday(&end_time, NULL);
                            temp_arr[i]->end = (double)end_time.tv_sec + end_time.tv_usec / 1000000.0;
                            history_arr[num_commands++] = temp_arr[i];
                            printf("process has ended\n");
                        }
                    }
                    else
                    {
                        printf("\nsomething abnormal hapended\n");
                    }
                }
            }
            // printf("start %d end %d", start, end);
        }
    }

    // prints history and does the cleanup
    history();
    printf("Exited the while loop\n");

    munmap(shm, 1024);
    shm_unlink(name);
    close(fd);

    munmap(shm_count, 24);
    shm_unlink(num_command);
    close(fd_int);

    munmap(shm_p,400);
    shm_unlink(name_p);
    close(fd_p);

    exit(EXIT_SUCCESS);
}
