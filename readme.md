# HTTP Server from Scratch (C-HTTP)

![Build Status](https://img.shields.io/badge/build-passing-brightgreen) ![License](https://img.shields.io/badge/license-MIT-blue)

**NanoServe** is a lightweight, multi-threaded HTTP server written entirely in C from scratch. It is designed to demonstrate low-level systems programming concepts, including socket networking, thread pools, and memory management without relying on external networking libraries.

## 🏗 Architecture

NanoServe utilizes a **Thread Pool** architecture to handle concurrent connections efficiently.
1.  **Listener:** Binds to port 8080 and listens for incoming TCP connections.
2.  **Dispatcher:** Accepts connections and pushes file descriptors to a synchronized queue.
3.  **Workers:** A pool of pre-spawned threads pick up tasks, parse the HTTP request, and serve static resources.

### Application Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              NanoServe Architecture                          │
└─────────────────────────────────────────────────────────────────────────────┘


   ┌──────────┐     ┌──────────┐     ┌──────────┐
   │ Client 1 │     │ Client 2 │     │ Client N │
   └────┬─────┘     └────┬─────┘     └────┬─────┘
        │                │                 │
        │   HTTP         │   HTTP          │   HTTP
        │   Request      │   Request       │   Request
        │                │                 │
        └────────────────┴─────────────────┘
                         │
                         ▼
        ┌────────────────────────────────────┐
        │       LISTENER (Main Thread)       │
        │  - socket()                        │
        │  - bind(port 8080)                 │
        │  - listen(backlog=128)             │
        └────────────────┬───────────────────┘
                         │
                         ▼
        ┌────────────────────────────────────┐
        │     DISPATCHER (Accept Loop)       │
        │  - accept() incoming connections   │
        │  - Get client socket FD            │
        │  - Log client IP:port              │
        └────────────────┬───────────────────┘
                         │
                         │ queue_push(client_fd)
                         ▼
        ┌────────────────────────────────────┐
        │   SYNCHRONIZED TASK QUEUE (FIFO)   │
        │  ┌──────────────────────────────┐  │
        │  │ [FD1] [FD2] [FD3] ... [FDN] │  │
        │  └──────────────────────────────┘  │
        │  - Mutex protected                 │
        │  - Condition variable signaling    │
        │  - Bounded size (256)              │
        └────────────┬───────────────────────┘
                     │
                     │ queue_pop() (blocking)
                     │
        ┌────────────┴───────────────────────┐
        │                                     │
   ┌────▼──────┐  ┌──────────┐  ┌──────────┐│
   │ Worker 1  │  │ Worker 2 │  │ Worker N ││
   │ (pthread) │  │ (pthread)│  │ (pthread)││
   └────┬──────┘  └────┬─────┘  └────┬─────┘│
        │              │              │
        │   THREAD POOL (Pre-spawned Workers)
        │              │              │
        └──────────────┴──────────────┘
                       │
                       │ Each worker performs:
                       ▼
        ┌────────────────────────────────────┐
        │    REQUEST HANDLER (Worker Loop)   │
        │                                    │
        │  1. recv() - Read HTTP request     │
        │     ┌──────────────────────────┐   │
        │     │ GET /index.html HTTP/1.1 │   │
        │     │ Host: localhost:8080     │   │
        │     └──────────────────────────┘   │
        │                                    │
        │  2. HTTP PARSER                    │
        │     - Parse request line           │
        │     - Parse headers                │
        │     - Validate request             │
        │                                    │
        │  3. ROUTE HANDLER                  │
        │     - Map URI to file path         │
        │     - Security: check traversal    │
        │                                    │
        │  4. FILE SERVER                    │
        │     - Open file (fopen)            │
        │     - Read file content            │
        │     - Detect MIME type             │
        │                                    │
        │  5. RESPONSE BUILDER               │
        │     - Set status code (200/404)    │
        │     - Add headers (Content-Type)   │
        │     - Set response body            │
        │                                    │
        │  6. send() - Send HTTP response    │
        │     ┌──────────────────────────┐   │
        │     │ HTTP/1.1 200 OK          │   │
        │     │ Content-Type: text/html  │   │
        │     │ Content-Length: 1234     │   │
        │     │                          │   │
        │     │ <html>...</html>         │   │
        │     └──────────────────────────┘   │
        │                                    │
        │  7. close(client_fd)               │
        │  8. Loop back to queue_pop()       │
        └────────────────────────────────────┘
                       │
                       ▼
                  ┌─────────┐
                  │ Client  │
                  │ Receives│
                  │ Response│
                  └─────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                            Key Components                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│ • Listener:    Single thread managing the TCP socket                        │
│ • Dispatcher:  Distributes connections to worker threads                     │
│ • Queue:       Thread-safe FIFO with mutex + condition variables            │
│ • Workers:     Pool of N threads (default: 8) processing requests           │
│ • Parser:      HTTP/1.1 request parsing (method, URI, headers)              │
│ • File Server: Static file serving with MIME type detection                 │
│ • Logger:      Thread-safe logging subsystem                                │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                         Concurrency Model                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Main Thread:         Listener + Dispatcher                                 │
│  Worker Threads:      [T1] [T2] [T3] [T4] [T5] [T6] [T7] [T8]              │
│                                                                              │
│  Synchronization:     Mutex (queue access)                                  │
│                       Condition Variable (queue empty/full)                 │
│                                                                              │
│  Request Flow:        Client → Listener → Queue → Worker → Response         │
│  Processing Time:     ~1-10ms per request (depending on file size)          │
│  Concurrency:         Up to N simultaneous requests (N = worker count)      │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 🚀 Getting Started

### Prerequisites
* **Compiler**: GCC or Clang with C99 support
* **Build Tool**: GNU Make
* **Platform**: Linux or macOS (POSIX compliant)
* **Optional**: Valgrind (for memory leak detection)

### Project Structure

```
c-http-implementation/
├── src/                    # Source files (.c)
│   └── main.c             # Entry point
├── include/               # Header files (.h)
│   ├── listener.h         # TCP listener
│   ├── dispatcher.h       # Connection dispatcher
│   ├── queue.h            # Thread-safe queue
│   ├── threadpool.h       # Worker thread pool
│   ├── http_parser.h      # HTTP request parser
│   ├── http_response.h    # HTTP response builder
│   ├── fileserver.h       # Static file server
│   ├── logger.h           # Logging subsystem
│   └── utils.h            # Utility functions
├── bin/                   # Build output (executable)
├── www/                   # Document root (static files)
│   └── index.html         # Default welcome page
├── tests/                 # Unit tests
├── Makefile              # Build configuration
└── README.md             # This file
```

### Build and Run

**Clone and build:**
```bash
git clone https://github.com/YOUR_USERNAME/c-http-implementation.git
cd c-http-implementation
make
```

**Run the server:**
```bash
./bin/nanoserve
```

**Test in browser:**
```
http://localhost:8080
```

**Test with curl:**
```bash
curl http://localhost:8080
curl http://localhost:8080/index.html
```

### Build Targets

* `make` or `make all` - Build the server
* `make clean` - Remove build artifacts
* `make debug` - Build with debug symbols (`-g`)
* `make run` - Build and run the server
* `make test` - Run unit tests (when implemented)

### Configuration

**Port**: Default is 8080 (configurable in code or via command-line)
**Worker Threads**: Default is 8 threads
**Document Root**: `./www` directory
**Queue Size**: 256 connections

### Development Status

This project is currently in development. Check [TASKS.md](TASKS.md) for the implementation roadmap.

**Completed:**
- ✅ Project structure and build system setup
- ✅ Architecture design and documentation

**In Progress:**
- 🔄 Core component implementation
- 🔄 HTTP protocol handling
- 🔄 Static file serving

**Planned:**
- ⏳ Error handling and logging
- ⏳ Testing and validation
- ⏳ Performance optimization
