#ifndef HTTP_METHOD_HPP
#define HTTP_METHOD_HPP

#include <string>
#include <algorithm>

enum HttpMethod
{
	UNKNOWN,
	GET,
	POST,
	DELETE,
	HEAD
};

HttpMethod parseHttpMethod(const std::string &method_str);

std::string httpMethodToString(HttpMethod m);

#endif
