#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <atomic>
#include <system_error>
#include <cstring>
#include <exception>

#include "blocking_queue.hpp"
#include "noblocking_queue.hpp"
#include "tbb_queue.hpp"

#include <cstdint>
#include <random>

static inline std::vector<uint8_t> generate_ops_patterned(size_t total_ops) {
    std::vector<uint8_t> v;
    v.reserve(total_ops);
    // 定义模式：pair<count, op>，op: 1=enq, 0=deq
    const std::vector<std::pair<size_t,uint8_t>> pattern = {
        {1024, 1}, // 1000 enq
        {512,  0}, // 500 deq
        {1024,  1}, // 500 enq
        {1536, 0}  // 1000 deq
    };
    while (v.size() < total_ops) {
        for (const auto &p : pattern) {
            size_t cnt = p.first;
            uint8_t op = p.second;
            for (size_t i = 0; i < cnt && v.size() < total_ops; ++i) {
                v.push_back(op);
            }
            if (v.size() >= total_ops) break;
        }
    }
    return v;
}

const size_t TOTAL_OPERATIONS = 1<<20; 
const size_t ITEM_OPERATIONS = 2048;
const size_t COUNTS = TOTAL_OPERATIONS / ITEM_OPERATIONS;
const size_t QUEUE_CAPACITY = 1 << 20; // 队列容量
const std::vector<int> THREAD_COUNTS = [](){
    std::vector<int> v;
    for (int i = 1; i <= 32; i*=2) v.push_back(i);
    return v;
}();

template<typename QueueType>
double run_benchmark_queue(size_t threads) {
    // create queue
    QueueType queue(QUEUE_CAPACITY);

    // Generate global operations using the patterned generator to avoid随机短期不平衡
    std::vector<uint8_t> global_ops = generate_ops_patterned(ITEM_OPERATIONS);

    // per-thread operations count (varies slightly when remainder exists)
    size_t ops_per_thread = COUNTS / threads;

    // For preallocation of values, create per-thread buffer of integers to push
    std::vector<std::vector<int>> per_thread_values(threads);
    for (size_t t = 0; t < threads; ++t) {
        per_thread_values[t].resize(ITEM_OPERATIONS);
        // fill with deterministic values
        std::iota(per_thread_values[t].begin(), per_thread_values[t].end(), static_cast<int>(t * 1000000));
    }

    std::atomic<int> ready_count{0};
    std::atomic<bool> start_flag{false};
    std::atomic<bool> fatal_error{false};
    std::vector<std::thread> workers;
    workers.reserve(threads);

    auto worker_fn = [&](size_t tid){
        try {
            // signal ready
            ready_count.fetch_add(1, std::memory_order_release);

            // wait until master sets start_flag (with small sleep to avoid tight spin)
            while (!start_flag.load(std::memory_order_acquire)) {
                if (fatal_error.load(std::memory_order_acquire)) return;
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }

            for (size_t i = 0; i < ops_per_thread; ++i) {
                int idx = 0;
                if (fatal_error.load(std::memory_order_acquire)) return;
                for (size_t j = 0; j < global_ops.size(); ++j) {
                    if (global_ops[j]) {
                        // enqueue
                        queue.add(per_thread_values[tid][idx++]);
                    } else {
                        // dequeue
                        int tmp = queue.remove();
                        (void)tmp; // avoid unused warning
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Thread " << tid << " exception: " << e.what() << "\n";
            fatal_error.store(true);
            throw;
        } catch (...) {
            std::cerr << "Thread " << tid << " unknown exception\n";
            fatal_error.store(true);
            throw;
        }
    };

    // create threads, catch std::system_error to print diagnostics
    for (size_t t = 0; t < threads; ++t) {
        try {
            workers.emplace_back(worker_fn, t);
        } catch (const std::system_error& e) {
            std::cerr << "Failed to create thread " << t << ": what()=\"" << e.what() << "\""
                      << " errno=" << (errno ? std::strerror(errno) : "unknown") << "\n";
            // join already-created threads before returning
            for (auto &th : workers) {
                if (th.joinable()) th.join();
            }
            throw; // rethrow to caller
        }
        // small sleep to give thread chance to start and increment ready_count
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // wait until all threads are ready, but with timeout to detect stuck starts
    const auto wait_start_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (ready_count.load(std::memory_order_acquire) != static_cast<int>(threads)) {
        if (fatal_error.load(std::memory_order_acquire)) {
            for (auto &th : workers) if (th.joinable()) th.join();
            throw std::runtime_error("Fatal error occurred before start");
        }
        if (std::chrono::steady_clock::now() > wait_start_deadline) {
            std::cerr << "Timeout waiting for threads to become ready (ready=" << ready_count.load() << ", expected=" << threads << ")\n";
            for (auto &th : workers) if (th.joinable()) th.join();
            throw std::runtime_error("Timeout waiting for threads to become ready");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // start them
    start_flag.store(true, std::memory_order_release);
    auto t0 = std::chrono::high_resolution_clock::now();

    // join and measure, with a long timeout for the whole run
    const auto run_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(300);
    for (auto &th : workers) {
        if (th.joinable()) {
            if (std::chrono::steady_clock::now() > run_deadline) {
                std::cerr << "Benchmark run timeout exceeded\n";
                fatal_error.store(true);
                throw std::runtime_error("Benchmark run timeout");
            }
            th.join();
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    if (fatal_error.load(std::memory_order_acquire)) {
        throw std::runtime_error("Fatal error occurred in one of the worker threads");
    }

    std::chrono::duration<double> dur = t1 - t0;
    return dur.count();
}

int main(int argc, char** argv) {
    std::string out_csv = "benchmark_results.csv";
    if (argc >= 2) out_csv = argv[1];

    std::ofstream ofs(out_csv);
    if (!ofs) {
        std::cerr << "Cannot open " << out_csv << " for writing\n";
        return 1;
    }

    ofs << "queue,threads,seconds\n";

    for (int threads : THREAD_COUNTS) {
        std::cout << "Running with threads=" << threads << " ... " << std::flush;
        try {
                // BLOCKING_QUEUE
                {
                    double t = run_benchmark_queue<BlockingQueue<int>>(threads);
                    ofs << "BLOCKING_QUEUE," << threads << ","  << std::fixed << std::setprecision(6) << t << "\n";
                    ofs.flush();
                    std::cout << "BLOCKING=" << t << "s ";
                }

                // NONBLOCKING_QUEUE
                {
                    double t = run_benchmark_queue<NonBlockingQueue<int>>(threads);
                    ofs << "NONBLOCKING_QUEUE," << threads << "," << std::fixed << std::setprecision(6) << t << "\n";
                    ofs.flush();
                    std::cout << "NONBLOCKING=" << t << "s ";
                }

                // TBB
                {
                    double t = run_benchmark_queue<TBBQueue<int>>(threads);
                    ofs << "TBB_QUEUE," << threads << "," << std::fixed << std::setprecision(6) << t << "\n";
                    ofs.flush();
                    std::cout << "TBB=" << t << "s";
                }

                std::cout << "\n";
            } catch (const std::exception& e) {
                std::cerr << "\nBenchmark for threads=" << threads << " aborted: " << e.what() << "\n";
                // on error, abort further benchmarking
                return 2;
            } catch (...) {
                std::cerr << "\nUnknown exception during benchmark for threads=" << threads<< "\n";
                return 3;
            }
        }

    std::cout << "Benchmark finished. Results written to " << out_csv << "\n";
    std::cout << "Use plot_results.py to generate graphs from the CSV.\n";

    return 0;
}