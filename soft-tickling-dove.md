# Code Review: C++11 Non-Blocking HTTP Server

## Context
Full codebase review of the webserv project focusing on correctness, poll() event-loop safety, HTTP protocol compliance, memory/resource safety, security, and performance in hot paths.

---

## Issues

### Critical

#### 1. Double-close of connection fds in ConnectionManager destructor
- **severity:** critical
- **file:** `src/io/connection_manager.cpp:24-32`
- **problem:** Destructor calls `close(pair.first)` on each fd, then `delete pair.second` which calls `Connection::~Connection()` that also calls `close(fd)`. Every connection fd is closed twice.
- **explanation:** Double-close can close an fd that was already reassigned to a new socket by the kernel, silently corrupting another connection.
- **suggested fix:** Remove `close(pair.first)` from the destructor — let `Connection::~Connection()` handle it. Same applies to the cleanup loop at line 61-75 — remove `close(it->first)` at line 68 since `delete it->second` already closes the fd.

#### 2. EAGAIN/EWOULDBLOCK not handled in recv()
- **severity:** critical
- **file:** `src/io/connection.cpp:82-94`
- **problem:** `readBuffer()` treats all negative `recv()` returns as fatal, sets state to CLOSING. On non-blocking sockets, `recv()` returning -1 with `errno == EAGAIN || errno == EWOULDBLOCK` is normal (no data available yet).
- **explanation:** Under load, poll() may report POLLIN but recv() can still return EAGAIN (spurious wakeup). Current code drops these connections.
- **suggested fix:**
```cpp
if (bytes_received < 0)
{
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return "";
    DEBUG_LOG("Connection [fd: " << fd << "] error: " << strerror(errno));
    setState(CLOSING);
    return "";
}
```
Also: don't set CLOSING when returning "" for EAGAIN — the caller in `connection_manager.cpp:174` already does `if (buf.empty()) continue;` which is correct, but the readBuffer currently sets CLOSING before returning "".

#### 3. EAGAIN/EWOULDBLOCK not handled in send()
- **severity:** critical
- **file:** `src/io/connection.cpp:118-128`
- **problem:** `writeData()` treats all negative `send()` returns as fatal. EAGAIN on send means the kernel buffer is full — retry on next POLLOUT.
- **explanation:** Large responses will frequently hit EAGAIN between partial writes. Current code drops the connection instead of retrying.
- **suggested fix:**
```cpp
if (sent < 0)
{
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return; // wait for next POLLOUT
    DEBUG_LOG("Connection [fd: " << fd << "] send failed: " << strerror(errno));
    setState(CLOSING);
}
```

#### 4. Connection copy constructor/assignment causes double-close
- **severity:** critical
- **file:** `src/io/connection.cpp:12-41`
- **problem:** Copy constructor and assignment operator copy the raw fd. If either the original or the copy is destroyed, the fd is closed, leaving the other with a dangling fd. Assignment also leaks the target's original fd.
- **explanation:** While connections are heap-allocated (`new Connection(...)`) and stored as pointers, the copy/assignment operators still exist and could be called accidentally. The assignment operator also doesn't close the old fd before overwriting.
- **suggested fix:** Delete copy constructor and assignment operator, or implement proper ownership transfer (set source fd to -1 after copy). Given the code uses `Connection*` pointers everywhere, deleting is simplest.

#### 5. DELETE handler: no path traversal check
- **severity:** critical
- **file:** `src/webserv.cpp:146-178`
- **problem:** DELETE resolves the file path via `resolveFilePath()` + `normalizeAndMakeAbsolute()` but never calls `pathUnderRoot()`. An attacker can delete files outside the server root.
- **explanation:** `resolveFilePath()` has incomplete `..` handling (see issue #10), so traversal payloads may survive normalization.
- **suggested fix:** Add the same `pathUnderRoot()` check that static_file_handler uses:
```cpp
std::string root_abs = path_utils::normalizeAndMakeAbsolute("",
    location->root.empty() ? server->root : location->root);
if (!path_utils::pathUnderRoot(file_path, root_abs)) {
    return HttpResponseFactory::buildError(req, HttpStatus::FORBIDDEN, true).serialize();
}
```

#### 6. Upload handler: no path containment check
- **severity:** critical
- **file:** `src/http/upload_handler.cpp:92`
- **problem:** After sanitizing the filename and building the path with `normalizeAndMakeAbsolute(upload_dir, filename)`, there's no check that the result is actually under `upload_dir`.
- **explanation:** If sanitization has any gap (e.g. edge cases in `sanitizeUploadFilename`), files could be written anywhere.
- **suggested fix:** Add `pathUnderRoot(path, upload_dir_abs)` check before opening the file.

---

### Major

#### 7. resolveFilePath has incomplete `..` handling
- **severity:** major
- **file:** `src/path/path_utils.cpp:27-36`
- **problem:** The `..` check only matches the pattern `../` (with trailing slash). A path ending in `..` without trailing slash is not caught. Also, `..` preceded by non-`/` chars (e.g. `foo..bar`) isn't an issue, but `%2e%2e` (URL-encoded) would bypass entirely if URL decoding happens after this function.
- **explanation:** The path `GET /uploads/../../etc/passwd` — the final `..` at end of path would not be resolved by this function. Note: `normalizeAndMakeAbsolute()` also does not resolve `..`, so the traversal can reach the filesystem.
- **suggested fix:** Use `realpath()` for the final resolved path and verify it's under the root. This eliminates symlink attacks too.

#### 8. CGI handler blocks the entire event loop
- **severity:** major
- **file:** `src/http/cgi_handler.cpp:153-220`
- **problem:** The CGI handler runs in the request handler, which is called synchronously from the poll loop. The internal poll loop + `waitpid(pid, &status, 0)` blocks all other connections while a CGI script executes.
- **explanation:** A slow CGI script (e.g., PHP processing a large request) stalls the entire server. This defeats the purpose of the non-blocking architecture.
- **suggested fix:** For a 42 project this may be acceptable. A proper fix would integrate CGI pipe fds into the main ConnectionManager poll loop and track CGI state asynchronously. At minimum, use `WNOHANG` with waitpid and set a hard timeout.

#### 9. HEAD request returns 200 for non-existent files
- **severity:** major
- **file:** `src/webserv.cpp:189-196`
- **problem:** When `static_file_handler::serve()` returns empty (file not found), the HEAD method falls through to a hardcoded 200 OK response with empty body, instead of returning 404.
- **explanation:** HEAD should return the same status code that GET would return. Returning 200 for a missing resource violates HTTP semantics.
- **suggested fix:**
```cpp
if (head_only)
{
    HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::NOT_FOUND, true);
    return res.serialize();
}
```

#### 10. pathUnderRoot only checks server.root, not location.root
- **severity:** major
- **file:** `src/http/static_file_handler.cpp:113`
- **problem:** The traversal check uses `server.root` as the boundary, but the file was resolved from `location.root` (which may be different). If location.root is a subdirectory, the check is too permissive. If location.root is outside server.root, the check blocks legitimate access.
- **suggested fix:** Check against the effective root (`location.root.empty() ? server.root : location.root`).

#### 11. Socket assignment operator: fd leak + double-close
- **severity:** major
- **file:** `src/io/socket.cpp:35-40`
- **problem:** Assignment copies fd without closing the target's existing fd (leak) and creates two objects owning the same fd (double-close on destruction).
- **suggested fix:** Delete the assignment operator (copy constructor is already deleted).

#### 12. Double-close of listener socket fds in ConnectionManager destructor
- **severity:** major
- **file:** `src/io/connection_manager.cpp:34-39`
- **problem:** `close(it->getFd())` is called explicitly, then `delete(it)` calls `Socket::~Socket()` which also calls `close(fd)`.
- **suggested fix:** Remove the explicit `close(it->getFd())` — let the Socket destructor handle it.

---

### Minor

#### 13. POST without upload_dir or CGI returns 200 with empty body
- **severity:** minor
- **file:** `src/webserv.cpp:199-203`
- **problem:** Any POST request that doesn't match CGI or upload falls through to return 200 OK with empty body, silently discarding the request body.
- **explanation:** Could mask client-side bugs. Consider returning 405 or 400.

#### 14. No boundary length validation for multipart uploads
- **severity:** minor
- **file:** `src/http/upload_handler.cpp:19`
- **problem:** No upper bound on boundary length. An extremely long boundary (megabytes) would make `body.find(delim, pos)` searches slow.
- **suggested fix:** Reject boundaries exceeding RFC 2046's 70-character limit.

#### 15. Connection `close_after_write` not copied in assignment operator
- **severity:** minor
- **file:** `src/io/connection.cpp:25-41`
- **problem:** The assignment operator copies all fields except `close_after_write`.
- **explanation:** Low impact since copy/assignment should be deleted (issue #4), but if they aren't, this causes incorrect behavior.

---

## Patch

The highest-priority minimal patches:

### Patch 1: Fix double-close in ConnectionManager destructor
**File:** `src/io/connection_manager.cpp`

```diff
 ConnectionManager::~ConnectionManager()
 {
 	for (auto &pair : _connections)
 	{
 		if (pair.second)
 			delete pair.second;
-		if (pair.first > 0)
-			close(pair.first);
 	}
 	_connections.clear();
 	for (auto &it : _listener_sockets)
 	{
-		if (it->getFd() > 0)
-			close(it->getFd());
 		delete (it);
 	}
 	DEBUG_LOG("All connections cleaned up");
 }
```

Also in `pollConnections()` cleanup loop (~line 67-68):
```diff
 		_parsers.erase(it->first);
 		delete it->second;
-		close(it->first);
 		it = _connections.erase(it);
```

And in POLLERR/POLLHUP handler (~line 155-158):
```diff
 		_parsers.erase(fd);
 		delete cit->second;
 		_connections.erase(cit);
-		close(fd);
```

### Patch 2: Handle EAGAIN in recv/send
**File:** `src/io/connection.cpp`

In `readBuffer()`:
```diff
 	if (bytes_received < 0)
+	{
+		if (errno == EAGAIN || errno == EWOULDBLOCK)
+			return "";
 		DEBUG_LOG("Connection [fd: " << fd << "] error: " << strerror(errno));
-	else
+		setState(CLOSING);
+		return "";
+	}
+	if (bytes_received == 0)
+	{
 		DEBUG_LOG("Connection [fd: " << fd << "] closed by peer");
-	setState(CLOSING);
-	return "";
+		setState(CLOSING);
+		return "";
+	}
```

In `writeData()`:
```diff
 	else {
+		if (errno == EAGAIN || errno == EWOULDBLOCK)
+			return;
 		DEBUG_LOG("Connection [fd: " << fd << "] send failed (errno: " << strerror(errno) << ")");
 		setState(CLOSING);
 	}
```

### Patch 3: Path traversal protection for DELETE
**File:** `src/webserv.cpp`, in the DELETE handler:

```diff
 		file_path = path_utils::normalizeAndMakeAbsolute("", file_path);
+		std::string root_abs = path_utils::normalizeAndMakeAbsolute("",
+			location->root.empty() ? server->root : location->root);
+		if (!path_utils::pathUnderRoot(file_path, root_abs)) {
+			return HttpResponseFactory::buildError(req, HttpStatus::FORBIDDEN, true).serialize();
+		}
 		if (access(file_path.c_str(), F_OK) == 0)
```

### Patch 4: Fix HEAD returning 200 for missing files
**File:** `src/webserv.cpp`:

```diff
-		if (head_only)
-		{
-			HttpResponse res;
-			res.setStatus(HttpStatus::OK);
-			res.setBody("");
-			res.setShouldClose(false);
-			return res.serialize();
-		}
+		// static_file_handler returned empty → file not found
+		HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::NOT_FOUND, true);
+		return res.serialize();
```

---

## Verdict

**CHANGES_REQUESTED**

6 critical issues, 6 major issues, 3 minor issues found.

The **double-close** (issues #1, #12) and **missing EAGAIN handling** (issues #2, #3) are the most urgent — they cause connection corruption under load and can crash the server. The **DELETE path traversal** (issue #5) is the most urgent security issue — it allows deleting arbitrary files on the filesystem.
