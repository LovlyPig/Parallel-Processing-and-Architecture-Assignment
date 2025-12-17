#include <iostream>
#include <vector>
#include <queue>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cmath>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/spin_mutex.h>
#include <tbb/combinable.h>
#include <tbb/global_control.h>
#include <tbb/task_arena.h>

#if __linux__
#include <pthread.h>
#include <sys/time.h>
#endif

static std::vector<uint32_t> prime_up_to_1e4;

// 埃式筛
void init_prime() {
    const uint32_t limit = 10000;
    std::vector<bool> is_prime(limit + 1, true);
    is_prime[0] = is_prime[1] = false;

    for (uint32_t i = 2; i <= limit; ++i) {
        if (is_prime[i]) {
            prime_up_to_1e4.push_back(i);
            for (uint32_t j = i * i; j <= limit; j += i) {
                is_prime[j] = false;
            }
        }
    }
}

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

double find_primes_tbb(std::ofstream &outfile) {

    std::vector<bool> is_prime(100000001, true);
    is_prime[0] = is_prime[1] = false;

    tbb::parallel_for(0, 1, [](int) {});
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism, 8);
    double start_time = get_time_sec();
    tbb::parallel_for(tbb::blocked_range<int>(2, 100000001), [&] (auto& r) {
        // int local_count = 0;
        // uint64_t local_sum = 0;
        // std::vector<uint32_t> local_primes;

        // int tid = tbb::this_task_arena::current_thread_index();
        // printf("Thread %d processing range [%d, %d)\n", tid, r.begin(), r.end());
        for (uint32_t p : prime_up_to_1e4) {
            int start = std::max(p * p, ((r.begin() + p - 1) / p) * p);

            for (int j = start; j < r.end(); j += p) {
                is_prime[j] = false;
            }
        }
    });
    double end_time = get_time_sec();

    int prime_count = 0;
    uint64_t prime_sum = 0;
    std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> top_primes;
    for (uint32_t i = 2; i <= 100000000; ++i) {
        if (is_prime[i]) {
            prime_count++;
            prime_sum += i;
            top_primes.push(i);
            if (top_primes.size() > 10) {
                top_primes.pop();
            }
        }
    }

    outfile << "Execution Time: " << (end_time - start_time) << " seconds\n";
    outfile << "Total Primes Found: " << prime_count << "\n";
    outfile << "Sum of All Primes: " << prime_sum << "\n";
    outfile << "Top 10 Largest Primes: ";
    while (!top_primes.empty()) {
        outfile << top_primes.top() << ", ";
        top_primes.pop();
    }
    outfile << "\n";

    return end_time - start_time;
}

double find_primes_single_thread(std::ofstream &outfile) {

    std::vector<bool> is_prime(100000001, true);
    is_prime[0] = is_prime[1] = false;

    double start_time = get_time_sec();
    for (uint32_t p : prime_up_to_1e4) {
        for (uint32_t j = p * p; j <= 100000000; j += p) {
            is_prime[j] = false;
        }
    }
    double end_time = get_time_sec();

    int prime_count = 0;
    uint64_t prime_sum = 0;
    std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> top_primes;
    for (uint32_t i = 2; i <= 100000000; ++i) {
        if (is_prime[i]) {
            prime_count++;
            prime_sum += i;
            top_primes.push(i);
            if (top_primes.size() > 10) {
                top_primes.pop();
            }
        }
    }
    outfile << "Execution Time: " << (end_time - start_time) << " seconds\n";
    outfile << "Total Primes Found: " << prime_count << "\n";
    outfile << "Sum of All Primes: " << prime_sum << "\n";
    outfile << "Top 10 Largest Primes: ";
    while (!top_primes.empty()) {
        outfile << top_primes.top() << ", ";
        top_primes.pop();
    }
    outfile << "\n\n";

    return end_time - start_time;
}   


int main(int argc, char* argv[]) {

    init_prime();
    
    std::ofstream outfile;
    outfile.open("primes.txt", std::ios::out | std::ios::trunc);
    if (!outfile.is_open()) {
        std::cout << "Error opening  for writing.\n" << std::endl;
        outfile.close();
        exit(1);
    }

    outfile << "Find Primes Single Threaded Execution:\n";
    double time_single = find_primes_single_thread(outfile);

    outfile << "Find Primes TBB Parallel Execution:\n";
    double time_tbb = find_primes_tbb(outfile);

    std::cout << "TBB Speedup: " << time_single / time_tbb << "x\n";
    outfile.close();
    return 0;
}