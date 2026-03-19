#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <map>
#include <vector>
#include "io/connection.hpp"
#include "io/tls_connection.hpp"
#include "http/http_parser.hpp"
#include "router.hpp"
#include "config/server/server_config.hpp"
#include <string>
#include "io/socket.hpp"
#include "io/tls_socket.hpp"
#include <functional>
#include <openssl/ssl.h>

class ConnectionManager
{
public:
	using HandleRequestFunc = std::function<std::string(const HttpRequest &, const std::string &, const std::string &)>;

	ConnectionManager(Router &router, HandleRequestFunc handleRequest)
		: _router(router), _handleRequest(handleRequest) {}
	~ConnectionManager();

	void addListener(Socket *socket, const std::string &addr, const std::string &port);
	void addTlsListener(TlsSocket *socket, const std::string &addr, const std::string &port);
	void pollConnections();
	bool hasActiveConnections() const;

private:
	std::map<int, Connection *> _connections;
	std::map<int, TlsConnection *> _tls_connections;
	std::map<int, HttpParser> _parsers;
	std::vector<Socket *> _listener_sockets;
	std::vector<TlsSocket *> _tls_listener_sockets;
	std::map<int, std::pair<std::string, std::string>> _listener_fd_to_addr_port;
	std::map<int, std::pair<std::string, std::string>> _tls_listener_fd_to_addr_port;
	Router &_router;
	HandleRequestFunc _handleRequest;
};

#endif // CONNECTION_MANAGER_HPP
