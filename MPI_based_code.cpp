// Done by Zofia Seweryńska
// 05.2025, Poland

#include <iostream>
#include <array>
#include <vector>
#include <algorithm>
#include <numeric>

#include "mpi.h"
using namespace std;

const int ARRSIZE = 100000;

class MPI_Producer {
    int value = 0;
    int array_num;
    int world_rank;
    int world_size;

public:
    MPI_Producer(int n, int rank, int size) : array_num(n), world_rank(rank), world_size(size) {}

    void run() {
        array<int, ARRSIZE> message;
        double start_time = MPI_Wtime();

        for (int i = 0; i < array_num; ++i) {
            for (auto& element : message) {
                element = value++;
            }

            int consumer_rank = 1 + (i % (world_size - 1));
            MPI_Send(message.data(), ARRSIZE, MPI_INT, consumer_rank, 0, MPI_COMM_WORLD);
        }

        array<int, ARRSIZE> done_message;
        done_message.fill(-1);
        for (int rank = 1; rank < world_size; ++rank) {
            MPI_Send(done_message.data(), ARRSIZE, MPI_INT, rank, 0, MPI_COMM_WORLD);
        }

        double end_time = MPI_Wtime();
    }
};

class MPI_Consumer {
    int world_rank;
    int local_count = 0;
    double local_time_ms = 0;

public:
    MPI_Consumer(int rank) : world_rank(rank) {}

    void run() {
        array<int, ARRSIZE> message;
        double start = MPI_Wtime();

        while (true) {
            MPI_Status status;
            MPI_Recv(message.data(), ARRSIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            if (message[0] == -1) break;

            sort(message.begin(), message.end());
            ++local_count;
        }

        double end = MPI_Wtime();
        local_time_ms = (end - start) * 1000.0;
    }

    int getCount() const { return local_count; }
    double getTimeMs() const { return local_time_ms; }
};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    const int array_num = 1000;
    const int queue_capacity = 300;

    double global_start = MPI_Wtime();

    int local_count = 0;
    double local_time = 0;

    if (world_rank == 0) {
        MPI_Producer producer(array_num, world_rank, world_size);
        producer.run();
        local_time = MPI_Wtime() - global_start;
    }
    else {
        MPI_Consumer consumer(world_rank);
        consumer.run();
        local_count = consumer.getCount();
        local_time = consumer.getTimeMs() / 1000.0;  // seconds
    }

    vector<int> all_counts(world_size, 0);
    vector<double> all_times(world_size, 0.0);

    MPI_Gather(&local_count, 1, MPI_INT, all_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&local_time, 1, MPI_DOUBLE, all_times.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double max_runtime = 0.0;
    MPI_Reduce(&local_time, &max_runtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    if (world_rank == 0) {
        cout << "queue capacity: " << queue_capacity << "\n";
        cout << "number of arrays to produce: " << array_num << "\n";
        cout << "number of consumer threads: " << world_size - 1 << "\n";

        for (int i = 1; i < world_size; ++i) {
            cout << "Process " << i << " processed " << all_counts[i]
                << " arrays in " << static_cast<int>(all_times[i] * 1000) << " ms\n";
        }
        cout << "Total runtime: " << static_cast<int>(max_runtime * 1000) << " ms\n";
    }

    MPI_Finalize();
    return 0;
}
