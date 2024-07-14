#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <stdbool.h>
#include <string.h>
#include "utils.h"

bool running = true;
pid_t pid;
int pids[MAX_USERS] = {0};
char *queue_names[MAX_USERS];
mqd_t queues[MAX_USERS];
int others = 0;
struct screen {
    char content[CHAT_SIZE];
    int i;
} screen;
struct mq_attr attr;
extern char username[USERNAME_LEN];
extern pthread_mutex_t mutex;

void sign_in(pid_t pid) {
    // Writes the process's ID to the processes.txt file
    int proc_file = open(PROC_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (proc_file == -1) {
        printf("Error: opening file %s\n", PROC_FILE);
        exit(EXIT_FAILURE);
    }
    if (flock(proc_file, LOCK_EX) == -1) {
        printf("Error: locking file %s\n", PROC_FILE);
        close(proc_file);
        exit(EXIT_FAILURE);
    }
    dprintf(proc_file, "%d\n", pid);
    if (flock(proc_file, LOCK_UN) == -1) {
        printf("Error: unlocking file %s\n", PROC_FILE);
        close(proc_file);
        exit(EXIT_FAILURE);
    }
    if (close(proc_file) == -1) {
        printf("Error: closing file %s\n", PROC_FILE);
        exit(EXIT_FAILURE);
    }
}

void get_pids_from_file(int arr[]) {
    // Writes pid's from processes.txt to the given array
    char big_buff[32];

    int proc_file = open("processes.txt", O_RDONLY);
    if (proc_file == -1) {
        printf("Error: opening file %s\n", PROC_FILE);
        exit(EXIT_FAILURE);
    }
    if (flock(proc_file, LOCK_EX) == -1) {
        printf("Error: locking file %s\n", PROC_FILE);
        close(proc_file);
        exit(EXIT_FAILURE);
    }
    if (read(proc_file, big_buff, 32) == -1) {
        printf("Error: reading file %s\n", PROC_FILE);
        close(proc_file);
        exit(EXIT_FAILURE);
    }
    if (flock(proc_file, LOCK_UN) == -1) {
        printf("Error: unlocking file %s\n", PROC_FILE);
        close(proc_file);
        exit(EXIT_FAILURE);
    }
    if (close(proc_file) == -1) {
        printf("Error: closing file %s\n", PROC_FILE);
        exit(EXIT_FAILURE);
    }

    char small_buff[8];
    int i = 0;  // iterates over the array storing pids which is written to
    int k = 0;  // iterates over the smaller reading buffer as it stores line by line of text
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

void update_pids() {
    // Reads the pid's from processes.txt and updates the pids[] array accordingly
    sleep(TIMEOUT);
    int arr[MAX_USERS] = {0};
    get_pids_from_file(arr);

    int i = 1;
    for (int j = 0; j < MAX_USERS; ++j) {
        if (arr[j] == pid) {
            continue;
        } else if (arr[j] != 0) {
            pids[i++] = arr[j];
            others++;
        }
    }

    if (others == MAX_USERS) {
        printf("Error: Chat room is full\n");
        exit(EXIT_SUCCESS);
    }
}

void initialize() {
    printf(CLEAR_SCREEN);
    pid = getpid();
    pids[0] = pid;
    sign_in(pid);
    update_pids();

    // Allocate space for mq names and assign them to '/pid'
    for (int i = 0; i < MAX_USERS; ++i) {
        if (pids[i] == 0) break;
        queue_names[i] = (char *) malloc(QUEUE_NAME_SIZE * sizeof(char));
        if (queue_names[i] == NULL) {
            printf("Error: allocating memory for message queue names\n");
            for (int j = 0; j < i; ++j) {
                free(queue_names[j]);
            }
            exit(EXIT_FAILURE);
        }
        snprintf(queue_names[i], QUEUE_NAME_SIZE, "/%d", pids[i]);
    }

    // Initialize the mq_attr struct
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_maxmsg = 8;
    attr.mq_flags = 0;
    attr.mq_curmsgs = 0;
}

void open_queues() {
    for (int i = 0; i < MAX_USERS; ++i) {
        if (pids[i] == 0) return;
        mqd_t res;
        if (i == 0) {   // Set up the process's receiving queue
            res = mq_open(queue_names[i], O_RDONLY | O_CREAT | O_NONBLOCK, 0644, &attr);
        } else {        // Set up the queues for sending messages
            res = mq_open(queue_names[i], O_WRONLY | O_CREAT | O_NONBLOCK, 0644, &attr);
        }
        if (res == (mqd_t) - 1) {
            printf("Error: opening a message queue\n");
            deallocate();
            exit(EXIT_FAILURE);
        }
        queues[i] = res;
    }
}

void *send(void *arg) {
    while (running) {
        sleep(1);

        char msg[MSG_SIZE];
        memset(msg, 0, MSG_SIZE);
        strcat(msg, username);
        strcat(msg, ": ");

        char input[MSG_SIZE - strlen(username) - 2];  // actual message size is smaller because of sender's username
        fgets(input, MSG_SIZE - strlen(username) - 2, stdin);
        strcat(msg, input);

        // exit message
        if (strcmp(input, "exit\n") == 0) {
            for (int i = 1; i < MAX_USERS; ++i) {
                if (pids[i] == 0) break;
                mq_send(queues[i], input, MSG_SIZE, 0);
            }
            running = false;
            break;
        }

        // regular message
        pthread_mutex_lock(&mutex);
        screen_add("You: ");
        screen_add(input);
        screen_refresh();
        pthread_mutex_unlock(&mutex);
        for (int i = 1; i < MAX_USERS; ++i) {
            if (pids[i] == 0) break;
            mq_send(queues[i], msg, MSG_SIZE, 0);
        }
    }
    return NULL;
}

void *receive(void *arg) {
    while (running) {
        sleep(1);

        char msg[MSG_SIZE];
        memset(msg, 0, MSG_SIZE);
        if (mq_receive(queues[0], msg, MSG_SIZE, 0) == -1) continue;

        // exit message
        if (strcmp(msg, "exit\n") == 0) {
            others--;
            continue;
        }

        // regular message
        pthread_mutex_lock(&mutex);
        screen_add(msg);
        screen_refresh();
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void screen_add(char *text) {
    for (int j = 0; j < strlen(text); ++j) {
        screen.content[screen.i++] = text[j];
    }
}

void screen_refresh() {
    printf(CLEAR_SCREEN);
    printf("%s", screen.content);
}

void deallocate() {
    for (int i = 0; i < MAX_USERS; ++i) {
        if (pids[i] == 0) break;
        free(queue_names[i]);
    }
}

void cleanup() {
    // Frees allocated memory, closes mq descriptors and removes their names
    for (int i = 0; i < MAX_USERS; ++i) {
        if (pids[i] == 0) break;
        mq_close(queues[i]);
        mq_unlink(queue_names[i]);
        free(queue_names[i]);
    }

    // Delete content of processes.txt if process is the last to leave
    if (others == 0) {
        int proc_file = open(PROC_FILE, O_WRONLY | O_TRUNC);
        close(proc_file);
    }
}
