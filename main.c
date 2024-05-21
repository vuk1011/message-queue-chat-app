#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>

int main(void) {
    // Append the process ID to a file
    pid_t pid = getpid();
    FILE *file = fopen("processes.txt", "a");
    fprintf(file, "%d\n", pid);
    fclose(file);

    return 0;
}
