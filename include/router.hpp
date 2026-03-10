#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>
#include "http/http_method.hpp"
#include "config/config.hpp"
#include "config/server/server_config.hpp"
#include "config/location/location_config.hpp"

class Router
{
public:
	Router(const Config &config);

	const ServerConfig *getServerForListen(const std::string &listen_addr, const std::string &listen_port) const;
	const LocationConfig *getLocationForPath(const std::string &path, const std::string &host, const std::string &listen_addr, const std::string &listen_port) const;
	const LocationConfig *route(const HttpMethod &method, const std::string &path, const std::string &host, const std::string &listen_addr, const std::string &listen_port);

	const ServerConfig *selectServer(const std::string &host, const std::string &listen_addr, const std::string &listen_port) const;
	const LocationConfig *matchLocation(const ServerConfig &server, const std::string &path) const;
	
private:
	const Config &_config;
	static const std::string ANY_ADDRESS;

	bool isMethodAllowed(const LocationConfig &location, const HttpMethod &method) const;
};

#endif
