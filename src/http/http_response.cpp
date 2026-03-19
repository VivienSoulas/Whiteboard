#include "http/http_response.hpp"
#include "http/http_status.hpp"
#include "string/string_parser.hpp"
#include <sstream>
#include <cctype>

// ------------------------- Constructor / Destructor -------------------------

HttpResponse::HttpResponse() : version_(HTTP_1_1),
							   headers_(),
							   body_(),
							   statusCode_(200),
							   shouldClose_(false),
							   reasonPhrase_(defaultReasonPhrase(200)) {}

HttpResponse::~HttpResponse() {}

// -------------------------------- Public API --------------------------------

/** @brief Sets the HTTP version used in the status line (HTTP/1.0 or HTTP/1.1). */
void HttpResponse::setVersion(Version v) { version_ = v; }

/** @brief Returns the HTTP version used in the status line. */
HttpResponse::Version HttpResponse::getVersion() const { return version_; }

/** @brief Sets the status code and assigns the default reason phrase for that code. */
void HttpResponse::setStatus(int code)
{
	statusCode_ = code;
	reasonPhrase_ = defaultReasonPhrase(code);
}

/** @brief Returns the response status code (e.g. 200, 404). */
int HttpResponse::getStatusCode() const { return statusCode_; }

/** @brief Returns the reason phrase used in the status line (e.g. "OK", "Not Found"). */
const std::string &HttpResponse::getReasonPhrase() const { return reasonPhrase_; }

/** @brief Marks whether the connection should be closed after sending this response. */
void HttpResponse::setShouldClose(bool close) { shouldClose_ = close; }

/** @brief Returns whether the connection should be closed after sending this response. */
bool HttpResponse::shouldClose() const { return shouldClose_; }

/**
 * @brief Replaces all existing values for a header with a single value.
 *
 * Header names are stored case-insensitively by normalising to lower-case.
 * Rejects CR/LF in name or value to prevent response splitting.
 */
void HttpResponse::setHeader(const std::string &name, const std::string &value)
{
	if (name.empty())
		return;
	if (containsCRLF(name) || containsCRLF(value))
		return;

	std::string key = string_parser::toLower(name);
	if (key == "content-length")
		return;

	std::vector<std::string> &values = headers_[key];
	values.clear();
	values.push_back(value);
}

/**
 * @brief Appends a value to a header (supports repeated headers like Set-Cookie).
 *
 * Header names are stored case-insensitively by normalising to lower-case.
 * Rejects CR/LF in name or value to prevent response splitting.
 */
void HttpResponse::addHeader(const std::string &name, const std::string &value)
{
	if (name.empty())
		return;
	if (containsCRLF(name) || containsCRLF(value))
		return;

	std::string key = string_parser::toLower(name);
	if (key == "content-length")
		return;

	headers_[key].push_back(value);
}

/** @brief Removes a header and all its values. */
void HttpResponse::removeHeader(const std::string &name)
{
	if (name.empty())
		return;
	headers_.erase(string_parser::toLower(name));
}

/** @brief Returns true if the header exists (case-insensitive). */
bool HttpResponse::hasHeader(const std::string &name) const
{
	if (name.empty())
		return false;
	return headers_.find(string_parser::toLower(name)) != headers_.end();
}

/**
 * @brief Returns the first value for a header, or empty string if not present.
 *
 * For multi-value headers (e.g. Set-Cookie), callers should use getHeaders()
 * if they need all values.
 */
std::string HttpResponse::getHeader(const std::string &name) const
{
	if (name.empty())
		return std::string();

	HeaderMap::const_iterator it = headers_.find(string_parser::toLower(name));
	if (it == headers_.end() || it->second.empty())
		return std::string();

	return it->second[0];
}

/** @brief Returns a read-only view of all headers and their values. */
const HttpResponse::HeaderMap &HttpResponse::getHeaders() const { return headers_; }

/** @brief Sets the response body (raw bytes stored in a std::string). */
void HttpResponse::setBody(const std::string &body) { body_ = body; }

/** @brief Returns the response body. */
const std::string &HttpResponse::getBody() const { return body_; }

/** @brief Returns the size of the response body in bytes. */
std::size_t HttpResponse::getBodySize() const { return body_.size(); }

/** @brief Convenience: sets the Content-Type header to the given value. */
void HttpResponse::setContentType(const std::string &value) { setHeader("Content-Type", value); }

/**
 * @brief Serialises the response into HTTP/1.x wire format (status line + headers + CRLF + body).
 *
 * Ensures Content-Length matches the current body size, and sets Connection header according
 * to the current shouldClose_ status.
 */
std::string HttpResponse::serialize() const
{
	std::ostringstream oss;

	oss << (version_ == HTTP_1_0 ? "HTTP/1.0 " : "HTTP/1.1 ");
	oss << statusCode_ << " " << reasonPhrase_ << "\r\n";

	HeaderMap h = headers_;

	std::string body = body_;
	if (statusCode_ == 204)
		body.clear();

	std::ostringstream len;
	len << body.size();
	h["content-length"].clear();
	h["content-length"].push_back(len.str());

	if (shouldClose_)
	{
		h["connection"].clear();
		h["connection"].push_back("close");
	}

	// For HTTP/1.0, always set Connection: close if not already present
	if (version_ == HTTP_1_0 && h.find("connection") == h.end())
	{
		h["connection"].push_back("close");
	}

	for (HeaderMap::const_iterator it = h.begin(); it != h.end(); ++it)
	{
		for (std::vector<std::string>::const_iterator v = it->second.begin();
			 v != it->second.end(); ++v)
			oss << it->first << ": " << *v << "\r\n";
	}

	oss << "\r\n";
	oss << body;
	return oss.str();
}

/**
 * @brief Returns a standard HTTP reason phrase for a known status code.
 *
 * Falls back to "Unknown" for codes not listed here.
 */
std::string HttpResponse::defaultReasonPhrase(int code)
{
	switch (code)
	{
	case HttpStatus::OK:
		return "OK";
	case HttpStatus::CREATED:
		return "Created";
	case HttpStatus::NO_CONTENT:
		return "No Content";
	case HttpStatus::MOVED_PERMANENTLY:
		return "Moved Permanently";
	case HttpStatus::FOUND:
		return "Found";
	case HttpStatus::BAD_REQUEST:
		return "Bad Request";
	case HttpStatus::FORBIDDEN:
		return "Forbidden";
	case HttpStatus::NOT_FOUND:
		return "Not Found";
	case HttpStatus::METHOD_NOT_ALLOWED:
		return "Method Not Allowed";
	case HttpStatus::REQUEST_TIMEOUT:
		return "Request Timeout";
	case HttpStatus::LENGTH_REQUIRED:
		return "Length Required";
	case HttpStatus::PAYLOAD_TOO_LARGE:
		return "Payload Too Large";
	case HttpStatus::REQUEST_HEADER_FIELDS_TOO_LARGE:
		return "Request Header Fields Too Large";
	case HttpStatus::INTERNAL_SERVER_ERROR:
		return "Internal Server Error";
	case HttpStatus::NOT_IMPLEMENTED:
		return "Not Implemented";
	case HttpStatus::HTTP_VERSION_NOT_SUPPORTED:
		return "HTTP Version Not Supported";
	default:
		return "Unknown";
	}
}

// ---------------------------- Private Methods -------------------------------

/**
 * @brief Returns true if string contains CR or LF characters.
 *
 * Used to prevent header injection (response splitting).
 */
bool HttpResponse::containsCRLF(const std::string &s)
{
	return (s.find('\r') != std::string::npos || s.find('\n') != std::string::npos);
}
