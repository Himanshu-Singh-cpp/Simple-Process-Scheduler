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

int dummy_main(int argc, char **argv);
void intHandler(int dummy)
{
}
int main(int argc, char **argv)
{
    /* You can add any code here you want to support your SimpleScheduler implementation*/
    signal(SIGINT, intHandler);
    int ret = dummy_main(argc, argv);
    return ret;
}
#define main dummy_main
