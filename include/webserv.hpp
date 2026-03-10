#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "config/config_file.hpp"
#include "config/config.hpp"
#include "config/server/server_config.hpp"
#include "config/location/location_config.hpp"
#include "io/socket.hpp"
#include "io/connection_manager.hpp"
#include "http/http_parser.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/http_response_factory.hpp"
#include "http/http_status.hpp"
#include "router.hpp"
#include "http/http_method.hpp"
#include "path/path_utils.hpp"
#include <map>
#include <set>
#include <vector>
#include <poll.h>
#include <fstream>
#include <set>
#include <vector>
#include <poll.h>
#include <fstream>

#ifndef POLL_TIMEOUT
#define POLL_TIMEOUT 120000 // 120 seconds (for poll() in milliseconds)
#endif

class Webserv
{
public:
	Webserv(const std::string &config_path);
	~Webserv();
	// Accept new connections and process all connections (read/write/timeout/close)
	void accept();

private:
	ConfigFile _config_file;
	Router _router;
	ConnectionManager _connection_manager;
	std::map<int, std::pair<std::string, std::string>> _listener_fd_to_addr_port;

	void setupListeners();
	std::string handleRequest(const HttpRequest &req, const std::string &listen_addr,
							  const std::string &listen_port);
	std::vector<std::string> locationMethodsToAllow(const LocationConfig &loc);
};

#endif
