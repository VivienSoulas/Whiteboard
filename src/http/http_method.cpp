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
	else if (upper_method == "PUT")
		return PUT;
	else if (upper_method == "PATCH")
		return PATCH;
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
		case PUT:    return "PUT";
		case PATCH:  return "PATCH";
		default:     return "UNKNOWN";
	}
}
