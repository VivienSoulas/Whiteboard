# What to Install

Assuming a fresh Debian/Ubuntu system.

---

## 1. C++ compiler + make

Required to build the server.

```bash
sudo apt update
sudo apt install build-essential
```

Provides: `g++` (C++11), `make`.

---

## 2. Python 3

Required for CGI scripts (`assets/example/cgi-bin/hello.py`).

```bash
sudo apt install python3
```

---

## 3. PHP CLI

Required for CGI scripts (`assets/www/cgi-bin/hello.php`).

```bash
sudo apt install php-cli
```

---

## 4. A modern web browser

Required for the whiteboard (`/whiteboard`). Uses ES6 modules — no build step needed.

Any of: Chrome 61+, Firefox 60+, Edge 16+.

---

## Not needed

| Thing | Why |
|---|---|
| Node.js | Not used at runtime (JS runs in browser only) |
| Go | `assets/tester` is a pre-compiled binary |
| nginx / apache | This project **is** the web server |
| npm / bundler | Frontend has no build step |

---

## Build and run

```bash
make        # compile → out/webserv
make run    # compile + start on ports 8080–8082
```

Then open: `http://localhost:8080/whiteboard/`
