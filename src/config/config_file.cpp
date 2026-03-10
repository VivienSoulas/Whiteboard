#include "config/config_file.hpp"

ConfigFile::ConfigFile(const std::string &path) : _path(path)
{
	std::string content = this->readFile();
	ConfigParser parser(content);
	this->_config = parser.parse();
}

ConfigFile::~ConfigFile()
{
}

const Config &ConfigFile::getConfig() const
{
	return this->_config;
}

std::string ConfigFile::readFile()
{
	std::ifstream file(this->_path.c_str());
	if (!file.is_open())
	{
		std::string error_msg = "Failed to open config file: " + this->_path;
		error_msg += " (";
		error_msg += strerror(errno);
		error_msg += ")";
		throw std::runtime_error(error_msg);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}
