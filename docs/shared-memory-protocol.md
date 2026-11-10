# Shared-memory protocol proposal (UPSM v1)

## Scope and trust boundary

Use POSIX shared memory only for bulk request/result bytes. Use an `AF_UNIX` `SOCK_SEQPACKET` socket as the control plane for request notifications, acknowledgements, health checks, and shutdown. Never make compiler-dependent C++ objects (`std::atomic`, pointers, `std::string`, native structs with implicit padding) part of the cross-language ABI.

The first implementation is single-producer/single-consumer with one request in flight. The producer owns the request slot until it receives a matching acknowledgement. The daemon validates metadata and copies the request into private memory before solving. Do not add concurrent slots before the single-slot protocol passes crash/restart tests.

## Wire format

Specify byte offsets and encode/decode fields explicitly. All integers are little-endian; floating-point payload values are IEEE-754 binary32. A v1 header contains:

| Field | Type | Rule |
|---|---|---|
| `magic` | 8 bytes | `QNSUPSM1` |
| `protocol_version` | `uint16` | exactly 1 |
| `header_bytes` | `uint16` | exactly the v1 header size |
| `slot_bytes` | `uint32` | exactly the mapped slot size |
| `generation` | `uint64` | nonzero and strictly increasing per producer lifetime |
| `payload_bytes` | `uint32` | bounded; must match shape |
| `node_count` | `uint32` | in configured range; never silently clamped |
| `state_dimension` | `uint32` | exactly the configured dimension |
| `payload_checksum` | `uint32` | CRC32C for accidental corruption only |
| `reserved` | fixed bytes | all zero in v1 |

Reject invalid magic/version, lengths, arithmetic overflow, unsupported dimensions, nonzero reserved bytes, stale/replayed generations, checksum mismatch, and non-finite numeric values. Set a strict maximum mapping and payload size before allocation.

CRC is not authentication. Restrict shared-memory and socket permissions, validate peer credentials where available, and do not treat a checksum as protection from a malicious local peer.

## Ownership and exchange

1. Producer writes the entire frame to the shared slot while holding the documented interprocess synchronization primitive.
2. Producer releases that primitive and sends `SUBMIT(generation)` over the control socket.
3. Daemon receives the message, validates the referenced frame and generation, and copies it into private memory.
4. Producer does not modify the outstanding slot until a matching response arrives.
5. Daemon computes and independently validates the candidate result.
6. Daemon publishes result bytes and sends exactly one `ACK_COMMITTED(generation)` or `ACK_FAILED(generation, error_code)`.
7. Producer accepts only an acknowledgement matching its outstanding generation.

Do not use C++ `std::atomic` as a cross-process or Go/C++ synchronization contract. Define a process-shared POSIX synchronization primitive and access it through a narrow C ABI adapter, or otherwise specify and test the platform-specific synchronization contract. The control socket is the notification mechanism; its message alone does not make concurrent slot writes safe.

## State machine

```text
IDLE -> RECEIVED -> VALIDATING -> PROCESSING -> VERIFYING -> COMMITTED
                       |             |             |
                       +-------------+-------------+-> FAILED

Timeout, peer loss, restart with an in-flight generation -> RECOVERY_REQUIRED
```

Messages are fixed-size, versioned `SOCK_SEQPACKET` records: `SUBMIT`, `ACK_COMMITTED`, `ACK_FAILED`, `PING`, `PONG`, `SHUTDOWN`, and `SHUTDOWN_ACK`. Reject malformed, truncated, oversized, unexpected, or out-of-generation messages.

## Timeouts and restart

- Use monotonic-clock deadlines for receive, solve, and acknowledgement.
- On timeout, disconnect, malformed data, or solver failure, do not publish partial results. Acknowledge failure if possible and mark the generation uncommitted.
- On restart, never infer success from stale shared-memory bytes. Treat prior in-flight work as uncommitted; require explicit resubmission or a new generation.
- Shared memory is volatile, not a durable checkpoint. Persist a separately validated last-known-good state if rollback is required.
- With one in-flight request, producer backpressure means waiting for acknowledgement or timing out; never overwrite the outstanding slot.

## Operational requirements

Use restrictive permissions, validate object ownership and mapping size, set resource limits, and log generation, protocol version, solver version, duration, validation outcome, and failure reason without dumping raw payload by default. `mlock` and `MAP_POPULATE` are opt-in platform optimizations, not correctness or latency guarantees; benchmark them on the target system.

This proposal is Linux/POSIX-oriented. The wire encoding can be portable, but socket credentials, object lifecycle, and process-shared synchronization need platform-specific adapters and interoperability tests.
