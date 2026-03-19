#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "config/location/location_config.hpp"

class ServerConfig
{
public:
	ServerConfig();
	~ServerConfig();

	std::vector<std::pair<std::string, std::string>> listen;
	std::string server_name;
	size_t max_body_size;
	std::map<int, std::string> error_pages;
	std::string root;
	std::vector<LocationConfig> locations;

	// SSL/TLS configuration
	bool ssl_enabled;
	std::string ssl_certificate_path;
	std::string ssl_certificate_key_path;
	std::string ssl_port;
};

#endif
