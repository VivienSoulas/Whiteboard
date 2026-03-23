#ifndef STATIC_FILE_HANDLER_HPP
#define STATIC_FILE_HANDLER_HPP

#include <string>
#include "config/server/server_config.hpp"
#include "config/location/location_config.hpp"
#include "http/http_request.hpp"

namespace static_file_handler
{
	std::string serve(const ServerConfig &server, const LocationConfig &location, const std::string &path, bool head_only, int* out_status = nullptr);
}

#endif // STATIC_FILE_HANDLER_HPP
