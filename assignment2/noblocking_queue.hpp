#ifndef NOBLOCKING_QUEUE_HPP
#define NOBLOCKING_QUEUE_HPP

#pragma once

#include <vector>
#include <atomic>
#include <stdexcept>
#include <cstdint>
#include <cassert>
#include <thread>
#include <chrono>

template <typename T>
class NonBlockingQueue {
private:
    struct Node {
        std::atomic<size_t> seq;
        T data;
    };

    size_t capacity;
    std::vector<Node> buffer;
    std::atomic<size_t> head;
    std::atomic<size_t> tail;

    // 退避 让步
    void backoff() {
        static thread_local int spins = 0;
        if (++spins < 16) {
            for (volatile int i = 0; i < 30; ++i) {
                asm volatile("":::"memory");
            }
        } else {
            std::this_thread::yield();
            if (spins > 1024) spins = 0;
        }
    }

public:
    explicit NonBlockingQueue(size_t capacity)
        : capacity(capacity), buffer(capacity), head(0), tail(0) {
        
        for (size_t i = 0; i < capacity; ++i) {
            buffer[i].seq.store(i, std::memory_order_relaxed);
        }
    }

    void add(const T &item) {
        size_t pos;
        while (true) {
            pos = tail.load(std::memory_order_relaxed);
            Node &node = buffer[pos % capacity];
            size_t seq = node.seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)pos;

            if (dif == 0) {
                if (tail.compare_exchange_weak(pos, pos+1, std::memory_order_relaxed)) {
                    node.data = item;
                    node.seq.store(pos + 1, std::memory_order_release);
                    return;
                }
            } else if (dif < 0) {
                // full
                backoff();
            } else {}
        }
    }

    T remove() {
        size_t pos;
        while (true) {
            pos = head.load(std::memory_order_relaxed);
            Node &node = buffer[pos % capacity];
            size_t seq = node.seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)(pos+1);
            if (dif == 0) {
                if (head.compare_exchange_weak(pos, pos+1, std::memory_order_relaxed)) {
                    T out = node.data;
                    node.seq.store(pos + capacity, std::memory_order_release);
                    return out;
                }
            } else if (dif < 0) {
                // empty
                backoff();
            } else {}
        }
    }
};

#endif // NOBLOCKING_QUEUE_HPP
