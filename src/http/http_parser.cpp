#include "http/http_parser.hpp"
#include "http/http_status.hpp"
#include "http/http_method.hpp"
#include "string/string_parser.hpp"
#include <cstdlib>
#include <cerrno>
#include <limits>
#include <cctype>
#include "logger.hpp"

// ------------------------- Constructors / Destructor ------------------------

HttpParser::HttpParser() : state_(READING_HEADERS),
						   request_(),
						   buffer_(),
						   maxHeaderBytes_(8192),
						   headerEndPos_(0),
						   maxBodyBytes_(1024 * 1024),
						   contentLength_(0),
						   hasContentLength_(false),
						   shouldClose_(false),
						   errorStatus_(0),
						   chunkPhase_(READING_CHUNK_SIZE),
						   currentChunkSize_(0),
						   isChunked_(false) {}

HttpParser::~HttpParser() {}

HttpParser::Result::Result() : state(READING_HEADERS),
							   statusCode(0),
							   shouldClose(false) {}

HttpParser::Result::Result(State s, int code, bool close) : state(s),
															statusCode(code),
															shouldClose(close) {}

// -------------------------------- Public API --------------------------------

/** @brief Set max header size; exceeding it makes feed() return ERROR (400). */
void HttpParser::setMaxHeaderBytes(std::size_t n) { maxHeaderBytes_ = n; }

/** @brief Set max body size; exceeding it makes feed() return ERROR (413). */
void HttpParser::setMaxBodyBytes(std::size_t n) { maxBodyBytes_ = n; }

HttpParser::Result HttpParser::validateBodySize()
{
	if (request_.body.size() > maxBodyBytes_)
		return setError(HttpStatus::PAYLOAD_TOO_LARGE, true);
	return Result(state_, 0, shouldClose_);	
}

/** @brief Get the parsed request (valid only when the state is COMPLETE). */
const HttpRequest &HttpParser::getRequest() const { return request_; }

/**
 * @brief Feeds data into the parser and advances it until more input is needed
 * or a request is complete.
 *
 * Call this with newly received bytes. The parser will consume as much as it can
 * (headers, then body) and return when the request is complete, an error occurs,
 * or more data is required.
 */
HttpParser::Result HttpParser::feed(const char *data, std::size_t len)
{
	if (state_ == ERROR)
		return Result(ERROR, errorStatus_, shouldClose_);

	if (data != 0 && len > 0)
		buffer_.append(data, len);

	while (true)
	{
		// Check body size during streaming before appending more data
		if (state_ == READING_BODY)
		{
			if (request_.body.size() > maxBodyBytes_)
			{
				return setError(HttpStatus::PAYLOAD_TOO_LARGE, true);
			}
		}

		State prevState = state_;
		std::size_t prevSize = buffer_.size();

		Result r(state_, 0, shouldClose_);

		if (state_ == READING_HEADERS)
			r = tryParsingHeaders();
		else if (state_ == READING_BODY)
			r = tryParsingBody();
		else
			return Result(COMPLETE, 0, shouldClose_);

		if (state_ == COMPLETE || state_ == ERROR)
			return r;

		if (state_ == prevState && buffer_.size() == prevSize)
			return Result(state_, 0, shouldClose_);
	}
}

/**
 * @brief Reset the parser to read the next request on the same connection.
 *
 * Clears the current request and parsing state, but preserves any unread
 * bytes already buffered (for keep-alive / pipelined input).
 */
void HttpParser::reset()
{
	state_ = READING_HEADERS;
	request_ = HttpRequest();
	headerEndPos_ = 0;
	contentLength_ = 0;
	hasContentLength_ = false;

	isChunked_ = false;
	chunkPhase_ = READING_CHUNK_SIZE;
	currentChunkSize_ = 0;

	shouldClose_ = false;
	errorStatus_ = 0;
}

// -------------------------- Parsing Entrypoints -----------------------------

/**
 * @brief Attempts to parse a complete HTTP header block from buffer_.
 *
 * Looks for the CRLFCRLF terminator, enforces maxHeaderBytes_, parses the request
 * line and header fields, then erases the header bytes from buffer_ so the body
 * (if any) starts at offset 0. Sets state_ to READING_BODY if chunked or
 * Content-Length is present, otherwise marks the request COMPLETE.
 *
 * @return READING_HEADERS if more data is needed, READING_BODY/COMPLETE on success,
 * or ERROR on malformed/oversized headers.
 */
HttpParser::Result HttpParser::tryParsingHeaders()
{
	std::string::size_type pos = buffer_.find("\r\n\r\n");
	if (pos == std::string::npos)
	{
		if (buffer_.size() > maxHeaderBytes_)
			return setError(HttpStatus::REQUEST_HEADER_FIELDS_TOO_LARGE, true);
		return Result(READING_HEADERS, 0, shouldClose_);
	}

	headerEndPos_ = static_cast<std::size_t>(pos + 4);
	if (headerEndPos_ > maxHeaderBytes_)
		return setError(HttpStatus::REQUEST_HEADER_FIELDS_TOO_LARGE, true);

	const std::string header_block = buffer_.substr(0, pos);
	std::string::size_type end = header_block.find("\r\n");

	std::string request_line = header_block.substr(0, end);
	if (!parseRequestLine(request_line))
		return setError(HttpStatus::BAD_REQUEST, true);

	std::string::size_type start = (end == std::string::npos)
									   ? header_block.size()
									   : end + 2;

	while (start < header_block.size())
	{
		end = header_block.find("\r\n", start);
		std::string line = (end == std::string::npos)
							   ? header_block.substr(start)
							   : header_block.substr(start, end - start);
		if (line.empty())
			return setError(HttpStatus::BAD_REQUEST, true);
		if (!parseHeaderLine(line))
			return setError(HttpStatus::BAD_REQUEST, true);
		if (end == std::string::npos)
			break;
		start = end + 2;
	}

	if (request_.version == HttpRequest::HTTP_1_1 && !hasNonEmptyHeader("host"))
		return setError(HttpStatus::BAD_REQUEST, true);

	shouldClose_ = computeShouldClose(request_);
	buffer_.erase(0, headerEndPos_);
	headerEndPos_ = 0;

	if ((request_.method == POST || request_.method == PUT || request_.method == PATCH) && (!hasContentLength_ && !isChunked_))
		return setError(HttpStatus::LENGTH_REQUIRED, true);
	if (hasContentLength_ || isChunked_)
	{
		state_ = READING_BODY;
		DEBUG_LOG("Switching to READING_BODY (" << (isChunked_ ? "chunked" : "Content-Length: " + std::to_string(contentLength_)) << ")");
		return Result(READING_BODY, 0, shouldClose_);
	}
	state_ = COMPLETE;
	return Result(COMPLETE, 0, shouldClose_);
}

/**
 * @brief Attempts to parse request body according to the framing determined from headers.
 *
 * If Transfer-Encoding is chunked, delegates to tryParsingChunkedBody(). Otherwise, if a
 * Content-Length is present, waits until that many bytes are buffered, copies them into
 * request_.body, and erases them from buffer_. If neither framing is present, the body
 * is treated as empty. On success sets state_ to COMPLETE; on insufficient data returns
 * READING_BODY; on oversize returns ERROR.
 */
HttpParser::Result HttpParser::tryParsingBody()
{
	if (isChunked_)
		return tryParsingChunkedBody();

	if (hasContentLength_)
	{
		if (contentLength_ > maxBodyBytes_)
			return setError(HttpStatus::PAYLOAD_TOO_LARGE, true);

		if (contentLength_ == 0)
		{
			state_ = COMPLETE;
			return Result(COMPLETE, 0, shouldClose_);
		}

		if (buffer_.size() < contentLength_)
			return Result(READING_BODY, 0, shouldClose_);

		request_.body.assign(buffer_, 0, contentLength_);
		buffer_.erase(0, contentLength_);
	}

	state_ = COMPLETE;
	DEBUG_LOG("Body parsing complete (" << request_.body.size() << " bytes)");
	return Result(COMPLETE, 0, shouldClose_);
}

/**
 * @brief Attempts to parse a chunked-encoded body from buffer_.
 *
 * Drives the chunk state machine (size line -> data + CRLF -> trailers). Appends decoded
 * chunk data to request_.body, consumes parsed bytes from buffer_, enforces maxBodyBytes_,
 * returns READING_BODY when more bytes are needed, and sets state_ to COMPLETE after the
 * final 0-sized chunk and trailer terminator.
 */
HttpParser::Result HttpParser::tryParsingChunkedBody()
{
	while (true)
	{
		if (chunkPhase_ == READING_CHUNK_SIZE)
		{
			std::string::size_type lineEnd = buffer_.find("\r\n");
			if (lineEnd == std::string::npos)
				return Result(READING_BODY, 0, shouldClose_);

			std::string sizeLine = buffer_.substr(0, lineEnd);

			std::size_t chunkSize = 0;
			if (!parseChunkSizeLine(sizeLine, chunkSize))
				return setError(HttpStatus::BAD_REQUEST, true);

			currentChunkSize_ = chunkSize;
			// Per-chunk bounds checking: reject individual chunks over 100MB
			if (currentChunkSize_ > 100*1024*1024)
				return setError(HttpStatus::BAD_REQUEST, true);
			buffer_.erase(0, lineEnd + 2);

			if (currentChunkSize_ == 0)
				chunkPhase_ = READING_CHUNK_TRAILERS;
			else
			{
				if (request_.body.size() + currentChunkSize_ > maxBodyBytes_)
					return setError(HttpStatus::PAYLOAD_TOO_LARGE, true);
				chunkPhase_ = READING_CHUNK_DATA;
			}
			continue;
		}

		if (chunkPhase_ == READING_CHUNK_DATA)
		{
			if (buffer_.size() < currentChunkSize_ + 2)
				return Result(READING_BODY, 0, shouldClose_);

			request_.body.append(buffer_, 0, currentChunkSize_);

			if (buffer_.compare(currentChunkSize_, 2, "\r\n") != 0)
				return setError(HttpStatus::BAD_REQUEST, true);

			buffer_.erase(0, currentChunkSize_ + 2);
			currentChunkSize_ = 0;
			chunkPhase_ = READING_CHUNK_SIZE;
			continue;
		}

		if (chunkPhase_ == READING_CHUNK_TRAILERS)
		{
			if (buffer_.size() >= 2 && buffer_.compare(0, 2, "\r\n") == 0)
			{
				buffer_.erase(0, 2);
				state_ = COMPLETE;
				return Result(COMPLETE, 0, shouldClose_);
			}

			std::string::size_type trailersEnd = buffer_.find("\r\n\r\n");
			if (trailersEnd == std::string::npos)
				return Result(READING_BODY, 0, shouldClose_);

			buffer_.erase(0, trailersEnd + 4);
			state_ = COMPLETE;
			return Result(COMPLETE, 0, shouldClose_);
		}

		return setError(HttpStatus::INTERNAL_SERVER_ERROR, true);
	}
}

/** @brief Switches the parser to ERROR state and returns an error Result. */
HttpParser::Result HttpParser::setError(int statusCode, bool shouldClose)
{
	DEBUG_LOG("HttpParser error: status " << statusCode << " (shouldClose: " << (shouldClose ? "yes" : "no") << ")");
	state_ = ERROR;
	errorStatus_ = statusCode;
	shouldClose_ = shouldClose;
	return Result(ERROR, statusCode, shouldClose);
}

// ------------------------------- Line Parsing -------------------------------

/**
 * @brief Parses the HTTP request-line ("METHOD SP target SP HTTP-version").
 *
 * Validates the basic syntax (exactly two spaces, non-empty tokens, target starts with '/')
 * and fills request_ fields. Unknown methods/versions are recorded as UNKNOWN_* and are
 * expected to be handled later (e.g. 405/505), while malformed syntax returns false.
 */
bool HttpParser::parseRequestLine(const std::string &line)
{
	std::vector<std::string> tokens = string_parser::split(line, ' ');
	if (tokens.size() != 3 || tokens[0].empty() || tokens[1].empty() || tokens[2].empty())
		return false;

	request_.method = parseHttpMethod(tokens[0]);

	if (tokens[2] == "HTTP/1.1")
		request_.version = HttpRequest::HTTP_1_1;
	else if (tokens[2] == "HTTP/1.0")
		request_.version = HttpRequest::HTTP_1_0;
	else
		request_.version = HttpRequest::UNKNOWN_VERSION;

	if (tokens[1][0] != '/')
		return false;

	request_.target = tokens[1];
	splitTarget(request_.target, request_.path, request_.query);
	DEBUG_LOG("Parsed request line: " << tokens[0] << " " << tokens[1] << " " << tokens[2]);
	return true;
}

/**
 * @brief Parses a single "Name: value" header line and updates parser/request state.
 *
 * Splits at the first ':' and trims whitespace around the name and value. Normalises
 * the header name to lowercase before storing it in request_.headers.
 *
 * Duplicate policy: "cookie" appends additional values using "; "; "content-length" validates
 * and records Content-Length, rejects conflicting values and rejects any Content-Length if
 * Transfer-Encoding: chunked was set; "transfer-encoding" accepts only tokens "identity" and
 * "chunked", requires that "chunked" is present and is the last token, rejects TE+Content-Length,
 * rejects duplicate TE; other headers - last value wins (overwrites the previous stored value).
 *
 * @return true on success, false on malformed/unsupported headers or invalid combinations.
 */
bool HttpParser::parseHeaderLine(const std::string &line)
{
	std::pair<std::string, std::string> kv = string_parser::splitOnce(line, ':');
	if (line.find(':') == std::string::npos)
		return false;

	std::string name = string_parser::toLower(string_parser::trim(kv.first));
	std::string value = string_parser::trim(kv.second);

	if (name.empty())
		return false;

	HttpRequest::HeaderMap::iterator it = request_.headers.find(name);
	if (it != request_.headers.end())
	{
		if (name == "cookie")
		{
			if (!it->second.empty() && !value.empty())
				it->second += "; ";
			it->second += value;
			return true;
		}
		if (name == "transfer-encoding")
			return false;
	}

	request_.headers[name] = value;

	if (name == "content-length")
	{
		if (isChunked_)
			return false;

		std::size_t n = 0;
		if (!parseContentLength(value, n))
			return false;

		if (hasContentLength_ && n != contentLength_)
			return false;

		hasContentLength_ = true;
		contentLength_ = n;
	}
	else if (name == "transfer-encoding")
	{
		if (value.empty())
			return false;

		std::string lowerValue = string_parser::toLower(value);

		bool foundChunked = false;
		bool chunkedWasLast = false;

		std::vector<std::string> tokens = string_parser::split(lowerValue, ',');
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			std::string token = string_parser::trim(tokens[i]);

			if (token.empty() || (token != "chunked" && token != "identity"))
				return false;

			if (token == "chunked")
				foundChunked = true;

			if (i == tokens.size() - 1)
			{
				if (token == "chunked")
					chunkedWasLast = true;
			}
		}

		if (!foundChunked || !chunkedWasLast || hasContentLength_) {
			DEBUG_LOG("Invalid header combination: Transfer-Encoding chunked must be last and cannot coexist with Content-Length");
			return false;
		}
		isChunked_ = true;
		DEBUG_LOG("Request is chunked-encoded");
	}
	return true;
}

/**
 * @brief Parses and validates a Content-Length header value.
 *
 * Accepts only a non-negative decimal integer that fits into size_t, with no
 * extra characters or signs. On success, stores the parsed length in outLen.
 *
 * @return true if the value is a valid Content-Length, false otherwise.
 */
bool HttpParser::parseContentLength(const std::string &value, std::size_t &outLen)
{
	std::string s = string_parser::trim(value);
	if (s.empty())
		return false;

	if (s[0] == '-' || s[0] == '+')
		return false;

	errno = 0;
	char *end = 0;
	unsigned long n = std::strtoul(s.c_str(), &end, 10);

	if (end == s.c_str() || *end != '\0' || errno == ERANGE)
		return false;
	if (n > static_cast<unsigned long>(std::numeric_limits<std::size_t>::max()))
		return false;

	outLen = static_cast<std::size_t>(n);
	return true;
}

/**
 * @brief Parses a chunk size line (hex), ignoring any ";extensions".
 *
 * Extracts the hex size token before ';', trims whitespace, rejects signs/junk/overflow,
 * and writes the parsed size to outSize.
 *
 * @return true if the line contains a valid chunk size, false otherwise.
 */
bool HttpParser::parseChunkSizeLine(const std::string &line, std::size_t &outSize)
{
	std::string sizePart = string_parser::trim(string_parser::splitOnce(line, ';').first);

	if (sizePart.empty() || sizePart[0] == '-' || sizePart[0] == '+')
		return false;

	errno = 0;
	char *end = 0;
	unsigned long long n = std::strtoull(sizePart.c_str(), &end, 16);

	if (end == sizePart.c_str() || *end != '\0' || errno == ERANGE)
		return false;
	if (n > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max()))
		return false;

	outSize = static_cast<std::size_t>(n);
	DEBUG_LOG("Parsed chunk size: " << outSize);
	return true;
}

// ------------------------------- Utilities ----------------------------------

/** @brief Splits a request target into path and query (everything after '?'). */
void HttpParser::splitTarget(const std::string &target, std::string &path, std::string &query)
{
	std::pair<std::string, std::string> p = string_parser::splitOnce(target, '?');
	path = p.first;
	query = p.second;
}

/**
 * @brief Checks whether a header exists and has a non-empty value.
 *
 * Looks up @p name in the parsed request headers and returns true if the
 * header is present and its trimmed value is not empty.
 */
bool HttpParser::hasNonEmptyHeader(const std::string &name) const
{
	HttpRequest::HeaderMap::const_iterator it = request_.headers.find(string_parser::toLower(name));
	return (it != request_.headers.end() && !string_parser::trim(it->second).empty());
}

/**
 * @brief Checks whether a header contains a specific comma-separated token.
 *
 * Looks up @p header in the parsed request headers and scans its value for
 * a case-insensitive match of @p token, splitting on commas and trimming
 * surrounding whitespace.
 */
bool HttpParser::hasHeaderToken(const std::string &header, const std::string &token) const
{
	HttpRequest::HeaderMap::const_iterator it = request_.headers.find(header);
	if (it == request_.headers.end())
		return false;

	const std::string valueLower = string_parser::toLower(it->second);
	const std::string tokenLower = string_parser::toLower(token);

	std::vector<std::string> parts = string_parser::split(valueLower, ',');
	for (size_t i = 0; i < parts.size(); ++i)
	{
		std::string part = string_parser::trim(parts[i]);

		if (part == tokenLower)
			return true;
	}
	return false;
}

/**
 * @brief Determines whether the connection should be closed after responding.
 *
 * Implements HTTP/1.x connection persistence rules using the "Connection" header:
 * - HTTP/1.1: persistent by default; close only if "close" token is present.
 * - HTTP/1.0: non-persistent by default; keep alive only if "keep-alive" is present.
 */
bool HttpParser::computeShouldClose(const HttpRequest &req) const
{
	bool hasClose = hasHeaderToken("connection", "close");
	bool hasKeepAlive = hasHeaderToken("connection", "keep-alive");

	if (req.version == HttpRequest::HTTP_1_1)
		return hasClose;
	if (req.version == HttpRequest::HTTP_1_0)
		return !hasKeepAlive;
	return true;
}
