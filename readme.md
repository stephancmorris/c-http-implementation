# HTTP Server from Scratch (C-HTTP)

![Build Status](https://img.shields.io/badge/build-passing-brightgreen) ![License](https://img.shields.io/badge/license-MIT-blue)

**NanoServe** is a lightweight, multi-threaded HTTP server written entirely in C from scratch. It is designed to demonstrate low-level systems programming concepts, including socket networking, thread pools, and memory management without relying on external networking libraries.

## 🏗 Architecture

NanoServe utilizes a **Thread Pool** architecture to handle concurrent connections efficiently.
1.  **Listener:** Binds to port 8080 and listens for incoming TCP connections.
2.  **Dispatcher:** Accepts connections and pushes file descriptors to a synchronized queue.
3.  **Workers:** A pool of pre-spawned threads pick up tasks, parse the HTTP request, and serve static resources.

## 🚀 Getting Started

### Prerequisites
* GCC or Clang
* Make
* Linux/macOS (POSIX compliant)

### Build and Run

```bash
git clone [https://github.com/YOUR_USERNAME/nanoserve-c.git](https://github.com/YOUR_USERNAME/nanoserve-c.git)
cd nanoserve-c
make
./bin/nanoserve
