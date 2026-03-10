#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <set>
#include <vector>
#include "http/http_method.hpp"

class LocationConfig
{
public:
	LocationConfig();
	~LocationConfig();

	std::string path;
	std::string root;
	std::set<HttpMethod> methods;
	std::string index;
	bool directory_listing;
	std::string upload_dir;
	std::string cgi_extension;
	int redirect_code;
	std::string redirect_path;
	size_t max_body_size;

	void setMethods(const std::vector<HttpMethod> &methods);
};

#endif
