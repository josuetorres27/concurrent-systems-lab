# Heat Diffusion Simulation & Domain Decomposition

## 1. Physical & Mathematical Formulation

The transient heat conduction in a two-dimensional isotropic medium with constant thermal diffusivity $\alpha$ is governed by the 2D parabolic partial differential equation (Fourier's Heat Equation):

$$\frac{\partial u}{\partial t} = \alpha \left( \frac{\partial^2 u}{\partial x^2} + \frac{\partial^2 u}{\partial y^2} \right)$$

where:
- $u(x, y, t)$ represents temperature at spatial coordinate $(x, y)$ and time $t$.
- $\alpha = \frac{k}{c_p \rho}$ is the thermal diffusivity coefficient ($\text{m}^2/\text{s}$).

### 1.1 Finite Difference Discretization
Using explicit forward Euler time stepping and second-order central spatial differences on a uniform Cartesian grid with spatial step $h = \Delta x = \Delta y$:

$$\frac{\partial u}{\partial t} \approx \frac{u_{i,j}^{t+1} - u_{i,j}^t}{\Delta t}$$

$$\frac{\partial^2 u}{\partial x^2} + \frac{\partial^2 u}{\partial y^2} \approx \frac{u_{i+1,j}^t + u_{i-1,j}^t + u_{i,j+1}^t + u_{i,j-1}^t - 4u_{i,j}^t}{h^2}$$

Rearranging gives the 5-point stencil update equation, with $r = \frac{\alpha \Delta t}{h^2}$:

$$u_{i,j}^{t+1} = u_{i,j}^t + r \left( u_{i+1,j}^t + u_{i-1,j}^t + u_{i,j+1}^t + u_{i,j-1}^t - 4u_{i,j}^t \right)$$

### 1.2 Numerical Stability Criterion (CFL Condition)
For numerical stability of the explicit scheme in 2D, the Courant-Friedrichs-Lewy (CFL) condition requires:

$$r = \frac{\alpha \Delta t}{h^2} \le \frac{1}{4}$$

### 1.3 Parameters Used by the Solvers
All three solvers use the same constants and boundary conditions:

| Parameter | Value |
| :--- | :---: |
| **$\alpha$** | 1.0 |
| **$\Delta t$** | 0.2 |
| **$h$** | 1.0 |
| **$r$** | 0.2 (within the CFL bound) |
| **Top row** | 100 |
| **Bottom row** | 0 |
| **Left and right columns** | 50 (including the corners) |
| **Interior start value** | 0 |
| **Iteration limit** | 25,000 |

The grid size (default 200 × 200, from 3 × 3 to 100,000 × 100,000) and $\varepsilon$ (default 0.01) are command-line arguments. Each iteration updates every interior cell and records the largest change. The simulation stops after the first iteration in which

$$\max_{i,j} |u_{i,j}^{t+1} - u_{i,j}^t| \le \varepsilon$$

or after 25,000 iterations, whichever comes first.

---

## 2. Parallelization Strategies

### 2.1 Domain Decomposition (1D Row Slicing)
The $R \times C$ grid is stored as one flat array of doubles in row-major order (`idx = i * cols + j`). The pthreads solver splits the $R - 2$ interior rows among $P$ threads in contiguous blocks. Each thread gets $\lfloor (R-2)/P \rfloor$ rows, and the first $(R-2) \bmod P$ threads get one extra row:

```text
+------------------------------------------------+  Row 0 (boundary, 100)
|                   Thread 0                     |  rows_per_thread (+1 while remainder lasts)
+------------------------------------------------+
|                   Thread 1                     |
+------------------------------------------------+
|                      ...                       |
+------------------------------------------------+
|                   Thread P-1                   |  last interior rows, up to row R-2
+------------------------------------------------+  Row R-1 (boundary, 0)
```

For example, 10 interior rows and 4 threads give blocks of 3, 3, 2 and 2 rows. The threads share memory, so there are no halo or ghost rows: a thread reads the rows just above and below its block directly from the shared `current` array. When there are more threads than interior rows, the extra threads get no rows and only take part in the barriers.

### 2.2 POSIX Threads Implementation
- **Double Buffering:** Each iteration reads from `current` and writes to `next` (fields of `SharedSimulation`). The two pointers are swapped after every iteration, so no memory is allocated per step.
- **Barrier Synchronization (`pthread_barrier_t`):** One barrier, sized to the thread count, is used twice per iteration:
  1. *After the computation:* Every thread has written its rows of `next` and its local maximum change in `thread_max_deltas[tid]`.
  2. Between the two barriers, thread 0 alone takes the maximum of `thread_max_deltas`, sets `converged` when it is at most $\varepsilon$, and swaps `current` and `next`.
  3. *After the reduction:* All threads see the swapped pointers and the `converged` flag before they check it and start the next iteration.

### 2.3 OpenMP Implementation
The OpenMP solver parallelizes the loop over interior rows. Each thread keeps a private maximum that OpenMP combines at the end of the loop:

```c
#pragma omp parallel for reduction(max:max_delta) schedule(static)
for (long long i = 1; i < rows - 1; ++i) {
  long long row_offset = i * cols;
  for (long long j = 1; j < cols - 1; ++j) {
    long long idx = row_offset + j;
    double u_ij = current[idx];
    double u_next = u_ij + r * (
      current[idx - cols] +
      current[idx + cols] +
      current[idx - 1] +
      current[idx + 1] -
      4.0 * u_ij
    );
    next[idx] = u_next;

    double delta = fabs(u_next - u_ij);
    if (delta > max_delta) {
      max_delta = delta;
    }
  }
}
```

The pointers are swapped after each parallel loop. With the same grid and $\varepsilon$, the serial, pthreads and OpenMP solvers run the same number of iterations and report the same final maximum change.

---

## 3. Scaling & Performance Metrics

Each solver prints its elapsed time and throughput in megacells per second. Comparing the runs gives:

- **Speedup:** $S(P) = \frac{T_1}{T_P}$
- **Parallel Efficiency:** $E(P) = \frac{S(P)}{P} = \frac{T_1}{P \cdot T_P}$
- **Amdahl's Law Limit:**
  $$S(P) = \frac{1}{(1 - f) + \frac{f}{P}}$$
  where $f$ is the parallelizable fraction of the run time. The barrier waits and the serial reduction done by thread 0 in each iteration belong to the serial part, so their share grows as the grid gets smaller.
