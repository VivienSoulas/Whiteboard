#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include "http/http_method.hpp"

struct HttpRequest
{
	enum Version
	{
		UNKNOWN_VERSION,
		HTTP_1_0,
		HTTP_1_1
	};

	typedef std::map<std::string, std::string> HeaderMap;

	HttpMethod		method;
	Version			version;

	std::string		target;
	std::string		path;
	std::string		query;

	HeaderMap		headers;
	std::string		body;

	HttpRequest() :
		method(UNKNOWN),
		version(UNKNOWN_VERSION),
		target(),
		path(),
		query(),
		headers(),
		body()
	{}
};

#endif
