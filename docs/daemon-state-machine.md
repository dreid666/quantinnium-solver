# Daemon worker state machine

This document describes a bounded worker, not a complete production IPC daemon.

## Responsibilities

- **Transport adapter:** validates wire framing, peer/request generation, bounds, and checksum; returns a privately owned input snapshot; performs bounded, matching-generation acknowledgements.
- **Worker:** validates application-level input, calls the solver, validates result shape/ranges, and submits either a committed result or explicit failure.
- **Domain verifier:** independently checks domain constraints, residual tolerances, and safety rules before commit. This must be implemented for the actual solver domain.
- **Recovery controller:** handles timeout, process loss, stale generation, and failed verification without committing partial output.

## Lifecycle

```text
START -> WAIT_FOR_FRAME -> VALIDATE_INPUT -> SOLVE -> VALIDATE_RESULT
                                                -> CHECK_DOMAIN_INVARIANTS
                                                   -> ACK_COMMITTED -> WAIT_FOR_FRAME
                                                   -> ACK_FAILED -> WAIT_FOR_FRAME
Any ambiguous transport failure -> RECOVERY_REQUIRED
```

A frame can be committed only if its generation is nonzero and current, the node count and values are valid, the solver output has the expected count and range, and domain-specific verification succeeds. Finite output alone is not proof of correctness.

The worker catches solver exceptions and represents failure explicitly. The transport must distinguish timeout, shutdown, peer loss, and malformed input in its concrete implementation; the abstract skeleton keeps this adapter out of scope.

## Restart and shutdown

On restart, consider all in-flight generations uncommitted. Do not infer success from stale memory. Shutdown should stop accepting new requests, finish or fail the active request within a deadline, send an acknowledgement if possible, and release owned resources.

The included largest-magnitude demo solver is a smoke-test example only. It must not be used as a production counterplay or control algorithm.
