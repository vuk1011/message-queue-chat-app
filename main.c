#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>

#define PROC_FILE "processes.txt"
#define MAX_USERS 4

bool running = true;
pid_t pid;
int pids[MAX_USERS] = {0};

void sign_in(pid_t pid);

void *update_pids(void *arg);

void get_pids_from_file(int arr[]);

int main(void) {
    pid = getpid();
    pthread_t listening;

    sign_in(pid);

    pthread_create(&listening, NULL, update_pids, NULL);
    sleep(8);
    running = false;
    pthread_join(listening, NULL);

    for (int i = 0; i < MAX_USERS; ++i) {
        printf("%d\n", pids[i]);
    }

    return 0;
}

void sign_in(pid_t pid) {
    pids[0] = pid;
    int proc_file = open(PROC_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    flock(proc_file, LOCK_EX);
    dprintf(proc_file, "%d\n", pid);
    flock(proc_file, LOCK_UN);
    close(proc_file);
}

void get_pids_from_file(int arr[]) {
    char big_buff[32];
    int i = 0;

    int proc_file = open("processes.txt", O_RDONLY);
    flock(proc_file, LOCK_EX);
    read(proc_file, big_buff, 32);
    flock(proc_file, LOCK_UN);
    close(proc_file);

    char small_buff[8];
    int k = 0;
    for (int j = 0; j < strlen(big_buff); ++j) {
        char ch = big_buff[j];
        if (ch == '\n') {
            small_buff[k] = '\0';
            arr[i++] = atoi(small_buff);
            memset(small_buff, 0, sizeof(small_buff));
            k = 0;
        }
        small_buff[k++] = ch;
    }
}

void *update_pids(void *arg) {
    while (running) {
        sleep(5);
        int arr[MAX_USERS] = {0};
        get_pids_from_file(arr);

        int i = 1;
        for (int j = 0; j < MAX_USERS; ++j) {
            if (arr[j] == pid) {
                continue;
            }
            pids[i++] = arr[j];
        }
    }
    return NULL;
}
