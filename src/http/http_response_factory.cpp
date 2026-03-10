#include "http/http_response_factory.hpp"
#include "http/http_status.hpp"
#include <sstream>
#include "logger.hpp"

// -------------------------------- Public API --------------------------------

/**
 * @brief Builds a generic HTML error response for a given status code.
 *
 * Sets status, a simple HTML body, Content-Type, and applies connection policy.
 */
HttpResponse HttpResponseFactory::buildError(const HttpRequest &req, int statusCode, bool close)
{
	DEBUG_LOG("Building error response: " << statusCode << " (request path: " << req.path << ")");
	HttpResponse res;
	applyConnectionPolicy(req, res, close);

	res.setStatus(statusCode);
	res.setContentType("text/html; charset=utf-8");
	res.setBody(makeDefaultErrorBody(statusCode, res.getReasonPhrase()));
	return res;
}

/**
 * @brief Builds a 200 OK response with a provided body and Content-Type.
 *
 * Applies connection policy and stores body; Content-Length is finalised in serialize().
 */
HttpResponse HttpResponseFactory::buildOk(const HttpRequest &req, const std::string &body,
										  const std::string &contentType, bool close)
{
	HttpResponse res;
	applyConnectionPolicy(req, res, close);

	res.setStatus(HttpStatus::OK);
	res.setContentType(contentType.empty() ? "application/octet-stream" : contentType);
	res.setBody(body);
	return res;
}

/**
 * @brief Builds a 405 Method Not Allowed response with an Allow header.
 *
 * Adds an "Allow" header containing the permitted methods and a simple HTML body.
 */
HttpResponse HttpResponseFactory::buildMethodNotAllowed(const HttpRequest &req,
														const std::vector<std::string> &allowed, bool close)
{
	HttpResponse res;
	applyConnectionPolicy(req, res, close);

	res.setStatus(HttpStatus::METHOD_NOT_ALLOWED);
	res.setHeader("Allow", joinAllow(allowed));
	res.setContentType("text/html; charset=utf-8");
	res.setBody(makeDefaultErrorBody(HttpStatus::METHOD_NOT_ALLOWED, res.getReasonPhrase()));
	return res;
}

/**
 * @brief Builds a 505 HTTP Version Not Supported response.
 *
 * Does not depend on a parsed request; defaults to HTTP/1.1 status line.
 */
HttpResponse HttpResponseFactory::buildVersionNotSupported(bool close)
{
	HttpResponse res;

	res.setShouldClose(close);
	res.setStatus(HttpStatus::HTTP_VERSION_NOT_SUPPORTED);
	res.setContentType("text/html; charset=utf-8");
	res.setBody(makeDefaultErrorBody(HttpStatus::HTTP_VERSION_NOT_SUPPORTED, res.getReasonPhrase()));
	return res;
}

/**
 * @brief Builds a redirect response with a Location header and minimal HTML body.
 *
 * Intended for 301/302. The Location value is used as-is; calling function
 * should pass an absolute-path or absolute URI.
 */
HttpResponse HttpResponseFactory::buildRedirect(const HttpRequest &req,
												int statusCode, const std::string &location, bool close)
{
	DEBUG_LOG("Building redirect response: " << statusCode << " -> " << location << " (request path: " << req.path << ")");
	HttpResponse res;
	applyConnectionPolicy(req, res, close);

	res.setStatus(statusCode);
	res.setHeader("Location", location);
	res.setContentType("text/html; charset=utf-8");

	std::ostringstream oss;
	oss << "<!doctype html><html><head><meta charset=\"utf-8\">"
		<< "<title>" << statusCode << " " << escapeHtml(res.getReasonPhrase()) << "</title>"
		<< "</head><body>"
		<< "<h1>" << statusCode << " " << escapeHtml(res.getReasonPhrase()) << "</h1>"
		<< "<p>Redirecting to <a href=\"" << escapeHtml(location) << "\">"
		<< escapeHtml(location) << "</a>.</p>"
		<< "</body></html>";

	res.setBody(oss.str());
	return res;
}

// ---------------------------- Private Methods -------------------------------

/**
 * @brief Applies HTTP version and connection-close policy to a response.
 *
 * Mirrors the request HTTP version when known and sets whether the connection
 * should be closed after sending the response.
 */
void HttpResponseFactory::applyConnectionPolicy(const HttpRequest &req, HttpResponse &res, bool close)
{
	if (req.version == HttpRequest::HTTP_1_0)
		res.setVersion(HttpResponse::HTTP_1_0);
	else
		res.setVersion(HttpResponse::HTTP_1_1);

	res.setShouldClose(close);
}

/**
 * @brief Joins a list of allowed methods for the Allow header.
 *
 * Produces a comma + space separated string, e.g. "GET, POST, DELETE".
 */
std::string HttpResponseFactory::joinAllow(const std::vector<std::string> &allowed)
{
	std::ostringstream oss;
	for (std::size_t i = 0; i < allowed.size(); ++i)
	{
		if (i != 0)
			oss << ", ";
		oss << allowed[i];
	}
	return oss.str();
}

/**
 * @brief Builds a minimal HTML error body for a given HTTP status.
 *
 * Escapes the reason phrase to avoid HTML injection in the generated body.
 */
std::string HttpResponseFactory::makeDefaultErrorBody(int statusCode, const std::string &reason)
{
	std::ostringstream oss;
	oss << "<!doctype html><html><head><meta charset=\"utf-8\">"
		<< "<title>" << statusCode << " " << escapeHtml(reason) << "</title>"
		<< "</head><body>"
		<< "<h1>" << statusCode << " " << escapeHtml(reason) << "</h1>"
		<< "</body></html>";
	return oss.str();
}

/**
 * @brief Escapes a string for safe inclusion in HTML text/attributes.
 *
 * Replaces &, <, >, ", ' with HTML entities.
 */
std::string HttpResponseFactory::escapeHtml(const std::string &s)
{
	std::string out;
	out.reserve(s.size());

	for (std::string::size_type i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		if (c == '&')
			out += "&amp;";
		else if (c == '<')
			out += "&lt;";
		else if (c == '>')
			out += "&gt;";
		else if (c == '"')
			out += "&quot;";
		else if (c == '\'')
			out += "&#39;";
		else
			out.push_back(c);
	}
	return out;
}
