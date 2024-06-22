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
#define CLEAR_SCREEN "\033[H\033[J"
#define MAX_USERS 4
#define TIMEOUT 5
#define NAME_SIZE 16
#define CHAT_SIZE 1024
#define MSG_SIZE 256

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

pthread_mutex_t mutex;

void sign_in(pid_t pid);

void update_pids();

void get_pids_from_file(int arr[]);

void initialize();

void open_queues();

void cleanup();

void *send(void *arg);

void *receive(void *arg);

void screen_add(char *text);

void screen_refresh();

int main(void) {
    pthread_mutex_init(&mutex, NULL);
    initialize();
    open_queues();

    pthread_t sending, receiving;
    pthread_create(&sending, NULL, send, NULL);
    pthread_create(&receiving, NULL, receive, NULL);
    pthread_join(sending, NULL);
    pthread_join(receiving, NULL);

    cleanup();
    pthread_mutex_destroy(&mutex);
    return 0;
}

void sign_in(pid_t pid) {
    // Writes the process's ID to the processes.txt file
    int proc_file = open(PROC_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    flock(proc_file, LOCK_EX);
    dprintf(proc_file, "%d\n", pid);
    flock(proc_file, LOCK_UN);
    close(proc_file);
}

void get_pids_from_file(int arr[]) {
    // Writes pid's from processes.txt to the given array
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
        printf("Error: Chat room is full.\n");
        exit(0);
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
        queue_names[i] = (char *) malloc(NAME_SIZE * sizeof(char));
        if (pids[i] == 0) continue;
        snprintf(queue_names[i], NAME_SIZE, "/%d", pids[i]);
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
        if (i == 0) {   // Set up the process's receiving queue
            queues[i] = mq_open(queue_names[i], O_RDONLY | O_CREAT | O_NONBLOCK, 0644, &attr);
        } else {    // Set up the queues for sending messages
            queues[i] = mq_open(queue_names[i], O_WRONLY | O_CREAT | O_NONBLOCK, 0644, &attr);
        }
    }
}

void cleanup() {
    // Frees allocated memory, closes mq descriptors and removes their names
    for (int i = 0; i < MAX_USERS; ++i) {
        free(queue_names[i]);
        mq_close(queues[i]);
        mq_unlink(queue_names[i]);
    }
    // Delete content of processes.txt if process is the last to leave
    if (others == 0) {
        int proc_file = open(PROC_FILE, O_WRONLY | O_TRUNC);
        close(proc_file);
    }
}

void *send(void *arg) {
    while (running) {
        sleep(1);
        char msg[MSG_SIZE];
        fgets(msg, MSG_SIZE, stdin);
        pthread_mutex_lock(&mutex);
        screen_add("Send: ");
        screen_add(msg);
        screen_refresh();
        pthread_mutex_unlock(&mutex);
        for (int i = 1; i < MAX_USERS; ++i) {
            if (pids[i] == 0) break;
            mq_send(queues[i], msg, MSG_SIZE, 0);
        }
        if (strcmp(msg, "exit\n") == 0) {
            running = false;
            break;
        }
    }
    return NULL;
}

void *receive(void *arg) {
    while (running) {
        sleep(1);
        char msg[MSG_SIZE];
        if (mq_receive(queues[0], msg, MSG_SIZE, 0) == -1) continue;
        pthread_mutex_lock(&mutex);
        screen_add("Received: ");
        screen_add(msg);
        screen_refresh();
        pthread_mutex_unlock(&mutex);
        if (strcmp(msg, "exit\n") == 0) {
            others--;
        }
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
    printf("Send: ");
}
