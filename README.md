# Concurrent Systems Lab

[![Standard](https://img.shields.io/badge/C%20Standard-C99%20%2F%20C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Concurrency](https://img.shields.io/badge/Concurrency-Pthreads%20%7C%20OpenMP-orange.svg)](https://computing.llnl.gov/tutorials/pthreads/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build & Tests](https://img.shields.io/badge/Tests-8%2F8%20Passing-brightgreen.svg)](Makefile)
[![Author](https://img.shields.io/badge/Author-Josu%C3%A9%20Torres%20Sibaja-0ea5e9.svg)](https://github.com/josuetorres27)

A modular laboratory exploring concurrent programming paradigms, parallel computing models, deadlock prevention mechanisms, and hardware-level synchronization primitives in C. Features physical simulations, classic concurrency patterns, micro-benchmarks, and an interactive 2D web visualizer.

---

## 🏛️ Architecture & Project Structure

```text
concurrent-systems-lab/
├── Makefile                        # Root orchestrator: builds & runs all modules
├── LICENSE                         # MIT License
├── README.md                       # Main laboratory documentation
├── docs/                           # Theoretical and algorithmic specifications
│   ├── Concurrency_Patterns.md     # In-depth analysis of classic patterns
│   ├── Heat_Diffusion_Theory.md    # PDE discretization & domain decomposition
│   └── Roundabout_Specification.md # Coffman conditions & semaphore transit
├── patterns/                       # Core concurrency patterns & synchronization
│   ├── bounded-buffer/             # Producer-Consumer with counting semaphores
│   ├── dining-philosophers/        # Dijkstra resource hierarchy (deadlock-free)
│   ├── molecule-assembler/         # H2O multi-thread rendezvous barrier
│   └── synchronization-bench/      # Benchmark: Atomics vs Spinlocks vs Mutexes vs Semaphores
├── simulations/                    # Multi-threaded numerical & physical models
│   ├── heat-diffusion/             # 2D Finite Difference Heat Equation solver
│   │   ├── serial/                 # Baseline single-threaded implementation
│   │   ├── pthreads/               # 1D row slicing domain decomposition + barriers
│   │   └── openmp/                 # Work-sharing constructs & SIMD loop reduction
│   └── roundabout/                 # Traffic simulator with POSIX semaphores
└── web/                            # Interactive multi-tab 2D visualizer
    └── index.html                  # HTML5 Canvas + Web Audio API synthesizer
```

---

## 🔬 Modules Overview

### 1. Traffic Roundabout Simulation (`simulations/roundabout/`)
- **Focus:** Preventing deadlocks in cyclic topologies with shared limited-capacity sectors.
- **Mechanism:** Implements Dijkstra's resource allocation and master capacity semaphores (`sem_t`) to eliminate Coffman circular wait conditions across multi-threaded vehicles.
- **Specification:** See [docs/Roundabout_Specification.md](docs/Roundabout_Specification.md).

### 2. 2D Heat Diffusion Solver (`simulations/heat-diffusion/`)
- **Focus:** Parallel PDE solving via 5-point finite difference stencils.
- **Implementations:**
  - **Serial:** Single-threaded baseline for precision verification.
  - **POSIX Threads:** Explicit 1D horizontal row domain decomposition with double buffering and `pthread_barrier_t` synchronization.
  - **OpenMP:** Multi-core parallel loops with `#pragma omp parallel for reduction(max:delta)`.
- **Theory & Formulation:** See [docs/Heat_Diffusion_Theory.md](docs/Heat_Diffusion_Theory.md).

### 3. Classic Concurrency Patterns (`patterns/`)
- **Bounded Buffer:** Thread-safe ring buffer with dual semaphores (`sem_empty`, `sem_full`) and mutex protection.
- **Dining Philosophers:** Asymmetric Dijkstra resource ordering avoiding dining deadlocks without coordinator bottlenecks.
- **H2O Molecule Assembler:** Three-thread rendezvous pattern generating water molecules ($2\text{H} + 1\text{O}$) with reusable barriers.
- **Synchronization Benchmark:** Quantitative micro-benchmark comparing throughput between:
  1. C11 Atomics (`atomic_fetch_add`)
  2. POSIX Spinlocks (`pthread_spinlock_t`)
  3. POSIX Mutexes (`pthread_mutex_t`)
  4. POSIX Counting Semaphores (`sem_t`)
- **Details:** See [docs/Concurrency_Patterns.md](docs/Concurrency_Patterns.md).

---

## ⚡ Quick Start & Compilation

### Prerequisites
- GCC / Clang with C99, C11, POSIX Threads (`-pthread`), and OpenMP (`-fopenmp`) support.
- Make build tool.
- Linux, macOS, or Windows Subsystem for Linux (WSL).

### Build All Modules
```bash
# Clone the repository
git clone https://github.com/josuetorres27/concurrent-systems-lab.git
cd concurrent-systems-lab

# Build all 8 targets
make all
```

### Run Automated Verification Test Suite
```bash
make test
```
*Executes all 8 modules consecutively and reports status.*

### Individual Module Execution
```bash
# Run Roundabout Traffic Simulator (15 vehicles auto-generated)
./simulations/roundabout/bin/roundabout_sim

# Run Heat Diffusion (Pthreads: 4 threads, 400 iterations)
./simulations/heat-diffusion/pthreads/bin/heat_pthreads 4 400

# Run Heat Diffusion (OpenMP: 4 threads, 400 iterations)
./simulations/heat-diffusion/openmp/bin/heat_openmp 4 400

# Run Low-Level Synchronization Benchmark (4 threads, 5,000,000 increments each)
./patterns/synchronization-bench/bin/sync_bench 4 5000000
```

---

## 📊 Benchmark Preview

Representative results on an 8-core / 16-thread CPU (20,000,000 total increments):

| Synchronization Primitive | Time Elapsed (s) | Throughput (Ops/sec) | Relative Speedup |
|---|---|---|---|
| **C11 Atomic Fetch-and-Add** | `0.114 s` | ~175,400,000 ops/s | **1.00x (Baseline Fast)** |
| **Pthread Spinlock** | `0.238 s` | ~84,000,000 ops/s | **0.48x** |
| **Pthread Mutex** | `0.589 s` | ~33,900,000 ops/s | **0.19x** |
| **POSIX Binary Semaphore** | `1.412 s` | ~14,160,000 ops/s | **0.08x** |

---

## 🎨 Interactive 2D Web Visualizer

The visualizer provides live simulations of the underlying models in any modern browser without external dependencies:

```bash
# Open directly in your browser
open web/index.html      # macOS
xdg-open web/index.html  # Linux
start web/index.html     # Windows PowerShell
```

### Visualizer Features
- **Roundabout Simulator:** Real-time 2D canvas displaying vehicles navigating quadrant sectors, with interactive controls to enable/disable semaphore guards, adjust arrival rates, and induce or resolve deadlocks dynamically.
- **Heat Diffusion:** Live numerical colormap rendering thermal conductivity across a discretized grid with thread domain slicing overlays.
- **Concurrency Patterns:** Dynamic visual state monitors for Bounded Buffer slot occupancy and Dining Philosophers eating/thinking states.
- **Procedural Audio Synthesizer:** Real-time Web Audio API sound feedback for vehicle entries, heat steps, and philosopher transitions.

---

## 📄 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Developed with precision by **Josué Torres Sibaja**.
