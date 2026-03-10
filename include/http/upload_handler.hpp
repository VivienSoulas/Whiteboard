#ifndef UPLOAD_HANDLER_HPP
#define UPLOAD_HANDLER_HPP

#include <string>
#include "config/server/server_config.hpp"
#include "config/location/location_config.hpp"
#include "http/http_request.hpp"

namespace upload_handler
{
	std::string handle(const LocationConfig &location, const HttpRequest &req);
}

#endif // UPLOAD_HANDLER_HPP
