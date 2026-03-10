#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <iostream>
#include "config/server/server_config.hpp"

class Config
{
public:
	Config();
	~Config();
	const std::vector<ServerConfig> &getServerConfigs() const;
	void addServerConfig(const ServerConfig &serverConfig);
	void print() const;

private:
	std::vector<ServerConfig> _serverConfigs;
};

#endif
