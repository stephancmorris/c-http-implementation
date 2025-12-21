# High-Reliability Idempotent HTTP Server

![Language](https://img.shields.io/badge/language-C11-blue)
![Status](https://img.shields.io/badge/status-active-success)
![Focus](https://img.shields.io/badge/focus-distributed%20systems-orange)

**NanoServe** is a multi-threaded, non-blocking HTTP server built from scratch in C. 

Unlike standard web servers, NanoServe is engineered specifically for **transactional reliability**. It implements an **application-agnostic Idempotency Layer** directly into the server lifecycle, guaranteeing that critical requests (like payment captures or ledger updates) are never processed twice, even in the event of network retries or client failures.

## 🚀 Motivation

In distributed payment systems (e.g., ISO8583 gateways, payment switches), network timeouts often cause clients to retry requests. Without idempotency, this leads to "double-spend" anomalies. 

While many developers solve this at the *application* layer (database constraints), NanoServe solves it at the *infrastructure* layer. By handling deduplication in memory before the request reaches the business logic, we reduce database load and guarantee consistency at the protocol level.

## ✨ Key Features

* **Native Idempotency:** Supports `Idempotency-Key` headers out of the box. Automatically caches and replays responses for duplicate keys.
* **Zero-Dependency C:** Built using standard libraries (`<pthread.h>`, `<sys/socket.h>`) to demonstrate core OS fundamentals.
* **Thread Pool Architecture:** Pre-allocated worker threads to handle high-concurrency loads without the overhead of fork().
* **Thread-Safe State:** Custom implementation of a concurrent Hash Map using read-write locks (`pthread_rwlock_t`) for high-performance key lookups.

## 🏗 System Architecture

### 1. The Request Lifecycle
1.  **Accept:** Main thread accepts connection and pushes the file descriptor to a **Task Queue**.
2.  **Worker:** A worker thread picks up the task.
3.  **Parse:** Custom HTTP parser extracts headers, specifically looking for `X-Idempotency-Key`.
4.  **Idempotency Check (The "Guard"):**
    * **Hit:** If the key exists, the server immediately returns the *cached response* (e.g., `200 OK`) without executing the handler.
    * **Miss:** The server marks the key as `PENDING` and proceeds.
5.  **Process:** The business logic executes.
6.  **Cache:** The final response is stored in the Idempotency Map associated with the key.
7.  **Respond:** Response sent to client.

### 2. The Idempotency Engine (Specs)
The core differentiator is a custom thread-safe Hash Map designed for this server.

* **Data Structure:** Chained Hash Table.
* **Collision Resolution:** Linked Lists.
* **Concurrency Control:** * Fine-grained locking (bucket-level mutexes) to reduce contention.
    * Atomic operations for status flags (`PENDING`, `COMPLETED`).
* **Memory Management:** Automatic TTL (Time-To-Live) cleanup mechanism to prevent memory exhaustion from old keys.

## 🛠 Technical Stack

* **Language:** C (C11 Standard)
* **Build System:** Make
* **Testing:** Unit tests (custom harness) + Integration tests (Python/requests)
* **Tools:** Valgrind (Memcheck), GDB

## 📦 Getting Started

### Prerequisites
* GCC or Clang
* Make
* Linux/Unix environment (or WSL)

### Installation
```bash
git clone [https://github.com/stephanmorris/nanoserve.git](https://github.com/stephanmorris/nanoserve.git)
cd nanoserve
make
