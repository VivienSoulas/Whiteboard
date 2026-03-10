#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include "string/string_parser.hpp"
#include "config/config.hpp"
#include "config/server/server_config.hpp"
#include "config/location/location_config.hpp"

class ConfigParser : public StringParser
{
public:
	ConfigParser(const std::string &content);
	~ConfigParser();
	Config parse();

private:
	void parseServerBlock(ServerConfig &server);
	void parseLocationBlock(ServerConfig &server);
	void parseLocationDirectives(LocationConfig &location);
};

#endif
