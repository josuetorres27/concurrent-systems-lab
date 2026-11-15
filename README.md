# Concurrent Systems Lab

[![Standard](https://img.shields.io/badge/C%20Standard-C11-blue?style=flat-square)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Concurrency](https://img.shields.io/badge/Concurrency-Pthreads%20%7C%20OpenMP-orange?style=flat-square)](https://hpc-tutorials.llnl.gov/posix/)
[![Smoke Tests](https://img.shields.io/badge/Smoke%20tests-make%20test-lightgrey?style=flat-square)](#testing)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow?style=flat-square)](https://opensource.org/license/MIT)

A modular lab for concurrent and parallel programming in C11 with POSIX Threads and OpenMP. It contains a semaphore-based traffic simulation, serial / Pthreads / OpenMP solvers for 2D heat diffusion, classic synchronization patterns, a micro-benchmark of locking primitives, and a dependency-free 2D web visualizer.

---

## Features

### Traffic Roundabout Simulation (`simulations/roundabout/`)

- **Focus:** Vehicles (one thread each) share a ring of four limited-capacity quadrants (N, W, S, E).
- **Mechanism:** Each quadrant is guarded by its own POSIX counting semaphore (`sem_t`) initialized to the quadrant capacity. A vehicle releases its current quadrant (`sem_post`) before acquiring the next one (`sem_wait`), so it never holds more than one quadrant at a time. Because there is no hold-and-wait, a circular wait cannot form and the simulation cannot deadlock.
- **Output:** Each vehicle's path and transit time, printed in vehicle-ID order using a mutex and condition variable.
- **Specification:** See [docs/Roundabout_Specification.md](docs/Roundabout_Specification.md).

### 2D Heat Diffusion Solver (`simulations/heat-diffusion/`)

- **Focus:** Iterative 5-point finite-difference stencil on a 2D plate until the maximum per-cell change is at most `epsilon` (or 25,000 iterations).
- **Implementations:**
  - **Serial:** Single-threaded baseline. All three versions converge in the same number of iterations to the same final delta for the same inputs.
  - **POSIX Threads:** Explicit 1D row domain decomposition with double buffering and `pthread_barrier_t` synchronization. Thread 0 aggregates the per-thread deltas and swaps the buffers between two barriers.
  - **OpenMP:** `#pragma omp parallel for reduction(max:max_delta) schedule(static)` over the grid rows.
- **Theory and Formulation:** See [docs/Heat_Diffusion_Theory.md](docs/Heat_Diffusion_Theory.md).

### Classic Concurrency Patterns (`patterns/`)

- **Bounded Buffer:** 8-slot ring buffer shared by 2 producers and 2 consumers (50 items), using two counting semaphores (`sem_empty`, `sem_full`) plus a mutex.
- **Dining Philosophers:** 5 philosophers, 5 rounds each. Every philosopher locks the lower-numbered fork first (resource ordering), which breaks the circular-wait condition without a central coordinator.
- **H2O Molecule Assembler:** Hydrogen and oxygen threads form 10 water molecules ($2\text{H} + 1\text{O}$). A binary admission semaphore releases one molecule at a time from the hydrogen and oxygen queues (`sem_h`, `sem_o`), and its three atoms meet at a reusable `pthread_barrier_t` with a count of 3.
- **Synchronization Benchmark:** Each thread increments a shared counter 1,000,000 times under high contention. It compares C11 atomics (`atomic_fetch_add_explicit`, relaxed ordering), POSIX spinlocks (`pthread_spinlock_t`), POSIX mutexes (`pthread_mutex_t`) and a POSIX semaphore used as a binary lock (`sem_t` initialized to 1).
- **Details:** See [docs/Concurrency_Patterns.md](docs/Concurrency_Patterns.md).

### Interactive 2D Web Visualizer (`web/`)

`web/index.html` is a single self-contained page (no external scripts or stylesheets) with browser-side animations of the models. It is a visual companion and does not run the C code.

- **Roundabout:** 2D canvas of vehicles moving through the four quadrants. Controls include spawning vehicles, adjusting quadrant capacity, arrival rate and speed, and toggling between semaphore-guarded and unsynchronized modes. In unsynchronized mode, a banner appears when every quadrant is full (deadlock).
- **Heat Diffusion:** Live temperature colormap of the grid, with optional overlays that show how rows are split among threads. Configurable grid size, thread count and epsilon.
- **Concurrency Patterns:** A bounded-buffer view with manual Produce/Consume buttons and live `sem_empty` / `sem_full` counters, plus a dining-philosophers diagram. Its "Toggle State" button assigns each philosopher a random thinking/hungry/eating state for illustration; it does not run the fork-ordering algorithm.
- **Audio:** Optional Web Audio API sound effects for roundabout events, tab switches, buffer operations, philosopher state toggles, and the heat simulation reaching equilibrium.

---

## Benchmark Results

One run of `sync_bench 4` (4 threads × 1,000,000 increments = 4,000,000 operations) on Ubuntu 22.04 under WSL2, GCC 11.4, 16 logical CPUs. Results depend heavily on hardware, OS and scheduler, so the ranking of spinlock vs. mutex in particular may differ on other machines.

| Synchronization Primitive | Time Elapsed (s) | Throughput (Mops/s) | Throughput vs. Atomic |
| :--- | :---: | :---: | :---: |
| **C11 Atomic Fetch-Add** | 0.0781 | 51.18 | 1.00x |
| **POSIX Spinlock** | 0.4157 | 9.62 | 0.19x |
| **Pthread Mutex** | 0.2504 | 15.97 | 0.31x |
| **POSIX Binary Semaphore** | 1.3692 | 2.92 | 0.06x |

---

## Project Structure

```text
concurrent-systems-lab/
├── Makefile                        # Root orchestrator: builds all modules, runs smoke tests
├── LICENSE                         # MIT License
├── README.md                       # Main laboratory documentation
├── docs/                           # Theoretical and algorithmic notes
│   ├── Concurrency_Patterns.md     # Analysis of the classic patterns
│   ├── Heat_Diffusion_Theory.md    # PDE discretization & domain decomposition
│   └── Roundabout_Specification.md # Coffman conditions & semaphore transit
├── patterns/                       # Classic concurrency patterns & synchronization
│   ├── bounded-buffer/             # Producer-consumer with semaphores + mutex
│   ├── dining-philosophers/        # Resource ordering (deadlock-free)
│   ├── molecule-assembler/         # H2O rendezvous with semaphores + barrier
│   └── synchronization-bench/      # Benchmark: atomics vs spinlock vs mutex vs semaphore
├── simulations/                    # Multi-threaded numerical & traffic models
│   ├── heat-diffusion/             # 2D finite-difference heat equation solver
│   │   ├── serial/                 # Single-threaded baseline
│   │   ├── pthreads/               # 1D row domain decomposition + barriers
│   │   └── openmp/                 # parallel for + max reduction
│   └── roundabout/                 # Traffic simulator with POSIX semaphores
└── web/                            # Interactive multi-tab 2D visualizer
    └── index.html                  # HTML5 Canvas + Web Audio API
```

---

## Getting Started

### Prerequisites

- GCC with C11, POSIX Threads (`-pthread`) and OpenMP (`-fopenmp`, libgomp) support.
- GNU Make.
- Linux or Windows Subsystem for Linux (WSL). Tested on Ubuntu 22.04 (WSL2) with GCC 11.4.

macOS is not supported. Its libc lacks `pthread_barrier_t` and `pthread_spinlock_t`, does not support unnamed semaphores (`sem_init`), and Apple Clang has no built-in OpenMP.

### Build

```bash
git clone https://github.com/josuetorres27/concurrent-systems-lab.git
cd concurrent-systems-lab

make all          # build all 8 binaries
```

Root `Makefile` targets:

| Target | Description |
| :--- | :--- |
| `make all` | Build all 8 modules |
| `make roundabout` | Build the roundabout simulator only |
| `make heat` | Build the serial, Pthreads and OpenMP heat solvers |
| `make patterns` | Build the four pattern programs |
| `make test` | Build everything, then run the smoke tests (see [Testing](#testing)) |
| `make clean` | Remove every module's `bin/` and `build/` directories |

### Run

Each module also has its own `Makefile` with `all`, `run` and `clean` targets, for example:

```bash
make -C simulations/heat-diffusion/openmp run   # runs: ./bin/heat_openmp 150 150 4 0.05
make -C patterns/synchronization-bench run      # runs: ./bin/sync_bench 4
```

Open the web visualizer directly in a browser:

```bash
xdg-open web/index.html  # Linux
start web/index.html     # Windows PowerShell
```

---

## Usage

All binaries are written to the module's `bin/` directory. Arguments are positional and optional; defaults are shown in brackets.

| Binary | Usage |
| :--- | :--- |
| `simulations/roundabout/bin/roundabout` | `[min_ms=5] [max_ms=20] [capacity=3] [-v\|--verbose] [-f file]` |
| `simulations/heat-diffusion/serial/bin/heat_serial` | `[rows=200] [cols=200] [epsilon=0.01]` |
| `simulations/heat-diffusion/pthreads/bin/heat_pthreads` | `[rows=200] [cols=200] [threads=4] [epsilon=0.01]` |
| `simulations/heat-diffusion/openmp/bin/heat_openmp` | `[rows=200] [cols=200] [threads=all cores] [epsilon=0.01]` |
| `patterns/synchronization-bench/bin/sync_bench` | `[threads=4]` (max 64; 1,000,000 increments per thread) |
| `patterns/bounded-buffer/bin/bounded_buffer` | No arguments |
| `patterns/dining-philosophers/bin/dining_philosophers` | No arguments |
| `patterns/molecule-assembler/bin/molecule_assembler` | No arguments |

Roundabout notes: `min_ms` / `max_ms` set the random transit delay per quadrant and `capacity` sets the vehicles allowed per quadrant. Flags may appear anywhere among the numbers; the capacity must be at least 1 and `max_ms` must not be smaller than `min_ms`. Without `-f`, 15 vehicles with random entry and exit points are generated. With `-f`, the file lists one vehicle per line as `ENTRY EXIT` using `N`, `E`, `S`, `W`.

Heat diffusion notes: `rows` and `cols` must be between 3 and 100,000, and the thread count must be at least 1.

Examples:

```bash
# Roundabout: 15 random vehicles, 5-20 ms per quadrant, capacity 3, verbose
./simulations/roundabout/bin/roundabout 5 20 3 -v

# Roundabout: vehicles read from a file
printf "N S\nE W\nS E\n" > traffic.txt
./simulations/roundabout/bin/roundabout 5 20 3 -f traffic.txt

# Heat diffusion: 200x200 plate, epsilon 0.01 (Pthreads and OpenMP with 4 threads)
./simulations/heat-diffusion/serial/bin/heat_serial 200 200 0.01
./simulations/heat-diffusion/pthreads/bin/heat_pthreads 200 200 4 0.01
./simulations/heat-diffusion/openmp/bin/heat_openmp 200 200 4 0.01

# Synchronization benchmark: 4 threads x 1,000,000 increments
./patterns/synchronization-bench/bin/sync_bench 4
```

---

## Testing

```bash
make test
```

`make test` builds all modules, then runs each of the 8 binaries once with small, fixed parameters (see the `test` target in the [Makefile](Makefile)). Make stops if any program exits with a non-zero status. These are smoke tests: they check that every program runs to completion, not the correctness of its output.

---

## Documentation

- [Roundabout_Specification.md](docs/Roundabout_Specification.md): Coffman conditions and semaphore transit.
- [Heat_Diffusion_Theory.md](docs/Heat_Diffusion_Theory.md): PDE discretization and domain decomposition.
- [Concurrency_Patterns.md](docs/Concurrency_Patterns.md): analysis of the classic patterns.

---

## License

This project is licensed under the [MIT License](LICENSE).

---

Created by **Josue Torres Sibaja** · [Portfolio](https://josuetorressibaja.com/) · [LinkedIn](https://www.linkedin.com/in/josue-torres-sibaja/) · [GitHub](https://github.com/josuetorres27)
