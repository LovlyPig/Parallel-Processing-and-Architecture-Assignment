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

pthread_mutex_t mutex[5];
pthread_mutex_t wait_mutex;
volatile int count = 0;

void* thinking_and_eating(void *arg) {
    int tid = *(int*)arg;
    while (true) {
        printf("Thread %d is thinking \n", tid);

        if (count == 5) {
            printf("\nDeadLock!\n");
            exit(1);
        }
        printf("Thread %d is now hungry \n", tid);
        pthread_mutex_lock(&mutex[tid]);
        pthread_mutex_lock(&wait_mutex);
        count++;
        if (count == 5) {
            printf("\nDeadLock!\n");
            for (int i = 0; i < 5; i++) {
                pthread_mutex_destroy(&mutex[i]);
            }
            exit(1);
        }
        pthread_mutex_unlock(&wait_mutex);
        pthread_mutex_lock(&mutex[(tid + 1) % 5]);

        printf("Thread %d is now eating \n", tid);

        pthread_mutex_unlock(&mutex[tid]);
        pthread_mutex_lock(&wait_mutex);
        count--;
        pthread_mutex_unlock(&wait_mutex);
        pthread_mutex_unlock(&mutex[(tid + 1) % 5]);
        
    }

    return NULL;
}

int main() {
    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        pthread_mutex_init(&mutex[i], NULL);
    }
    pthread_mutex_init(&wait_mutex, NULL);

    for (int i = 0; i < 5; i++) {
        int *ptr = (int *)malloc(sizeof(int));
        *ptr = i;
        int err = pthread_create(&threads[i], NULL, thinking_and_eating, ptr);
        if (err != 0) {
            printf("Can't create thread %d :[%s]\n", i, strerror(err));
            for (int i = 0; i < 5; i++) {
                pthread_mutex_destroy(&mutex[i]);
            }
            exit(1);
        }
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < 5; i++) {
        pthread_mutex_destroy(&mutex[i]);
    }
    pthread_mutex_destroy(&wait_mutex);

    return 0;
}