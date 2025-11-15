#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "../lib/game.h"

sem_t semaphore;

void* foo(void* arg) {
    sem_wait(&semaphore);

    sleep(1);
    printf("Hello from thread!\n");
    printf("arg: %s\n", (char*)arg);

    sem_post(&semaphore);
    return NULL;
}

int main() {
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;
    pthread_t thread4;

    sem_init(&semaphore, 0, 1);

    pthread_create(&thread1, NULL, foo, "Thread Argument 1");
    pthread_create(&thread2, NULL, foo, "Thread Argument 2");
    pthread_create(&thread3, NULL, foo, "Thread Argument 3");
    pthread_create(&thread4, NULL, foo, "Thread Argument 4");

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    return 0;
}
