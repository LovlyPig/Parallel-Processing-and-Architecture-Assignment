#include <iostream>
#include <vector>
#include <queue>
#include <cstdio>
#include <cstdlib>
#include <fstream>


#if __linux__
#include <pthread.h>
#include <sys/time.h>
#endif

bool is_prime(uint32_t n) {
    if (n <= 1) return false;

    for (uint32_t i = 2; i*i <= n; ++i) {
        if (n % i == 0) return false;
    }

    return true;
}

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void find_primes_single_thread(std::ofstream &outfile) {
    double start_time = get_time_sec();
    int prime_count = 0;
    uint64_t prime_sum = 0;
    // 小顶堆
    std::priority_queue<uint32_t, std::vector<uint32_t, std::greater<uint32_t>>> top_primes;
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
    while (!top_primes.empty()) {
        outfile << top_primes.top() << ", ";
        top_primes.pop();
    }
    outfile << "\n";
}   


int main(int argc, char* argv[]) {
    
    std::ofstream outfile;
    outfile.open("primes.txt", std::ios::out | std::ios::trunc);
    if (!outfile.is_open()) {
        std::cout << "Error opening  for writing.\n" << std::endl;
        outfile.close();
        exit(1);
    }

    outfile << "Find Primes Single Threaded Execution:\n";
    find_primes_single_thread(outfile);

    outfile.close();
    return 0;
}