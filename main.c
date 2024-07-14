#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

char username[USERNAME_LEN];
pthread_mutex_t mutex;

int main(int argc, char *argv[]) {
    if (argc == 2 && strlen(argv[1]) <= USERNAME_LEN) {
        strcpy(username, argv[1]);
    } else {
        printf("Error: program should be started with the username (length <= 16) as a command line argument\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Error: initializing mutex\n");
        exit(EXIT_FAILURE);
    }

    initialize();
    open_queues();

    pthread_t sending, receiving;
    if (pthread_create(&sending, NULL, send, NULL) != 0) {
        printf("Error: creating the sending thread\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&receiving, NULL, receive, NULL) != 0) {
        printf("Error: creating the receiving thread\n");
        pthread_cancel(sending);
        exit(EXIT_FAILURE);
    }
    if (pthread_join(sending, NULL) != 0) {
        printf("Error: joining the sending thread\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(receiving, NULL) != 0) {
        printf("Error: joining the receiving thread\n");
        exit(EXIT_FAILURE);
    }

    cleanup();
    if (pthread_mutex_destroy(&mutex) != 0) {
        printf("Error: destroying mutex\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}
