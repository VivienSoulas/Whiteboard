#include "router.hpp"
#include "http/http_method.hpp"
#include "logger.hpp"

const std::string Router::ANY_ADDRESS = "0.0.0.0";

Router::Router(const Config& config) : _config(config)
{
}

const ServerConfig* Router::getServerForListen(const std::string& listen_addr, const std::string& listen_port) const
{
	return selectServer("", listen_addr, listen_port);
}

const LocationConfig* Router::getLocationForPath(const std::string& path, const std::string& host, const std::string& listen_addr, const std::string& listen_port) const
{
	const ServerConfig* server = selectServer(host, listen_addr, listen_port);
	if (!server)
		return nullptr;
	return matchLocation(*server, path);
}

const LocationConfig* Router::route(const HttpMethod& method, const std::string& path, const std::string& host, const std::string& listen_addr, const std::string& listen_port)
{
	if (path.empty())
	{
		return nullptr;
	}

	const ServerConfig* server = selectServer(host, listen_addr, listen_port);
	if (!server)
	{
		return nullptr;
	}

	const LocationConfig* location = matchLocation(*server, path);
	if (!location)
	{
		return nullptr;
	}

	if (!isMethodAllowed(*location, method))
	{
		return nullptr;
	}

	return location;
}

const ServerConfig* Router::selectServer(const std::string& host, const std::string& listen_addr, const std::string& listen_port) const
{
	const std::vector<ServerConfig>& servers = _config.getServerConfigs();
	const ServerConfig* default_server = nullptr;
	const ServerConfig* name_match_server = nullptr;

	for (const auto& server : servers)
	{
		bool matches_listen = false;

		for (const auto& listen_pair : server.listen)
		{
			const std::string& server_addr = listen_pair.first;
			const std::string& server_port = listen_pair.second;

			bool addr_matches = (server_addr == listen_addr) ||
			                    (server_addr == ANY_ADDRESS) ||
			                    (listen_addr == ANY_ADDRESS);
							
			bool port_matches = (server_port == listen_port);

			if (addr_matches && port_matches)
			{
				matches_listen = true;
				break;
			}
		}

		if (!matches_listen && !server.ssl_port.empty())
		{
			bool addr_matches = true;
			bool port_matches = (server.ssl_port == listen_port);
			
			if (addr_matches && port_matches)
			{
				matches_listen = true;
			}
		}

		if (matches_listen)
		{
			if (!default_server)
			{
				default_server = &server;
			}

			if (!host.empty() && !server.server_name.empty() && server.server_name == host)
			{
				name_match_server = &server;
				break;
			}
		}
	}

	const ServerConfig* selected = name_match_server ? name_match_server : default_server;
	return selected;
}

const LocationConfig* Router::matchLocation(const ServerConfig& server,
                                            const std::string& path) const
{
	const LocationConfig* best_match = nullptr;
	size_t best_match_length = 0;

	for (const auto& location : server.locations)
	{
		const std::string& location_path = location.path;

		if (location_path.empty())
		{
			continue;
		}

		// EXTENSION MATCH: if path starts with '.' and doesn't have a '/'
		if (location_path[0] == '.' && location_path.find('/') == std::string::npos)
		{
			size_t dot_pos = path.find_last_of('.');
			if (dot_pos != std::string::npos)
			{
				std::string ext = path.substr(dot_pos);
				if (ext == location_path)
				{
					// Extension matches always win over prefix matches in this implementation
					best_match = &location;
					best_match_length = 10000 + location_path.length(); 
					continue;
				}
			}
		}

		// PREFIX MATCH logic
		if (path.find(location_path) == 0)
		{
			bool is_valid_match = false;
			if (location_path == "/")
			{
				is_valid_match = true;
			}
			else if (path.length() == location_path.length())
			{
				is_valid_match = true;
			}
			else if (path.length() > location_path.length())
			{
				if (path[location_path.length()] == '/')
				{
					is_valid_match = true;
				}
			}
			
			if (is_valid_match && location_path.length() > best_match_length)
			{
				best_match = &location;
				best_match_length = location_path.length();
			}
		}
	}

	if (best_match) {
		DEBUG_LOG("Matched location: " << best_match->path << " (for path: " << path << ")");
	} else {
		DEBUG_LOG("No location matched for path: " << path);
	}
	return best_match;
}

bool Router::isMethodAllowed(const LocationConfig& location,
                              const HttpMethod& method) const
{
	return location.methods.find(method) != location.methods.end();
}
