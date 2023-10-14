#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>

int num_commands = 0;
int buffer=0;
char **input_arr[100];



// activated on control c waits for scheduler to end and then performs cleanup
static void my_handler(int signum)
{
    if (signum == SIGINT)
    {
        printf("\n");
    }
    wait(NULL);
    exit(0);
}

// take space seprated string as input 
// retruns 2d array ended by null for execv
char **tokenise(char **param_array, char *command)
{
    int c = 0;
    char *comm_copy = strdup(command);
    if (comm_copy == NULL)
    {
        printf("Error allocating memory\n");
        exit(EXIT_FAILURE);
    }

    char *tok = strtok(comm_copy, " \n");
    param_array[c++] = tok;

    while (tok)
    {
        tok = strtok(NULL, " \n");
        param_array[c++] = tok;
        if (c == 64)
            break;
    }
    if (c > 64)
    {
        param_array = realloc(param_array, sizeof(char *) * 128);
        while (tok)
        {
            tok = strtok(NULL, " \n");
            param_array[c++] = tok;
        }
    }
    if (c > 128)
    {
        printf("Limit exceeded\n");
        exit(EXIT_FAILURE);
    }

    return param_array;
}

// takes the command as input to parse it to pass it to appropriate function
int launch(char *command,char*shm)
{
    char **param_array = (char **)malloc(64 * sizeof(char *));
    if (param_array == NULL)
    {
        printf("Error allocating memory\n");
        exit(EXIT_FAILURE);
    }
    param_array = tokenise(param_array, command);
    if (strcmp(param_array[0], "submit") == 0)
    {
        strcpy(shm+buffer,param_array[1]);
        buffer+=strlen(param_array[1])+1;
        return 0;
    }
    else
    {
        printf("Invalid input try again\n");
    }
}


// reads the user input and return the string in which it is stored
char *read_user_input()
{
    char *reader = NULL;
    size_t size = 0;
    if (getline(&reader, &size, stdin) == -1)
    {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
    return reader;
}


// infinite loop to take input from user and pass it to the scheduler
void shell_loop(char*shm,int*shm_count)
{
    int status;
    do
    {
        printf("user@LAPTOP:> ");
        char *command = read_user_input();
        status = launch(command,shm);
        num_commands++;
        *shm_count=num_commands;
    } while (status != -1);
}

int main(void)
{
    // signal handler for control c
    signal(SIGINT, my_handler);

    // creating shared memory
    const char* name = "shared_memory";
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 128);
    char* shm = (char*)mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    const char* num_command="num_command";
    int fd_int = shm_open(num_command, O_CREAT | O_RDWR, 0666);
    ftruncate(fd_int,8);
    int* shm_count=(int*)mmap(NULL,8,PROT_READ| PROT_WRITE,MAP_SHARED,fd_int,0);
    *shm_count=0;

    // forking parent process to run scheduler as the child process
    if(fork()==0){
        char*arg[2];
        arg[0]="./sch";
        arg[1]=NULL;
        execv(arg[0],arg);
    }
    else{
        shell_loop(shm,shm_count);
    }

    // cleanup of the shared memory
    munmap(shm, 128);
    shm_unlink(name);
    close(fd);

    munmap(shm_count,128);
    shm_unlink(num_command);
    close(fd_int);
    
    return 0;
}
