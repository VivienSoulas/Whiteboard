#ifndef HTTPRESPONSEFACTORY_HPP
#define HTTPRESPONSEFACTORY_HPP

#include "http/http_response.hpp"
#include "http/http_request.hpp"
#include "config/server/server_config.hpp"
#include <string>
#include <vector>

class HttpResponseFactory
{
public:
	static HttpResponse buildError(const HttpRequest &req, const ServerConfig *server, int statusCode, bool close);
	static HttpResponse buildOk(const HttpRequest &req, const std::string &body,
								const std::string &contentType, bool close);
	static HttpResponse buildMethodNotAllowed(const HttpRequest &req,	
											  const std::vector<std::string> &allowed, bool close);
	static HttpResponse buildVersionNotSupported(bool close);
	static HttpResponse buildRedirect(const HttpRequest &req,
									  int statusCode, const std::string &location, bool close);

private:
	static void applyConnectionPolicy(const HttpRequest &req, HttpResponse &res, bool close);
	static std::string joinAllow(const std::vector<std::string> &allowed);
	static std::string makeDefaultErrorBody(int statusCode, const std::string &reason);
	static std::string escapeHtml(const std::string &s);
};

#endif
