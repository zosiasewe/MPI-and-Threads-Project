# Scientific Computing Project II – MPI vs Threads

**Zofia Seweryńska**  
**Silesian University of Technology**  
**Faculty of Automatic Control, Electronics and Computer Science**  
**Date:** 24.05.2025  
**Course:** MAKRO S2 – Year I, Semester I  

---

##  Project Overview

This project focuses on converting a multithreaded C++ program into an MPI-based parallel implementation. The task involved understanding two distinct models of parallel computing:

- **C++ threads with shared memory** and synchronization.
- **MPI (Message Passing Interface)** for distributed memory parallelism.

The program’s core function is to **generate, distribute, and process large integer arrays** (each with 100,000 elements) using a producer–consumer model.

---

##  Thread-Based Version (Original)

###  Architecture

The original implementation used C++ threads to run in a **shared memory environment**:

- A **producer thread** generated arrays filled with sequential integers.
- Multiple **consumer threads** retrieved arrays from a **shared bounded queue** and processed them (sorted + checksum).
- Synchronization was done using:
  - `std::mutex`
  - `std::condition_variable`

###  Queue Details

- Queue capacity was limited (e.g., 300 arrays).
- Consumers would wait when the queue was empty.
- The producer would wait when the queue was full.

###  Benefits

- Efficient within a single system.
- Very low communication overhead due to shared memory.
- Near-linear performance up to the number of available logical CPU cores.

---

##  MPI-Based Version (Final)

###  Motivation for Migration

MPI is used for distributed memory systems. Unlike threads, MPI processes do **not share memory**, so all communication must happen through **explicit message passing**.

###  Structure

- **Rank 0 process** acts as the **producer**, sending arrays to consumer ranks via `MPI_Send`.
- **Ranks 1 to N** are **consumers** using `MPI_Recv` to get data, sort it, and compute checksums.
- `MPI_Gather`, `MPI_Barrier`, and `MPI_Reduce` are used for synchronization and performance summary.

###  Highlights

- Arrays are sent round-robin from producer to consumers.
- A "termination array" of -1 values signals consumers to stop.
- Performance data (number of arrays processed, time) is gathered at the end.

###  Code Components

- `MPI_Producer` class: generates and sends arrays.
- `MPI_Consumer` class: receives, processes arrays, and sends results back.
- `main()`: initializes MPI, selects role by rank, collects results, prints summary.

---

##  Performance Comparison

| Number of Consumers | Total Runtime (ms) |
|---------------------|--------------------|
| 1                   | 41384              |
| 2                   | 27363              |
| 4                   | 15532              |
| 8                   | 13814              |
| 16                  | 13761              |
| 32                  | 13971              |
| 64                  | 16594              |

- **Best performance:** at 8 consumers, matching the 8 logical cores of the system.
- **Beyond that:** additional processes increase communication overhead without performance gain.

---

##  Threads vs MPI: A Comparison

| Aspect            | Threads                         | MPI                              |
|-------------------|----------------------------------|-----------------------------------|
| Memory            | Shared                          | Distributed                       |
| Communication     | Fast (shared memory)            | Slower (via message passing)      |
| Synchronization   | Mutexes, condition variables    | MPI_Barrier, MPI_Gather, etc.     |
| Scalability       | Limited to single machine       | Scalable to multiple machines     |
| Performance       | Better at >8 cores on one node  | Slightly slower beyond 8 ranks    |
| Flexibility       | Limited to shared memory        | Flexible across nodes             |

- **Threads:** Best for shared-memory setups like a single powerful machine.
- **MPI:** Better for distributed or large-scale parallel environments.

---

##  Technical Details

###  System Info

- **CPU:** Intel® Core™ i5-8265U @ 1.80GHz
- **Cores:** 4 Physical, 8 Logical
- **OS:** Windows 10
- **Compiler:** Microsoft Visual C++ (cl.exe)
- **Version:** 19.44.35207.1
- **IDE:** Visual Studio 2022
- **Build Mode:** Release, x86

---

##  Files Included

- `MPI_based_code.cpp` – Final MPI-based implementation.
- `Thread_based_code.cpp` – Original thread-based implementation.
- `README.md` – This documentation.

---

##  How to Run (MPI Version)

Ensure MPI is installed (e.g., MS-MPI or MPICH), then compile and run:

```bash
mpic++ -o mpi_project mpi_project.cpp
mpiexec -n 5 ./mpi_project
