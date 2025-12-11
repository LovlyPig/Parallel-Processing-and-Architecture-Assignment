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


#if __linux__
#include <pthread.h>
#include <sys/time.h>
#endif

static std::vector<uint32_t> prime_up_to_1e8;

// 埃式筛
void init_prime() {
    const uint32_t limit = 10000;
    std::vector<bool> is_prime(limit + 1, true);
    is_prime[0] = is_prime[1] = false;

    for (uint32_t i = 2; i <= limit; ++i) {
        if (is_prime[i]) {
            prime_up_to_1e8.push_back(i);
            for (uint32_t j = i * i; j <= limit; j += i) {
                is_prime[j] = false;
            }
        }
    }
}

bool is_prime(uint32_t n) {
    if (n <= 1) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;

    uint32_t sqrt_limit = sqrtf((float)n);
    for (uint32_t p : prime_up_to_1e8) {
        if (p > sqrt_limit) break;
        if (n % p == 0) return false;
    }
    return true;
} 

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

double find_primes_tbb(std::ofstream &outfile) {

    std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> top_primes;

    tbb::atomic<int> prime_count(0);
    tbb::atomic<uint64_t> prime_sum(0);
    tbb::spin_mutex mtx;

    double start_time = get_time_sec();
    tbb::parallel_for(tbb::blocked_range<int>(0, 8), [&] (const tbb::blocked_range<int>& r) {
        int local_count = 0;
        uint64_t local_sum = 0;

        for (int tid = r.begin(); tid != r.end(); ++tid) {
            for (uint32_t i = (tid == 0 ? 2 : tid); i <= 100000000; i += 8) {
                if (is_prime(i)) {
                    local_count++;
                    local_sum += i;

                    tbb::spin_mutex::scoped_lock lock(mtx);
                    if (top_primes.size() < 10) {
                        top_primes.push(i);
                    } else {
                        top_primes.pop();
                        top_primes.push(i);
                    }
                }
            }
        }

        prime_count += local_count;
        prime_sum += local_sum;
    }, tbb::simple_partitioner());
    double end_time = get_time_sec();

    outfile << "Execution Time: " << (end_time - start_time) << " seconds\n";
    outfile << "Total Primes Found: " << prime_count << "\n";
    outfile << "Sum of All Primes: " << prime_sum << "\n";
    outfile << "Top 10 Largest Primes: ";
    std::vector<uint32_t> largest_primes;
    while (!top_primes.empty()) {
        outfile << top_primes.top() << ", ";
        top_primes.pop();
    }
    outfile << "\n";

    return end_time - start_time;

}

double find_primes_single_thread(std::ofstream &outfile) {
    
    int prime_count = 0;
    uint64_t prime_sum = 0;
    // 小顶堆
    std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> top_primes;
    
    double start_time = get_time_sec();
    for (uint32_t i = 2; i <= 100000000; ++i) {
        if (is_prime(i)) {
            // outfile << i << "\n";
            prime_count++;
            prime_sum += i;
            if (prime_count < 10) {
                top_primes.push(i);
            } else {
                top_primes.pop();
                top_primes.push(i);
            }
        }
    }
    double end_time = get_time_sec();

    outfile << "Execution Time: " << (end_time - start_time) << " seconds\n";
    outfile << "Total Primes Found: " << prime_count << "\n";
    outfile << "Sum of All Primes: " << prime_sum << "\n";
    outfile << "Top 10 Largest Primes: ";
    std::vector<uint32_t> largest_primes;
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

    outfile << "TBB Speedup: " << time_single / time_tbb << "x\n";
    outfile.close();
    return 0;
}