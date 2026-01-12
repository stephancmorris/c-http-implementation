**C-HTTP Payment Server** is a multi-threaded, Idempotent HTTP Server, and non-blocking HTTP server built from scratch in C. 

Unlike standard web servers, C-HTTP Payment Server is engineered specifically for **transactional reliability**. It implements an **application-agnostic Idempotency Layer** directly into the server lifecycle, guaranteeing that critical requests (like payment captures or ledger updates) are never processed twice, even in the event of network retries or client failures.

## Key Features

* **Native Idempotency:** Supports `Idempotency-Key` headers out of the box. Automatically caches and replays responses for duplicate keys.
* **Zero-Dependency C:** Built using standard libraries (`<pthread.h>`, `<sys/socket.h>`) to demonstrate core OS fundamentals.
* **Thread Pool Architecture:** Pre-allocated worker threads to handle high-concurrency loads without the overhead of fork().
* **Thread-Safe State:** Custom implementation of a concurrent Hash Map using read-write locks (`pthread_rwlock_t`) for high-performance key lookups.

## Getting Started

### Prerequisites
* GCC or Clang
* Make
* Linux/Unix environment (or WSL)
