# Whiteboard App

This project is now centered on an in-browser whiteboard application served by a custom C++ web server. The server remains the runtime layer, but the main product is the whiteboard itself: a lightweight SVG drawing space for sketching ideas, building rough layouts, annotating concepts, and iterating visually in the browser.

## Features

- Multiple drawing tools: select, rectangle, ellipse, line, arrow, pen, and text.
- Direct manipulation on canvas: create, select, move, resize, and delete shapes.
- Style controls for stroke color, fill color, fill on or off, and stroke width.
- Editable text blocks directly inside the canvas.
- Clear-all action for resetting the current board.
- Keyboard shortcuts for quickly switching tools.

## Launch

Build the server:

```bash
make
```

Run the server with the current configuration:

```bash
./out/webserv config/default.conf
or
make run
```

Then open:

```text
http://localhost:8080/
```

From the homepage, open the whiteboard at:

```text
http://localhost:8080/whiteboard/
```

## Make Targets

- `make` builds the project with optimizations.
- `make debug` builds with debug symbols.
- `make clean` removes object files.
- `make fclean` removes build artifacts.
- `make re` rebuilds from scratch.

## Structure

- `assets/whiteboard/` contains the whiteboard frontend.
- `assets/www/` contains the landing page served at `/`.
- `config/default.conf` defines the active server configuration.
- `src/` and `include/` contain the C++ web server implementation.