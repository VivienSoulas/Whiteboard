*This project has been created as part of the 42 curriculum by vsoulas, nkhamich and mde-krui

## Description
Webserv is an HTTP server written from scratch in C++. It demonstrates the fundamental concepts of handling clients, socket connections, configuration parsing, static routing, and executing CGI scripts. The goal of this project is to implement an equivalent of light NGINX to serve HTTP traffic using non-blocking I/O multiplexing (`poll()`).

## Instructions
**Compilation:**
The project uses a standard Makefile.
- `make` or `make all` - Build with optimizations.
- `make debug` - Build with debug symbols.
- `make clean` - Remove object files.
- `make fclean` - Remove all build artifacts including the executable.
- `make re` - Rebuild from scratch.

**Execution:**
Execute the server by providing a configuration file:
`./webserv path/to/config.conf`

If no configuration file is provided, an error will be displayed if a default cannot be found.

## Resources
- [RFC 9110 (HTTP Semantics)](https://www.rfc-editor.org/rfc/rfc9110.html)
- [C++ Reference](https://en.cppreference.com/)
- [CGI Env Variables](https://www6.uniovi.es/~antonio/ncsa_httpd/cgi/env.html)
- **AI Tools**: Used AI to verify subject requirements, to generate a config including assets for the website, and to add debug logs.