# AXLE Gateway Documentation

Welcome to the **Axle API Gateway** documentation.  

This project implements a **lightweight HTTP/1.1 API Gateway** in **C++**, built on an **event-driven architecture** using **kqueue/epoll**.  
It efficiently routes, proxies, and manages API requests between clients and microservices.

## Overview

| Key | Description |
|-----|--------------|
| **Language** | C++ |
| **Architecture** | Non-blocking I/O (event loop using kqueue/epoll) |
| **Purpose** | Acts as an entry point for routing, proxying, rate limiting, caching, and circuit breaking for backend microservices |


## Documentation Navigation

- [Architecture](architecture.md)  
- [HTTP Flow](http_flow.md)  
- [Build Instructions](build.md)  



## Compilation

Before compiling, ensure a `bin` directory exists in your project root.

Run the following command to compile the gateway:

```bash
g++ -std=c++17 main.cpp -o bin/main

```

## Saved configuration Structure

```saved
{
  "user-service": {
    "s_name": "user-service",
    "s_host": "127.0.0.1",
    "s_port": 5000,
    "d_exp": "PUBLIC",          // Default exposure: PUBLIC | PRIVATE
    "d_auth": true,             // Default auth requirement for routes

    "custom_routes": {
      "/orders/:id:POST": {
        "r_path":   "/orders/:id",
        "r_method": "POST",
        "r_exp":    "PROTECTED",
        "r_auth":   false
      },

      "/orders/:id:GET": {
        "r_path":   "/orders/:id",
        "r_method": "GET",
        "r_exp":    "PROTECTED",
        "r_auth":   false
      }
    }
  }
}
```