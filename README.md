# 42 Webserver

A high-performance, lightweight webserver built for the 42 project.  
Features include efficient request multiplexing with `epoll`, CGI execution, advanced session management with multi-cookie support, and flexible multi-virtual hosting via a user-friendly configuration file.

---

## Table of Contents

- [Features](#features)
- [Getting Started](#getting-started)
- [Configuration File](#configuration-file)
  - [Request Handling](#request-handling)
  - [Response Generation](#response-generation)
  - [Multiplexing with epoll](#multiplexing-with-epoll)
  - [CGI Execution](#cgi-execution)
  - [Session Management (Multi-Cookie Support)](#session-management-multi-cookie-support)
  - [Virtual Hosting](#virtual-hosting)
- [Performance](#performance)

---

## Features

- 🚀 **Lightweight and Fast:** Minimal dependencies, efficient networking, optimized for speed and low resource usage.
- ⚡ **Epoll-based Multiplexing:** Handles thousands of concurrent connections using the Linux `epoll` API.
- 🔌 **CGI Support:** Run external scripts (Python, PHP, etc.) for dynamic content.
- 🍪 **Multi-Cookie Session Management:** Robust per-user session handling using multiple cookies.
- 🌐 **Multi Virtual Hosting:** Host multiple domains with distinct configurations via a single config file.
- 🛠️ **Easy Configuration:** Human-readable config format for easy server and virtual host management.

---

## Getting Started

1. **Clone the repository:**
```bash
   git clone https://github.com/your-username/42-webserver.git
   cd 42-webserver
```

2. **Build:**
```bash
   make
```

3. **Run:**
```bash
   ./webserv path/to/config.conf
```

---

## Configuration File

Define servers, ports, document roots, server names, CGI handlers, and more.

``` INI
[server]
    host = localhost
    port = 8080
    server_names = ww.com ayoub.com 
    client_body_limit = 10000000000M

[error_pages]
    400 = www/errors/400.html
    404 = www/errors/404.html
    405 = www/errors/405.html
    411 = www/errors/411.html
    500 = www/errors/500.html
    501 = www/errors/501.html
    505 = www/errors/505.html

[route /]
    alias = www/html/
    index = index.html
    methodes = GET, POST
    allow_upload = true
    upload_directory = upload/

```

**Highlights:**
- `server_name`: Enables multi-virtual hosting.
- `location`: Maps URL paths to filesystem or CGI.
- `cgi`: Configures cgi support for that location.

---

### Request Handling

- Accepts connections and reads HTTP requests using non-blocking sockets.
- Parses HTTP/1.1 requests, including headers, cookies, and query strings.

### Response Generation

- Generates static file responses, directory listings, or forwards to CGI.
- Handles HTTP status codes, custom error pages, and header management.

### Multiplexing with epoll

- Uses the Linux `epoll` API for event-driven, non-blocking I/O.
- Efficiently manages thousands of simultaneous client connections.

### CGI Execution

- Supports execution of external CGI scripts (Python, PHP, etc.).
- Passes environment variables and request data, collects script output as HTTP response.

### Session Management (Multi-Cookie Support)

- Issues and validates multiple cookies per session (e.g., for authentication, preferences).
- Secure, HTTP-only, and optionally secure cookies for robust session handling.

### Virtual Hosting

- Serve multiple domains from the same server instance.
- Each domain can have unique root, index, error pages, and CGI settings.

---

## Performance

Our server is designed for **maximum throughput** and **minimal latency**, making it ideal for production and educational use.
