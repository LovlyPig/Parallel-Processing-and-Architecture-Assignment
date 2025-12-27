#include <threads.h>
#include <iostream>
#include <vector>
#include <thread>

#include "lock_free_list.h"

struct Args {
    LockFreeList<int> *list;
    int value;
    bool flag;
};

static void add(void *obj) {
    Args *a = (Args *)obj;
    LockFreeList<int> *list =a->list;
    a->flag = list->add(a->value);
}

static void remove_(void *obj) {
    Args *a = (Args *)obj;
    LockFreeList<int> *list =a->list;
    a->flag = list->remove(a->value);
}

static void contains(void *obj) {
    Args *a = (Args *)obj;
    LockFreeList<int> *list =a->list;
    a->flag = list->contains(a->value);
}

int user_main(int argc, char **argv) {
    LockFreeList<int> list;
    const int NUM_THREADS = 10;
    thrd_t threads[NUM_THREADS];
    Args args[NUM_THREADS]; 
    const std::vector<int> values = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].list = &list;
        args[i].value = values[i];
        args[i].flag = false;
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        thrd_create(&threads[i], (thrd_start_t)&add, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i+=2) {
        thrd_create(&threads[i], (thrd_start_t)&remove_, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        thrd_join(threads[i]);
        if (!args[i].flag) {
            std::cout << "Add/Remove operation failed for value: " << args[i].value << std::endl;
            exit(1);
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        thrd_create(&threads[i], (thrd_start_t)&contains, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        thrd_join(threads[i]);
        bool expected = (i % 2 != 0); 
        if (args[i].flag != expected) {
            std::cout << "Contains operation failed for value: " << args[i].value << std::endl;
            exit(1);
        }
    }

    return 0;
}