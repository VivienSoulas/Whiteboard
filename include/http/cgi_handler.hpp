#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include "config/server/server_config.hpp"
#include "config/location/location_config.hpp"
#include "http/http_request.hpp"

namespace cgi_handler
{
	std::string handle(const ServerConfig &server, const LocationConfig &location, const HttpRequest &req);
}

#endif
