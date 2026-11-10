# Implementation plan

## Phase 0 — Contract and threat model

- Review and freeze the wire format, slot ownership, permissions, limits, timeout behavior, and restart semantics in `docs/shared-memory-protocol.md`.
- Document supported OS/CPU/compiler combinations and whether untrusted local peers are in scope.
- Keep hardware actuation, production mutation, and automatic patch deployment disabled.

**Gate:** protocol review and cross-language golden encoding/decoding tests.

## Phase 1 — Correctness-first worker

- Keep the transport/solver boundary in `src/upsm_solver_daemon.cpp`.
- Test bad generation, empty/oversized frames, NaN/Inf, solver exception, wrong output count, and out-of-range actions.
- Implement domain-specific independent invariant verification before any production commit.

**Gate:** no failed or unverified frame can be acknowledged as committed.

## Phase 2 — POSIX transport adapter

- Implement shared-memory lifecycle and a narrow C ABI adapter with restrictive permissions and strict limits.
- Implement `SOCK_SEQPACKET` messages, process-shared slot synchronization, generation matching, timeouts, peer checks, and failure acknowledgements.
- Snapshot frame data before solve; preserve one-in-flight backpressure.
- Add malformed-message fuzzing and fault injection for producer/daemon termination at every state.

**Gate:** producer interoperability, restart, stale-generation, checksum, timeout, and resource-cleanup tests.

## Phase 3 — Domain solver and verifier

- Specify objective, constraints, units, tolerances, and failure policy.
- Replace demo behavior with a domain-specific solver and independent verifier.
- Preserve a separately durable last-known-good checkpoint if rollback is needed.

**Gate:** known-answer, boundary, adversarial-input, regression, and recovery tests reviewed by domain owners.

## Phase 4 — SIMD optimization

- Compare scalar, compiler-autovectorized, AVX2, and AVX-512 paths with runtime dispatch and a scalar fallback.
- Validate numerical error against a reference implementation.
- Record CPU, compiler/flags, OS, data shape, warm-up, sample distribution, and tail latency.
- Measure end-to-end solver latency, including copies, page faults, memory layout, and NUMA effects.

**Gate:** repeatable end-to-end benefit on target hardware without correctness regression. Do not gate CI performance on shared runners.

## Phase 5 — Operations and release

- Add structured metrics, bounded logs, graceful shutdown, system resource limits, CI, sanitizers, and runbooks.
- Require security and domain review plus human approval for consequential state changes or physical actuation.

**Gate:** documented recovery drills, reviewed threat model, and release sign-off.
