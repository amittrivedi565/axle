# AXLE Gateway Documentation

Welcome to the **API Gateway** project documentation.

This project implements a lightweight HTTP/1.1 API Gateway in C++ using **kqueue/epoll** event-driven architecture.

## Overview

- **Language:** C++
- **Architecture:** Non-blocking I/O event loop
- **Purpose:** Acts as an entry point for routing and proxying API requests to backend microservices, rate limiting, caching, circuit-breaker.

## Docs Navigation

- [Architecture](architecture.md)
- [HTTP Flow](http_flow.md)
- [Build Instructions](build.md)

## Compile Command
- `bin` directory will be required for compilation, after creating run this below command.
- `g++ main.cpp -o bin/main`