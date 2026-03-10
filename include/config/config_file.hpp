#ifndef CONFIG_FILE_HPP
#define CONFIG_FILE_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include "config/config.hpp"
#include "config/config_parser.hpp"

class ConfigFile
{
public:
	ConfigFile(const std::string &path);
	~ConfigFile();
	const Config &getConfig() const;

private:
	std::string _path;
	Config _config;
	std::string readFile();
};

#endif
