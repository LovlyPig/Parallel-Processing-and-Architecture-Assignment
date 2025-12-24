#ifndef RANDOM_BITS_HPP
#define RANDOM_BITS_HPP

#pragma once

#include <random>
#include <cstdint>
#include <vector>

static inline std::vector<uint8_t> generate_random_bits(size_t total_ops, double ratio) {
    std::vector<uint8_t> v;
    v.reserve(total_ops);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution d(ratio);

    for (size_t i = 0; i < total_ops; ++i) {
        v.push_back(d(gen) ? 1 : 0);
    }
    return v;
}

static inline bool check_bits(std::vector<uint8_t>& bits) {
    int balance_count = 0;
    for (auto b : bits) {
        if (balance_count == 0 && b == 0) return false;
        if (b == 1) balance_count++;
        else balance_count--;
    }

    return true;
}


#endif