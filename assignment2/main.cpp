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
#include "random_bits.hpp"

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

template<typename QueueType>
static inline void shuffle_queue(QueueType &queue, size_t eqn_count) {
    while (eqn_count > 0) {
        int random_ = rand() % 1000000;
        queue.add(random_);
        --eqn_count;
    }
}

const size_t TOTAL_OPERATIONS = 1<< 20; 
const size_t QUEUE_CAPACITY = 1 << 20; // 队列容量
const std::vector<double> RATIO{0.3, 0.5, 0.8}; // 不同入队出队比例
const std::vector<int> THREAD_COUNTS = [](){
    std::vector<int> v;
    for (int i = 1; i <= 32; ++i) v.push_back(i);
    return v;
}();

template<typename QueueType>
double run_benchmark_queue(size_t threads, double ratio) {
    // create queue
    QueueType queue(QUEUE_CAPACITY);

    std::vector<uint8_t> global_ops = generate_random_bits(TOTAL_OPERATIONS, ratio);
    // 预填充 避免空队列卡死
    if (ratio == 0.3)
        shuffle_queue(queue, TOTAL_OPERATIONS * 0.5);
    else if (ratio == 0.5)
        shuffle_queue(queue, TOTAL_OPERATIONS * 0.3);
    else
        shuffle_queue(queue, TOTAL_OPERATIONS * 0.1);

    size_t ops_per_thread = TOTAL_OPERATIONS / threads;
    std::vector<std::vector<int>> per_thread_values(threads);
    for (size_t t = 0; t < threads; ++t) {
        per_thread_values[t].resize(ops_per_thread);
        std::iota(per_thread_values[t].begin(), per_thread_values[t].end(), static_cast<int>(t * 100000));
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

            while (!start_flag.load(std::memory_order_acquire)) {
                if (fatal_error.load(std::memory_order_acquire)) return;
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }

            for (size_t i = 0; i < ops_per_thread; ++i) {
                uint8_t op = global_ops[tid * ops_per_thread + i];
                if (op == 1) { // enqueue
                    queue.add(per_thread_values[tid][i]);
                } else { // dequeue
                    int val = queue.remove();
                    (void)val; // 防止未使用警告
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


    for (size_t t = 0; t < threads; ++t) {
        workers.emplace_back(worker_fn, t);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

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

    // start 
    start_flag.store(true, std::memory_order_release);
    auto t0 = std::chrono::high_resolution_clock::now();

    for (auto &th : workers) {
        if (th.joinable()) {
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

    ofs << "queue,threads,ratio,seconds\n";

    for (auto ratio : RATIO) {
        //ofs << "# Ratio=" << ratio << "\n";
        std::cout << "Benchmarking with ratio=" << ratio << " ... \n";

        for (int threads : THREAD_COUNTS) {
            std::cout << "Running with threads=" << threads << " ... " << std::flush;
            try {
                    // BLOCKING_QUEUE
                    {
                        double t = run_benchmark_queue<BlockingQueue<int>>(threads, ratio);
                        ofs << "BLOCKING_QUEUE," << threads << ","  << ratio << "," << std::fixed << std::setprecision(6) << t << "\n";
                        ofs.flush();
                        std::cout << "BLOCKING=" << t << "s ";
                    }

                    // NONBLOCKING_QUEUE
                    {
                        double t = run_benchmark_queue<NonBlockingQueue<int>>(threads, ratio);
                        ofs << "NONBLOCKING_QUEUE," << threads << ","  << ratio << "," << std::fixed << std::setprecision(6) << t << "\n";
                        ofs.flush();
                        std::cout << "NONBLOCKING=" << t << "s ";
                    }

                    // TBB
                    {
                        double t = run_benchmark_queue<TBBQueue<int>>(threads, ratio);
                        ofs << "TBB_QUEUE," << threads << ","  << ratio << "," << std::fixed << std::setprecision(6) << t << "\n";
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
    }

    std::cout << "Benchmark finished. Results written to " << out_csv << "\n";
    std::cout << "Use plot_results.py to generate graphs from the CSV.\n";

    return 0;
}