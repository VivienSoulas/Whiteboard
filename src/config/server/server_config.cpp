#include "config/server/server_config.hpp"

ServerConfig::ServerConfig()
	: listen(std::vector<std::pair<std::string, std::string>>()),
	  server_name(""), max_body_size(0), error_pages(std::map<int, std::string>()),
	  root(""), locations(std::vector<LocationConfig>()),
	  ssl_enabled(false), ssl_certificate_path(""), ssl_certificate_key_path(""), ssl_port("")
{
}

ServerConfig::~ServerConfig()
{
}
