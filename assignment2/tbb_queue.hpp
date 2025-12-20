#ifndef TBB_QUEUE_HPP
#define TBB_QUEUE_HPP

#pragma once

#include <tbb/concurrent_queue.h>
#include <stdexcept>

template <typename T>
class TBBQueue {
private:
    tbb::concurrent_queue<T> queue;

public:
    explicit TBBQueue(size_t) {}

    void add(const T &item) {
        queue.push(item);
    }

    T remove() {
        T out;
        while (!queue.try_pop(out)) {
            std::this_thread::yield();
        }
        return out;
    }
};

#endif // TBB_QUEUE_HPP