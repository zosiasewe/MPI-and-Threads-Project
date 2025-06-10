// Done by Zofia Seweryńska
// 05.2025, Poland

#include <iostream>
#include <array>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <numeric>
#include <chrono>

using namespace std;

const int ARRSIZE = 100000;
mutex print_mutex;

class Queue {
    queue<array<int, ARRSIZE>> q;
    size_t max_size;
    mutex mtx;
    condition_variable cv; // for blocking threads until queue state changes
    bool done_producing = false;

public:
    Queue(size_t max_length) : max_size(max_length) {}
    void add(const array<int, ARRSIZE>& arr) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return q.size() < max_size; });
        q.push(arr);
        cv.notify_all();
    }

    bool remove(array<int, ARRSIZE>& result) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return !q.empty() || done_producing; });

        if (q.empty() && done_producing) {
            return false;
        }

        result = q.front();
        q.pop();
        cv.notify_all();
        return true;
    }

    void done() {
        unique_lock<mutex> lock(mtx);
        done_producing = true;
        cv.notify_all();
    }
};

class Producer {
    Queue& queue;
    int array_num;

public:
    Producer(Queue& q, int n) : queue(q), array_num(n) {}
    void run() {
        {
            lock_guard<mutex> lock(print_mutex);
            cout << "Start - Producer\n";
        }

        auto start = chrono::high_resolution_clock::now();

        for (int i = 0; i < array_num; ++i) {
            array<int, ARRSIZE> arr;
            int value = 0;
            for (auto it = arr.begin(); it != arr.end(); ++it) {
                *it = value++;
            }
            queue.add(arr);
        }

        queue.done();

        auto end = chrono::high_resolution_clock::now();
        {
            lock_guard<mutex> lock(print_mutex);
            cout << "Stop - Producer - " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms\n";
        }
    }
};

class Consumer {
    Queue& queue;
    int consumed = 0;

    int calculateChecksum(const array<int, ARRSIZE>& arr) {
        return accumulate(arr.begin(), arr.end(), 0);
    }

public:
    Consumer(Queue& q) : queue(q) {}
    void run() {
        {
            lock_guard<mutex> lock(print_mutex);
            cout << "Thread " << this_thread::get_id() << " - Start\n";
        }
        auto start = chrono::high_resolution_clock::now();

        while (true) {
            array<int, ARRSIZE> arr;
            bool success = queue.remove(arr);
            if (!success) break;

            sort(arr.begin(), arr.end());
            int checksum = calculateChecksum(arr);

            {
                lock_guard<mutex> lock(print_mutex);
                cout << "Thread " << this_thread::get_id() << " sum: " << checksum << "\n";
            }

            ++consumed;
        }
        auto end = chrono::high_resolution_clock::now();

        {
            lock_guard<mutex> lock(print_mutex);
            cout << "Thread " << this_thread::get_id() << " processed " << consumed
                << " arrays in " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms\n";
        }
    }
};

int main() {
    const size_t queue_capacity = 300;
    const int array_num = 1000;
    const int consumer_num = 64;

    {
        lock_guard<mutex> lock(print_mutex);
        cout << "hardware concurrency: " << thread::hardware_concurrency() << " threads\n";
        cout << "queue capacity: " << queue_capacity << "\n";
        cout << "number of arrays to produce: " << array_num << "\n";
        cout << "number of consumer threads: " << consumer_num << "\n";
    }

    auto global_start = chrono::high_resolution_clock::now();

    Queue queue(queue_capacity);
    Producer producer(queue, array_num);

    thread producerThread(&Producer::run, &producer);
    vector<thread> consumer_threads;
    vector<unique_ptr<Consumer>> consumers;

    for (int i = 0; i < consumer_num; ++i) {
        consumers.emplace_back(make_unique<Consumer>(queue));
        consumer_threads.emplace_back(&Consumer::run, consumers.back().get());
    }

    producerThread.join();
    for (auto& t : consumer_threads) {
        t.join();
    }
    auto global_end = chrono::high_resolution_clock::now();

    {
        lock_guard<mutex> lock(print_mutex);
        cout << "End\n";
        cout << "Total runtime: " << chrono::duration_cast<chrono::milliseconds>(global_end - global_start).count() << " ms\n";
    }

    return 0;
}
