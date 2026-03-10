#include "config/location/location_config.hpp"

LocationConfig::LocationConfig()
	: path(""), root(""), methods(std::set<HttpMethod>()), index(""),
	  directory_listing(false), upload_dir(""), cgi_extension(""),
	  redirect_code(0), redirect_path(""), max_body_size(0)
{
}

LocationConfig::~LocationConfig()
{
}

void LocationConfig::setMethods(const std::vector<HttpMethod> &methods)
{
	this->methods.clear();
	for (size_t i = 0; i < methods.size(); ++i)
	{
		this->methods.insert(methods[i]);
	}
}
