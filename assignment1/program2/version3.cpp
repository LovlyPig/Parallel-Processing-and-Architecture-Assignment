#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#if __linux__
#include <pthread.h>
#include <sys/time.h>
#endif

volatile bool exit_flag = false;

volatile int count = 0;
pthread_mutex_t mutex, wait_mutex, exit_mutex;
static std::vector<bool> chopsticks(5, true);


void* thinking_and_eating(void *arg) {
    int tid = *(int*)arg;
    while (true) {
        if (exit_flag) {
            break;
        }

        printf("Thread %d is thinking \n", tid);

        printf("Thread %d is now hungry \n", tid);

        pthread_mutex_lock(&wait_mutex);
        while (count == 4) {
            pthread_mutex_unlock(&wait_mutex);
            
            pthread_mutex_lock(&wait_mutex);
        }
        count++;
        pthread_mutex_unlock(&wait_mutex);
        
        pthread_mutex_lock(&mutex);
        while (!chopsticks[tid] || !chopsticks[(tid + 1) % 5]) {
            
            pthread_mutex_unlock(&mutex);
            // busy wait
            pthread_mutex_lock(&mutex);
        }
        chopsticks[tid] = false;
        chopsticks[(tid + 1) % 5] = false;
        pthread_mutex_unlock(&mutex);
        pthread_mutex_lock(&wait_mutex);
        count--;
        pthread_mutex_unlock(&wait_mutex);
        
        printf("Thread %d is now eating \n", tid);

        pthread_mutex_lock(&mutex);
        chopsticks[tid] = true;
        chopsticks[(tid + 1) % 5] = true;
        pthread_mutex_unlock(&mutex);
        
    }

    return NULL;
}

int main() {
    pthread_t threads[5];
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&wait_mutex, NULL);
    pthread_mutex_init(&exit_mutex, NULL);

    for (int i = 0; i < 5; i++) {
        int *ptr = (int *)malloc(sizeof(int));
        *ptr = i;
        int err = pthread_create(&threads[i], NULL, thinking_and_eating, ptr);
        if (err != 0) {
            printf("Can't create thread %d :[%s]\n", i, strerror(err));
            pthread_mutex_destroy(&mutex);
            pthread_mutex_destroy(&wait_mutex);
            pthread_mutex_destroy(&exit_mutex);
            exit(1);
        }
    }

    char input[10];
    while (true) {
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = 0; 

            if (strcmp(input, "n") == 0) {
                pthread_mutex_lock(&exit_mutex);
                exit_flag = true;
                pthread_mutex_unlock(&exit_mutex);
                break;
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&wait_mutex);
    pthread_mutex_destroy(&exit_mutex);

    return 0;
}