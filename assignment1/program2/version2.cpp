#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#if __linux__
#include <pthread.h>
#include <sys/time.h>
#endif

pthread_mutex_t mutex;
static std::vector<bool> chopsticks(5, true);

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void* thinking_and_eating(void *arg) {
    int tid = *(int*)arg;
    double start_time, end_time;
    while (true) {
        printf("Thread %d is thinking \n", tid);
        
        printf("Thread %d is now hungry \n", tid);
        start_time = get_time_sec();

        pthread_mutex_lock(&mutex);
        while (!chopsticks[tid] || !chopsticks[(tid + 1) % 5]) {
            pthread_mutex_unlock(&mutex);

            end_time = get_time_sec();
            if (end_time - start_time > 1.0) {
                printf("\nThread %d is starving!\n", tid);
                exit(1);
            }
            
            pthread_mutex_lock(&mutex);
        }

        chopsticks[tid] = false;
        chopsticks[(tid + 1) % 5] = false;
        pthread_mutex_unlock(&mutex);
        
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

    for (int i = 0; i < 5; i++) {
        int *ptr = (int *)malloc(sizeof(int));
        *ptr = i;
        int err = pthread_create(&threads[i], NULL, thinking_and_eating, ptr);
        if (err != 0) {
            printf("Can't create thread %d :[%s]\n", i, strerror(err));
            pthread_mutex_destroy(&mutex);
            exit(1);
        }
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);

    return 0;
}