#ifndef CHAT_APP_UTILS_H
#define CHAT_APP_UTILS_H

#define PROC_FILE "processes.txt"
#define CLEAR_SCREEN "\033[H\033[J"
#define TIMEOUT 3
#define MAX_USERS 4
#define USERNAME_LEN 16
#define QUEUE_NAME_SIZE 16
#define MSG_SIZE 256
#define CHAT_SIZE 1024

void sign_in(pid_t pid);

void get_pids_from_file(int arr[]);

void update_pids();

void initialize();

void open_queues();

void *send(void *arg);

void *receive(void *arg);

void screen_add(char *text);

void screen_refresh();

void deallocate();

void cleanup();

#endif //CHAT_APP_UTILS_H
