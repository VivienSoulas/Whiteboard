# CLAUDE.md

Guidance for Claude Code when working in this repository.

## Project

C++11 HTTP/1.0 + HTTP/1.1 web server (42 project).

NGINX-like architecture:
- non-blocking I/O
- `poll()` event loop
- request parsing → routing → handler dispatch

Frontend allows interaction with vector graphics on a whiteboard.

## Stack

C++, C, JavaScript, Node.js

## Build

```bash
make        # build → out/webserv (-O2)
make debug  # build with -g -DDEBUG=1
make run    # build + run config/default.conf
make test   # run tests (server must run on :8082)
make clean
make fclean
make re

Compiler: C++11
Flags: -Wall -Wextra -Werror (STRICT=0 disables strict)

Core Flow
main
 └─ Webserv
     ├─ ConfigFile → Config → ServerConfig[] → LocationConfig[]
     ├─ Router
     └─ ConnectionManager (poll loop)
          ├─ Socket[] (listen)
          ├─ Connection[]
          └─ HttpParser[]
                ↓
            handleRequest()

Connection states:

READING → WRITING → CLOSING

Parser states:

READING_HEADERS → READING_BODY → COMPLETE
Request Handling

Handlers selected after routing:

StaticFileHandler (GET, HEAD, directory listing)

CgiHandler (.php, .py, custom)

UploadHandler (multipart/form-data)

DELETE handler

Redirect handler (301, 302)

Source Layout
include/config/        config parsing
include/http/          HTTP protocol
include/http/multipart multipart parsing
include/io/            sockets + connection manager
include/               router, utils, logger, webserv

src/                   implementations
config/default.conf    example config (ports 8080–8082)
assets/                static + CGI + uploads
Event Loop

ConnectionManager:

poll()

accept new clients

read client sockets

parse requests

dispatch handlers

write responses

close finished connections

Key Classes

Webserv server orchestrator
Router host/path routing
ConnectionManager poll loop + connection lifecycle
Connection client state + buffers
HttpParser streaming request parser
HttpRequest parsed request
HttpResponse response builder

Coding Rules

Prefer:

small functions

deterministic logic

minimal abstraction

explicit control flow

Avoid:

unnecessary comments

large refactors unless asked

speculative improvements

Do Not Modify

public interfaces unless requested

config format

build system

poll() based architecture

Debug Strategy

Prefer:

logging

minimal patches

reproduce with curl

Example:

curl -v http://localhost:8080/
Response Rules

Default output:

code only

minimal formatting

no instruction restatement

no explanations

Explain only if asked.
