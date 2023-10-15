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
    char **command;
    pid_t pid;
    bool created;
    double start;
    double end;
    double time;
    int cycles;
    char *cmd;
};

// struct LinkedList PrioritizedReadyQueues[4];
int num_commands = 0;
struct program *arr[100];
struct program *history_arr[100];
int start = 50;
int end = 50;
static volatile int keepRunning = 1;

struct program *pop()
{
    if (end > start)
        return arr[--end];
    else
    {
        printf("Stack is empty\n");
        return NULL;
    }
}

void push(struct program *bottom)
{
    if (end < 100)
    {
        arr[--start] = bottom;
    }
    else
        printf("Array size reached\n");
}

bool isEmpty()
{
    if (end - start == 0)
        return true;
    return false;
}

bool isFull()
{
    if (end == 100)
        return true;
    return false;
}

struct program *top()
{
    return arr[end - 1];
}

void create_process()
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
        char* args[2];
        args[0]=top()->cmd;
        args[1]=NULL;
        execvp(args[0], args);
        perror("Execution failed\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        top()->pid = pid;
    }
}

void resume_process(struct program *p)
{
    kill(p->pid, SIGCONT);
}

int pause_process(struct program *p)
{
    return kill(p->pid, SIGSTOP);
}

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
        printf("Execution time %f\n", total_time);
        printf("Waiting Time %f\n", total_time-history_arr[i]->cycles*TSLICE);
    }
}

// parameter of while loop is set to false and while loop waits for natural termination
void intHandler(int dummy)
{
    keepRunning = 0;
}

void make_struct_and_push(char *command)
{
    // sets the start time, make array for execv and push process in the queue
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    struct program *p1 = (struct program *)malloc(sizeof(struct program));
    char *arr1[2];
    arr1[0] = command;
    arr1[1] = NULL;
    p1->command = arr1;
    p1->cmd=command;
    p1->created = false;
    p1->start = (double)start_time.tv_sec + start_time.tv_usec / 1000000.0;
    push(p1);
}

int main()
{

    // signal handler for control c to end program after natural termination
    signal(SIGINT, intHandler);

    // shared memory string to store all commands
    const char *name = "shared_memory";
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 128);
    char *shm = (char *)mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // shared memory int to store how many commands recieved during the sleep time
    const char *num_command = "num_command";
    int fd_int = shm_open(num_command, O_CREAT | O_RDWR, 0666);
    ftruncate(fd_int, 24);
    int *shm_count = (int *)mmap(NULL, 24, PROT_READ | PROT_WRITE, MAP_SHARED, fd_int, 0);

    // recieving value of NCPU and TSLICE from shell
    NCPU=*(shm_count+1);
    TSLICE=*(shm_count+2);

    struct program *temp_arr[NCPU];
    // while loop which is executed at end of each time slice
    while ((keepRunning == 1) || (start != end) || (count<*shm_count))
    {
        if (shm != NULL)
        {
            printf("count of commands is %d\n",count);
            // checking if new command it sent from shell to scheduler
            while (count < *shm_count)
            {
                make_struct_and_push(shm + buffer);
                buffer += strlen(shm + buffer) + 1;
                count++;
            }
        }

        printf("start %d end %d\n", start, end);

        // counts the number of process in queue
        int count_current = 0;
        // run the processes equal to NCPU
        for (int i = 0; i < NCPU; i++)
        {
            if (!isEmpty())
            {
                count_current++;

                if (top()->created)
                {
                    struct timeval middle;
                    gettimeofday(&middle, NULL);
                    top()->time = middle.tv_sec + middle.tv_usec / 1000000.0 - top()->end;
                    printf("Resumed process\n");
                    resume_process(top());
                }
                else
                {
                    struct timeval middle;
                    gettimeofday(&middle, NULL);
                    top()->time = middle.tv_sec + middle.tv_usec / 1000000.0 - top()->start;
                    top()->created = true;
                    create_process(top());
                }
                top()->cycles++;

                temp_arr[i] = pop();
            }
        }
        // scheduler goes to sleep for TSLICE time
        printf("gone to sleep\n");
        sleep(TSLICE);
        printf("exited the sleep\n");
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
                        push(temp_arr[i]);
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
        printf("start %d end %d", start, end);
    }

    // prints history and does the cleanup
    history();
    printf("Exited the while loop\n");

    munmap(shm, 128);
    shm_unlink(name);
    close(fd);

    munmap(shm_count, 24);
    shm_unlink(num_command);
    close(fd_int);

    exit(EXIT_SUCCESS);
}
