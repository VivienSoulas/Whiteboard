
# HTTP module – integration notes (TEMP, delete later)

The components I implemented:

- `HttpRequest` – parsed request data container
- `HttpParser` – incremental HTTP/1.x request parser
- `HttpResponse` – response container + serialisation
- `HttpResponseFactory` – helpers to build common response shapes
- `HttpStatus` - a shared source of truth for status code mapping

---

## 1. Components and intended lifecycle

* **`HttpParser`**
  *Stateful, per connection.*

  One `HttpParser` instance should exist **per client connection**, not per request. It accumulates bytes across reads, supports partial input, and may parse **multiple requests sequentially** on the same connection (keep-alive / pipelining).

  Typical lifecycle:

  * created when the connection is accepted
  * fed data via `feed()` on each readable event
  * after `COMPLETE`, `reset()` is called to parse the next request
  * destroyed when the connection is closed

* **`HttpRequest`**
  *Ephemeral, per request.*

  `HttpRequest` is populated by `HttpParser` and is **only valid after `feed()` returns `COMPLETE`**. It should be treated as **read-only** input for routing/handling logic.
  A new `HttpRequest` is constructed internally by the parser each time `reset()` is called.

* **`HttpResponse`**
  *Ephemeral, per response.*

  `HttpResponse` represents a single HTTP response. It is typically created via `HttpResponseFactory`, serialised once, sent to the socket, and then discarded.

  It does not own any connection state beyond the `shouldClose` flag.

* **`HttpResponseFactory`**
  *Stateless helper.*

  The factory contains only static helpers to build common **response shapes** (OK, error, redirect, 405, etc.). It holds no state and can be used freely wherever a response needs to be constructed.

---

To the best of my understanding, here's how other parts of the server are expected to interact with these components:

### Mental model (one connection, multiple requests)

```
[ socket accepted ]
       |
   create HttpParser
       |
   read -> feed()
       |
   COMPLETE?
       |
   get HttpRequest
       |
   route / handle
       |
   build HttpResponse -> serialize -> write
       |
   close?
     |    \
    yes   no
     |     |
   destroy parser
           |
        parser.reset()
           |
        read next request
```

---

## 2. HttpParser  – intended usage

### Feeding data
- Call `HttpParser::feed(const char* data, size_t len)` whenever bytes are read from the socket.
- `feed()` may be called with partial data (even 1 byte at a time).

### Parser results
`feed()` returns `HttpParser::Result`:

- `state == READING_HEADERS / READING_BODY`
  - More data is needed. Keep reading.
- `state == COMPLETE`
  - A full request is available via `getRequest()`.
- `state == ERROR`
  - The request is invalid. `statusCode` contains the HTTP error to return. `shouldClose` flag is set to true.

### Closing the connection
- `Result.shouldClose` indicates **minimum protocol requirement**.
- The connection owner may still decide to close for server reasons.
- So the final close decision is:

```

finalClose = parserResult.shouldClose || serverPolicyClose

```

### After sending a response
- If `finalClose == true` → close socket.
- Otherwise:
  - call `parser.reset()` to keep reading on the same connection
  - continue reading (keep-alive / pipelining supported).

---

## 2. What HttpParser guarantees

- Request line parsing (method / target / version).
- Target is split into `path` and `query`.
- Header names are normalised to lowercase.
- Supports:
  - `Content-Length` bodies
  - `Transfer-Encoding: chunked` bodies (including trailers)
- Rejects ambiguous framing (`TE + Content-Length`) with 400.
- Enforces:
  - max header size (431)
  - max body size (413)
- HTTP/1.1 requires `Host` header.
- Supports pipelined requests.

---

## 3. HttpResponse – important notes

- `Content-Length` is always calculated in `serialize()`. Don't set it manually.
- `Connection: close` is emitted **only** if `shouldClose == true`.
- Header names are stored case-insensitively (lowercased internally).
- Header values containing CR/LF are rejected to prevent response splitting.
- For status `204 No Content`, the body is cleared and `Content-Length: 0` is enforced during serialisation.

---

## 4. HttpResponseFactory – intended usage

The factory provides helpers for **response shapes**, not per-status logic.

### Generic error
Use for most 4xx/5xx errors:

```

HttpResponseFactory::buildError(req, statusCode, close);

```


### Successful response
```

HttpResponseFactory::buildOk(req, body, contentType, close);

```

### Method not allowed (405)
Use when method is not in `{GET, POST, DELETE}`:

```

buildMethodNotAllowed(req, {"GET","POST","DELETE"}, close);

```

Adds a correct `Allow` header automatically.

### Redirects (301 / 302)
```

buildRedirect(req, statusCode, location, close);

```

Adds `Location` header and minimal body.

### Unsupported HTTP version
```

buildVersionNotSupported(close);

```

Used when request version is invalid or unsupported.

---

## 5. Smoke tests (temporary)

This folder contains standalone test programs used to validate:
- `HttpParser` behaviour
- `HttpResponse` / `HttpResponseFactory` serialisation

They compile and run without the rest of the server.
**Note to self: delete these before final submission!**

---

There are also more detailed comments in the code itself, especially public API functions.
