#include "http/http_method.hpp"

HttpMethod parseHttpMethod(const std::string& method_str)
{
	std::string upper_method = method_str;
	std::transform(upper_method.begin(), upper_method.end(),
	              upper_method.begin(), ::toupper);

	if (upper_method == "GET")
		return GET;
	else if (upper_method == "POST")
		return POST;
	else if (upper_method == "DELETE")
		return DELETE;
	else if (upper_method == "HEAD")
		return HEAD;
	else
		return UNKNOWN;
}

std::string httpMethodToString(HttpMethod m)
{
	switch (m)
	{
		case GET:    return "GET";
		case POST:   return "POST";
		case DELETE: return "DELETE";
		case HEAD:   return "HEAD";
		default:     return "UNKNOWN";
	}
}
