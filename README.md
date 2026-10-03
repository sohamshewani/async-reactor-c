# async-reactor-c

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Stars](https://img.shields.io/github/stars/sohamshewani/async-reactor-c?style=flat)](https://github.com/sohamshewani/async-reactor-c)
[![Language: C11](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

`async-reactor-c` is a high-performance, non-blocking asynchronous event loop and network reactor engine written in modern C11. It is engineered from the ground up for low-latency, high-throughput I/O multiplexing, utilizing modular buffering primitives and a deterministic event demultiplexing core designed to scale seamlessly under heavy concurrent network loads.

---

## Architecture & Problem Statement

### The Problem
Traditional synchronous, thread-per-connection network models scale poorly under high concurrency. Operating system thread limits, excessive context-switching overhead, and stack memory consumption severely bottleneck performance when handling tens of thousands of simultaneous network connections. Furthermore, naive non-blocking I/O loops often suffer from busy-waiting, complex state fragmentation, and buffer management vulnerabilities (such as overflows and memory fragmentation).

### The Solution: The Reactor Pattern
`async-reactor-c` implements the **Reactor Design Pattern**. It decouples synchronous event demultiplexing (via underlying OS polling mechanisms like `epoll`/`kqueue`/`poll`) from asynchronous event handling. 

The architecture is strictly partitioned into four cohesive subsystems:
1. **Reactor Core (`include/reactor.h`)**: Manages the central event loop, tracks file descriptors, and dispatches readiness events (readable, writable, error) to registered callbacks.
2. **Server Engine (`include/server.h`)**: Orchestrates network lifecycle management, binding, listening, connection acceptance, and teardown logic atop the reactor.
3. **Dynamic Buffer Management (`include/buffer.h`)**: Provides safe, zero-copy-friendly, dynamically resizing I/O buffers with automatic boundary checks and memory recycling to prevent fragmentation and buffer-overflow vectors.
4. **Test & Verification Suite (`tests/test_engine.c`)**: Validates event loop timing invariants, stress-tests buffer expansions, and confirms deterministic callback sequencing.

---

## ASCII Architecture & Dataflow Diagram

```
 +-----------------------------------------------------------------------+
 |                         Client / Network Stream                       |
 +-----------------------------------------------------------------------+
                                     |
                                     v (TCP Packets)
 +-----------------------------------------------------------------------+
 |                       OS Kernel (TCP/IP Stack)                        |
 +-----------------------------------------------------------------------+
                                     |
                                     v (File Descriptor Readiness)
 +-----------------------------------------------------------------------+
 |                     Reactor Core (include/reactor.h)                  |
 |                   [ Multiplexing Demultiplexer Engine ]               |
 +-----------------------------------------------------------------------+
        |                                       |
        | (Event Dispatched)                    | (Event Dispatched)
        v                                       v
 +----------------------------+      +-----------------------------------+
 |   Server Event Handlers    |      |         Buffer Subsystem          |
 |   (include/server.h)       | <--> |        (include/buffer.h)         |
 |   - Accept Connections     |      |  - Dynamic Resizing               |
 |   - Read / Write Streams   |      |  - Safe Memory Bounds             |
 +----------------------------+      +-----------------------------------+
```

### Dataflow Execution Trace
1. **Poll Phase**: The reactor blocks on the multiplexing readiness call, waiting for registered file descriptors to become readable or writable.
2. **Demultiplexing Phase**: Upon OS notification, the reactor identifies triggered file descriptors and retrieves their associated event contexts.
3. **Dispatch Phase**: The reactor invokes the specific event callback registered in the server module (`include/server.h`).
4. **Buffer Interaction**: The callback reads raw stream bytes into or flushes bytes out of the managed dynamic buffers (`include/buffer.h`), ensuring memory safety and preventing partial-read data corruption.

---

## Algorithmic Decisions & Concurrency Invariants

### 1. $O(1)$ Event Registration and Dispatch
Event tracking structures maintain direct mappings between file descriptors and event handlers. Operations for registering, modifying, or unregistering file descriptors run in bounded time complexity, preventing loop starvation during connection storms.

### 2. Bounded Dynamic Buffering
The buffer subsystem (`include/buffer.h`) employs a doubling-capacity reallocation strategy when write limits exceed current thresholds. To prevent unbounded memory consumption attacks (Slowloris/memory exhaustion), the engine enforces strict upper bounds on individual buffer allocations.

### 3. Non-Blocking Invariant
All sockets handled by the reactor are explicitly configured into non-blocking mode (`O_NONBLOCK`). Handlers are guaranteed never to block the event loop thread on read/write operations; if an `EAGAIN` or `EWOULDBLOCK` status is encountered, control yields immediately back to the reactor loop.

### 4. Memory Safety & Deterministic Teardown
Resource acquisition is tightly coupled with explicit lifecycle APIs. Connections that encounter fatal errors or EOF conditions trigger immediate cleanup routines that deregister file descriptors from the reactor poll set prior to socket closure, preventing resource leaks.

---

## Build, Compilation & Test Execution Recipes

`async-reactor-c` ships with a production-ready `Makefile` configured with strict compiler warnings, optimization flags, and debugging instrumentation.

### Prerequisites
* **C Compiler**: GCC or Clang supporting C11 standard.
* **Build System**: GNU Make.
* **Operating System**: POSIX-compliant system (Linux/macOS recommended).

### Compilation Recipes

* **Build Everything (Release Mode)**:
  ```bash
  make
  ```

* **Build with Debug Symbols and Sanitizers**:
  ```bash
  make DEBUG=1
  ```

* **Clean Build Artifacts**:
  ```bash
  make clean
  ```

### Test Execution Recipe

To execute the test suite (`tests/test_engine.c`) and verify engine stability, memory safety, and buffer correctness:

```bash
make test
```

*(Optional: If running under Valgrind to verify zero memory leaks)*:
```bash
valgrind --leak-check=full --error-exitcode=1 ./bin/test_engine