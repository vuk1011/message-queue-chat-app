#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>

#define PROC_FILE "processes.txt"
#define MAX_USERS 4

void sign_in();

int main(void) {
    sign_in();

    return 0;
}

void sign_in() {
    pid_t pid = getpid();
    int proc_file = open(PROC_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    flock(proc_file, LOCK_EX);
    dprintf(proc_file, "%d\n", pid);
    flock(proc_file, LOCK_UN);
    close(proc_file);
}
