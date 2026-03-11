# Whiteboard App

This project is now centered on an in-browser whiteboard application served by a custom C++ web server. The server remains the runtime layer, but the main product is the whiteboard itself: a lightweight SVG drawing space for sketching ideas, building rough layouts, annotating concepts, and iterating visually in the browser.

## Features

- Drawing tools: select, rectangle, ellipse, line, arrow, pen, text, sticky notes, and connectors.
- Direct manipulation on canvas: create, select, move, resize, multi-select, align, duplicate, copy, paste, and delete shapes.
- Style controls for stroke color, fill color, fill on or off, stroke width, sticky note color categories, and line or arrow ending styles.
- Editable text and sticky note content directly inside the canvas.
- Undo and redo history for board edits.
- Zoom and pan controls for navigating larger boards.
- Local persistence: save and load boards as JSON files.
- Export options: SVG and PNG.
- Server-side board persistence: save, load, list, and delete named boards through a lightweight Python CGI API.

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

## Controls

- Tool shortcuts: `S` select, `R` rectangle, `E` ellipse, `L` line, `A` arrow, `P` pen, `T` text, `N` sticky note, `K` connector.
- History: `Ctrl/Cmd+Z` undo, `Ctrl/Cmd+Shift+Z` or `Ctrl/Cmd+Y` redo.
- Clipboard: `Ctrl/Cmd+C` copy, `Ctrl/Cmd+V` paste, `Ctrl/Cmd+D` duplicate, `Ctrl/Cmd+A` select all, `Ctrl/Cmd+ESC` clear selection.
- Navigation: mouse wheel to zoom, middle mouse drag or `Space` + drag to pan.
- Selection: drag on empty canvas to marquee-select multiple shapes.

## Make Targets

- `make` builds the project with optimizations.
- `make debug` builds with debug symbols.
- `make clean` removes object files.
- `make fclean` removes build artifacts.
- `make re` rebuilds from scratch.

## Structure

- `assets/whiteboard/` contains the whiteboard frontend.
- `assets/www/` contains the landing page served at `/`.
- `assets/www/api/boards.py` exposes the server-side board persistence API.
- `assets/www/api/boards/` stores saved board JSON files.
- `config/default.conf` defines the active server configuration.
- `src/` and `include/` contain the C++ web server implementation.

## Board Format

Boards are stored as JSON documents containing the board version, shapes, and viewport state. The same document structure is used for local import or export and for server-side saved boards.