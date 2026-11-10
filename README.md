# Quantinnium Solver

A bounded, auditable solver-invariance prototype. This initial implementation establishes a protocol contract, a validated worker skeleton, and a runtime-gated AVX-512 microbenchmark. It does not include a POSIX IPC adapter, domain-specific solver, hardware actuation, or autonomous production mutation.

## Build and test

Requires CMake 3.20+ and a C++23-capable GCC or Clang toolchain.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/avx512_kernel_bench
```

Optional benchmark arguments are vector length and iterations, for example `./build/avx512_kernel_bench 1048576 100`.

## Components

- `docs/shared-memory-protocol.md`: proposed one-request-at-a-time POSIX IPC contract.
- `docs/daemon-state-machine.md`: worker states, validations, and failure semantics.
- `src/upsm_solver_daemon.cpp`: transport-agnostic worker and demo solver. The demo solver is not a domain solver.
- `tests/worker_tests.cpp`: validation and solver-output smoke tests.
- `benchmarks/avx512_kernel_bench.cpp`: scalar and runtime-gated AVX-512 dot-product benchmark with a correctness check.
- `docs/implementation-plan.md`: staged implementation and release gates.

## Safety

The worker skeleton is not a complete daemon: there is no production transport adapter yet. Do not connect this baseline to production state or physical equipment. A real integration must provide an independently reviewed protocol adapter, domain-specific invariants, recovery, resource limits, and human approval for consequential actions. Benchmark output is host-specific and is not a latency or speedup guarantee.
