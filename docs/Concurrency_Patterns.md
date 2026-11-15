# Classic Concurrency Patterns & Benchmarking

## 1. Overview
This module demonstrates classic synchronization patterns and compares the cost of low-level synchronization primitives, using POSIX Threads (`pthreads`), POSIX semaphores and C11 atomics.

---

## 2. Patterns Catalog

### 2.1 Bounded Buffer (Producer-Consumer)
- **Problem Statement:** Two producer threads insert 25 items each into a ring buffer of 8 slots (`BUFFER_CAPACITY`), while two consumer threads remove 25 items each. Producers must block when the buffer is full; consumers must block when it is empty.
- **Synchronization Primitives:**
  - `sem_t sem_empty`: Initialized to `BUFFER_CAPACITY` (counts empty slots).
  - `sem_t sem_full`: Initialized to `0` (counts filled slots).
  - `pthread_mutex_t mutex`: Protects the ring buffer indices and size (`head`, `tail`, `count`).
- **Liveness Guarantee:** Threads block on the counting semaphores instead of busy-waiting, and the mutex prevents two threads from updating the same slot or index at once.

```text
Producers: sem_wait(&sem_empty) -> lock(&mutex) -> insert item -> unlock(&mutex) -> sem_post(&sem_full)
Consumers: sem_wait(&sem_full)  -> lock(&mutex) -> remove item -> unlock(&mutex) -> sem_post(&sem_empty)
```

---

### 2.2 Dining Philosophers (Dijkstra Resource Hierarchy)
- **Problem Statement:** 5 philosophers (`NUM_PHILOSOPHERS`) sit around a circular table and each eats 5 times. Eating requires both adjacent forks, which are mutexes (`pthread_mutex_t forks[5]`). Philosopher $i$ has fork $i$ on the left and fork $(i+1) \bmod N$ on the right.
- **Deadlock Condition:** If every philosopher simultaneously picks up their left fork, a circular wait is established:
  $$P_0 \to C_1 \land P_1 \to C_2 \land \dots \land P_{N-1} \to C_0$$
- **Solution:** **Resource Ordering (Dijkstra's Hierarchy)**.
  Every philosopher always requests the lower-numbered fork first:
  ```c
  int first_fork  = (left_fork < right_fork) ? left_fork : right_fork;
  int second_fork = (left_fork < right_fork) ? right_fork : left_fork;
  pthread_mutex_lock(&forks[first_fork]);
  pthread_mutex_lock(&forks[second_fork]);
  ```
  Because philosopher $N-1$ requests fork $0$ before fork $N-1$, the symmetry is broken and a circular wait cannot form.

---

### 2.3 H2O Molecule Assembler (Thread Rendezvous)
- **Problem Statement:** 20 hydrogen threads and 10 oxygen threads arrive asynchronously. They must be grouped into molecules of exactly $2\text{H} + 1\text{O}$, and the three atoms of each molecule must meet before bonding.
- **Synchronization Mechanism** (as in Downey's *The Little Book of Semaphores*):
  - `sem_t admission` (binary semaphore, initialized to 1) protects `hydrogen_count` and `oxygen_count`, the numbers of atoms waiting.
  - Each arriving atom takes `admission` and increments its count. If at least 2 hydrogens and 1 oxygen are waiting, it releases one molecule: `sem_post(&sem_h)` twice and `sem_post(&sem_o)` once, and it keeps `admission`. Otherwise it releases `admission`.
  - Every atom then waits on its queue (`sem_h` or `sem_o`), and the three released atoms meet at a `pthread_barrier_t` initialized to 3.
  - After the barrier, the oxygen increments `molecules_formed`, prints the molecule and releases `admission`.
- **Why the Groups Are Correct:** `admission` stays held from the moment a molecule is released until its oxygen has passed the barrier, so the atoms of the next molecule cannot reach the barrier early. `admission` is a semaphore rather than a mutex because it is released by a different thread (the oxygen) than the one that took it. Each atom prints the number of the molecule it joined.

---

### 2.4 Low-Level Synchronization Primitives Benchmark
A micro-benchmark in which $N$ threads (default 4, maximum 64) each increment a shared counter 1,000,000 times, protected by one of four mechanisms:

| Mechanism | Implementation | Behavior Under Contention |
| :--- | :--- | :--- |
| **C11 Atomic** | `atomic_fetch_add_explicit(..., memory_order_relaxed)` on an `atomic_llong` | A single atomic read-modify-write instruction; no lock. Threads still compete for the counter's cache line. |
| **POSIX Spinlock** | `pthread_spinlock_t` | Waiting threads spin in user space, consuming CPU until the lock is free. |
| **Pthread Mutex** | `pthread_mutex_t` with `PTHREAD_MUTEX_INITIALIZER` (default type) | Uncontended operations stay in user space; waiting threads sleep in the kernel (futex) until woken. |
| **POSIX Binary Semaphore** | `sem_t` initialized to 1 and used as a lock | Like the mutex, futex-based: uncontended operations stay in user space and waiting threads sleep in the kernel. |

The README reports one run with 4 threads on Ubuntu 22.04 under WSL2: the atomic counter was the fastest and the semaphore the slowest, and the mutex was faster than the spinlock. The order of the spinlock and the mutex in particular depends on the hardware, the operating system and the scheduler.
