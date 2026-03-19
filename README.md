# Whiteboard App

A lightweight in-browser whiteboard application served by a custom C++11 **HTTP/1.0 + HTTP/1.1 web server** with HTTPS/TLS support.

**Frontend**: Vector-based drawing canvas (pure JavaScript + SVG) for sketching, annotating, and visual collaboration.

**Backend**: High-performance non-blocking server with poll-based event loop, NGINX-like configuration, CGI support, and multipart file uploads.

This is a complete learning project implementing HTTP protocol semantics, socket I/O, SSL/TLS handshaking, and robust request/response handling from scratch in C++11.

---

## Requirements

- **C++11** compiler (g++ 4.8+)
- **OpenSSL** development libraries (for HTTPS support)
  - Ubuntu/Debian: `sudo apt-get install libssl-dev`
  - macOS: `brew install openssl`
- **make** build system
- **Python 3** (optional, for CGI board API)

---

## Quick Start

### Build

```bash
make                # Optimized build
make debug          # Debug build with symbols
make clean          # Remove object files
make fclean         # Remove all build artifacts
make re             # Clean rebuild
```

### Run (HTTP)

```bash
make run
# or manually:
./out/webserv config/default.conf
```

Then open in browser:

```
http://localhost:8080/
```

And navigate to the whiteboard at:

```
http://localhost:8080/whiteboard/
```

### Run (HTTPS)

First, generate a self-signed certificate (for testing):

```bash
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes
```

Then update `config/default.conf` to add:

```nginx
ssl_certificate ./cert.pem;
ssl_certificate_key ./key.pem;
ssl_port 8443;
```

Run the server:

```bash
./out/webserv config/default.conf
```

Access via HTTPS:

```
https://localhost:8443/whiteboard/
```

**Note**: Browser will warn about self-signed certificate (expected for testing).

---

## Whiteboard Features

### Drawing

- **Tools**: select, rectangle, ellipse, line, arrow, pen, text, sticky notes, and connectors
- **Direct manipulation**: create, select, move, resize, multi-select, align, duplicate, copy, paste, delete
- **Styling**: stroke color, fill color, stroke width, sticky note color categories, line/arrow endings
- **Text editing**: editable text and sticky note content inline on canvas

### Interaction

- **History**: undo/redo with keyboard shortcuts
- **Zoom & Pan**: mouse wheel zoom, middle-click or space-drag to pan
- **Selection**: marquee selection for multiple shapes
- **Persistence**: auto-save + load boards as JSON (local and server-side)
- **Export**: save as SVG or PNG

---

## Keyboard Shortcuts

### Tools

- `S` select, `R` rectangle, `E` ellipse, `L` line, `A` arrow, `P` pen, `T` text, `N` sticky note, `K` connector

### Editing

- `Ctrl/Cmd+Z` undo, `Ctrl/Cmd+Shift+Z` or `Ctrl/Cmd+Y` redo
- `Ctrl/Cmd+C` copy, `Ctrl/Cmd+V` paste, `Ctrl/Cmd+D` duplicate
- `Ctrl/Cmd+A` select all, `Ctrl/Cmd+ESC` clear selection

### Navigation

- Mouse wheel zoom, middle mouse drag or `Space+drag` pan

---

## Server Architecture

### Core Design

The server uses a **non-blocking I/O architecture** with a central **poll()-based event loop**:

```
main
 └─ Webserv
     ├─ Config (NGINX-like syntax)
     ├─ Router (virtual hosts + path matching)
     └─ ConnectionManager (event loop)
          ├─ Poll array (listeners + connections)
          ├─ HTTP connections (stateful)
          ├─ TLS connections (with handshake state)
          ├─ HttpParser (streaming request parsing)
          └─ Request handlers (static, CGI, upload)
```

### Connection States

- **READING**: awaiting client data
- **WRITING**: sending response
- **CLOSING**: cleanup and connection close
- **TLS_HANDSHAKE_READING** / **TLS_HANDSHAKE_WRITING**: SSL/TLS handshake negotiation

### Request Handlers

- **StaticFileHandler**: serves files, directory listings, symlink-safe path resolution
- **CgiHandler**: executes CGI scripts (.php, .py, custom binaries) with environment variable export
- **UploadHandler**: multipart/form-data parsing and atomic file writes
- **RedirectHandler**: 301/302 redirects

### Security Features (Recent Hardening)

- ✅ Symlink-safe path traversal protection (realpath validation)
- ✅ Signal handler reset in CGI child processes
- ✅ File descriptor inheritance cleanup in CGI children
- ✅ Atomic upload writes (no partial files on disk failure)
- ✅ Write buffer backpressure limits (DoS protection)
- ✅ Connection concurrency limits
- ✅ Streaming body size validation (memory efficiency)
- ✅ HTML-escaped directory listings (XSS protection)
- ✅ CGI child process timeout mechanism
- ✅ Proper HTTP/1.0 Connection: close headers

---

## Configuration

### NGINX-Like Syntax

Edit `config/default.conf`:

```nginx
server {
    listen localhost:8080;
    server_name example.com;
    
    # HTTPS/TLS (optional)
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;
    ssl_port 8443;
    
    max_body_size 5m;
    
    root ./assets/www;
    
    # Error pages
    error_page 404 /404.html;
    error_page 500 /50x.html;
    
    # Routing
    location / {
        methods GET POST HEAD;
        index index.html;
        directory_listing off;
    }
    
    location /upload {
        methods POST DELETE;
        upload_dir ./assets/uploads;
    }
    
    location /api {
        methods GET POST HEAD;
        cgi_extension .py;
    }
    
    # Redirects
    location /old {
        return 301 /;
    }
}
```

### Configuration Directives

| Directive | Scope | Example | Notes |
|-----------|-------|---------|-------|
| `listen` | server | `listen 0.0.0.0:8080` | Address and port |
| `server_name` | server | `server_name example.com` | Virtual host matching |
| `root` | server, location | `root ./assets/www` | Document root |
| `ssl_certificate` | server | `ssl_certificate ./cert.pem` | Path to SSL cert |
| `ssl_certificate_key` | server | `ssl_certificate_key ./key.pem` | Path to SSL key |
| `ssl_port` | server | `ssl_port 8443` | HTTPS listen port |
| `max_body_size` | server, location | `max_body_size 100m` | Request body limit |
| `methods` | location | `methods GET POST HEAD` | Allowed HTTP methods |
| `index` | location | `index index.html` | Default file in directory |
| `directory_listing` | location | `directory_listing on` | Enable dir browsing |
| `upload_dir` | location | `upload_dir ./uploads` | Upload destination |
| `cgi_extension` | location | `cgi_extension .php` | CGI script extension |
| `return` | location | `return 301 /path` | Redirect response |

---

## Project Structure

```
.
├── README.md                    # This file
├── LICENSE                      # License
├── Makefile                     # Build configuration
├── CLAUDE.md                    # Developer guidance
├── SERVER_CODE_REVIEW.md        # Security audit report
│
├── config/
│   └── default.conf             # NGINX-like server config
│
├── include/                     # Header files
│   ├── webserv.hpp              # Main server class
│   ├── router.hpp               # Virtual host + path routing
│   ├── logger.hpp               # Debug logging
│   ├── config/                  # Config parsing
│   ├── http/                    # HTTP protocol (parser, response, handlers)
│   ├── io/                      # Socket and connection management
│   ├── path/                    # Path utilities
│   ├── signal/                  # Signal handling
│   └── string/                  # String utilities
│
├── src/                         # Implementation files
│   ├── main.cpp                 # Entry point
│   ├── webserv.cpp              # Main server loop
│   ├── router.cpp               # Request routing
│   └── [mirror of include/]
│
├── assets/
│   ├── whiteboard/              # Frontend SPA (HTML, CSS, JS)
│   │   ├── index.html           # Main whiteboard app
│   │   ├── app.js               # Main app controller
│   │   ├── state.js             # Draw state management
│   │   ├── renderer.js          # Canvas rendering
│   │   ├── shapes/              # Shape definitions
│   │   └── tools/               # Tool implementations
│   ├── www/                     # Static content & landing page
│   │   ├── index.html           # Homepage
│   │   ├── 404.html             # Error pages
│   │   └── api/
│   │       ├── boards.py        # CGI board persistence API
│   │       └── boards/          # Saved board JSON files
│   └── uploads/                 # File upload destination
│
└── out/                         # Build output
    └── webserv                  # Compiled executable
```

---

## HTTP Protocol Support

### Version Compliance

- **HTTP/1.0**: full support with Connection: close semantics
- **HTTP/1.1**: support for keep-alive, Content-Length, Transfer-Encoding: chunked

### Request Parsing

- Streaming header parsing (no fixed size limits)
- Configurable body size limits per server/location
- Multipart/form-data support for file uploads
- Query string and fragment handling

### Response Features

- Automatic Content-Type detection (MIME types)
- Content-Length and chunked transfer encoding
- Custom error pages (configurable per status)
- Redirect support (301, 302)
- Conditional responses (304 Not Modified where applicable)

---

## CGI Support

The server executes CGI scripts with full HTTP header export:

```bash
REQUEST_METHOD=GET
SCRIPT_NAME=/api/boards.py
PATH_INFO=/boards
PATH_TRANSLATED=/home/user/assets/www/api/boards.py
QUERY_STRING=action=list
CONTENT_LENGTH=0
CONTENT_TYPE=application/json
HTTP_*=<request headers>
```

Supported scripts:
- `.php` → `/usr/bin/php-cgi`
- `.py` → `/usr/bin/python3`
- `.bla` → custom binary at `./assets/cgi_tester`
- Others → executed directly

**Timeout**: 30-second limit per CGI execution (configurable)

---

## Testing

### Build & Compile Check

```bash
make clean && make
```

### Run with Default Config

```bash
make run
```

Then test from another terminal:

```bash
curl -v http://localhost:8080/
curl -v http://localhost:8080/whiteboard/
curl -X POST -F "file=@test.txt" http://localhost:8080/upload
```

### Run Debug Build

```bash
make debug
./out/webserv config/default.conf
```

Outputs detailed logs to console showing connection events, parsing states, and handler execution.

---

## Board Format

Boards are JSON documents with structure:

```json
{
  "version": "1.0",
  "viewport": { "x": 0, "y": 0, "zoom": 1 },
  "shapes": [
    {
      "id": "shape-123",
      "type": "rectangle",
      "x": 100,
      "y": 100,
      "width": 200,
      "height": 150,
      "stroke": "#000000",
      "fill": "#FFFFFF",
      "strokeWidth": 2
    }
  ]
}
```

Supported shapes: rectangle, ellipse, line, arrow, pen, text, sticky note, connector.

---

## Performance Notes

- **Non-blocking I/O**: handles thousands of concurrent connections efficiently
- **Poll-based event loop**: O(n) scalability, no thread overhead
- **Streaming parsing**: constant memory for large request bodies
- **Connection timeouts**: 120-second idle timeout prevents resource exhaustion
- **Buffer limits**: write backpressure (10MB max) prevents slow-client DoS

---

## Development

### Add Custom Handler

1. Create new handler class in `include/http/my_handler.hpp`
2. Implement in `src/http/my_handler.cpp`
3. Register in `Router::selectHandler()` based on path/method
4. Update `Makefile` to include new source file

### Extend Configuration

1. Add field to `ServerConfig` or `LocationConfig` struct
2. Update config parser in `src/config/config_parser.cpp`
3. Use in handler logic as needed

### Customize Frontend

Edit files in `assets/whiteboard/`:
- `toolbar.js` for tool UI
- `renderer.js` for canvas rendering
- `shapes/` for shape definitions
- `index.html` for layout

---

## License

See [LICENSE](LICENSE) file.

---

## References

- CLAUDE.md: internal developer guidance and architecture notes
- SERVER_CODE_REVIEW.md: security audit and known issues
- RFC 7230-7235: HTTP/1.1 protocol specification
- RFC 5246: TLS 1.2 protocol specification