#include "config/config.hpp"
#include "config/config_file.hpp"
#include "http/http_method.hpp"
#include "logger.hpp"

Config::Config() : _serverConfigs(std::vector<ServerConfig>())
{
}

Config::~Config()
{
}

const std::vector<ServerConfig>& Config::getServerConfigs() const
{
	return this->_serverConfigs;
}

void Config::addServerConfig(const ServerConfig& serverConfig)
{
	this->_serverConfigs.push_back(serverConfig);
}

void Config::print() const
{
	if (!DEBUG)
		return;

	for (size_t i = 0; i < this->_serverConfigs.size(); ++i)
	{
		const ServerConfig& server = this->_serverConfigs[i];
		std::cout << "Server Block #" << (i + 1) << std::endl;

		std::cout << "  Listen:" << std::endl;
		for (size_t j = 0; j < server.listen.size(); ++j)
		{
			std::cout << "    " << server.listen[j].first << ":" << server.listen[j].second << std::endl;
		}

		if (!server.server_name.empty())
		{
			std::cout << "  Server Name: " << server.server_name << std::endl;
		}

		if (server.max_body_size > 0)
		{
			std::cout << "  Max Body Size: " << server.max_body_size << " bytes" << std::endl;
		}

		if (!server.error_pages.empty())
		{
			std::cout << "  Error Pages:" << std::endl;
			for (std::map<int, std::string>::const_iterator it = server.error_pages.begin();
				 it != server.error_pages.end(); ++it)
			{
				std::cout << "    " << it->first << " -> " << it->second << std::endl;
			}
		}

		if (!server.root.empty())
		{
			std::cout << "  Root: " << server.root << std::endl;
		}

		if (!server.locations.empty())
		{
			std::cout << "  Locations (" << server.locations.size() << "):" << std::endl;
			for (size_t j = 0; j < server.locations.size(); ++j)
			{
				const LocationConfig& loc = server.locations[j];
				std::cout << "    Location: " << loc.path << std::endl;

				if (!loc.methods.empty())
				{
					std::cout << "      Methods: ";
					bool first = true;
					for (std::set<HttpMethod>::const_iterator it = loc.methods.begin();
					     it != loc.methods.end(); ++it)
					{
						if (!first)
							std::cout << ", ";
						first = false;
						std::cout << httpMethodToString(*it);
					}
					std::cout << std::endl;
				}

				if (!loc.index.empty())
				{
					std::cout << "      Index: " << loc.index << std::endl;
				}

				std::cout << "      Directory Listing: " << (loc.directory_listing ? "on" : "off") << std::endl;

				if (!loc.upload_dir.empty())
				{
					std::cout << "      Upload Dir: " << loc.upload_dir << std::endl;
				}

				if (!loc.cgi_extension.empty())
				{
					std::cout << "      CGI Extension: " << loc.cgi_extension << std::endl;
				}

				if (loc.redirect_code > 0)
				{
					std::cout << "      Return: " << loc.redirect_code << " " << loc.redirect_path << std::endl;
				}
			}
		}

		std::cout << std::endl;
	}
}
