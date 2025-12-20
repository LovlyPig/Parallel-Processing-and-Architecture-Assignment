#ifndef BLOCKING_QUEUE_HPP
#define BLOCKING_QUEUE_HPP

#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stdexcept>

template <typename T>
class BlockingQueue {
private:
    size_t capacity;
    std::vector<T> buffer;
    size_t head;
    size_t tail;
    std::atomic<size_t> size;
    std::mutex head_mutex;
    std::mutex tail_mutex;
    std::condition_variable not_empty;
    std::condition_variable not_full;

public:
    explicit BlockingQueue(size_t capacity)
        : capacity(capacity), buffer(capacity), head(0), tail(0), size(0) {}
    
    void add(const T &item) {
        std::unique_lock<std::mutex> tail_lock(tail_mutex);
        not_full.wait(tail_lock, [this]() {
            return size.load(std::memory_order_acquire) < capacity;
        });

        buffer[tail] = item;
        tail = (tail + 1) % capacity;
        size.fetch_add(1, std::memory_order_release);

        not_empty.notify_one();
    }   

    T remove() {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        not_empty.wait(head_lock, [this]() {
            return size.load(std::memory_order_acquire) > 0;    
        });

        T out = buffer[head];
        head = (head + 1) % capacity;
        size.fetch_sub(1, std::memory_order_release);

        not_full.notify_one();
        return out;
    }
};

#endif // BLOCKING_QUEUE_HPP