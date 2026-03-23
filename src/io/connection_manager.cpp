#include "io/connection_manager.hpp"
#include "io/connection.hpp"
#include "io/tls_connection.hpp"
#include "http/http_parser.hpp"
#include "http/http_response_factory.hpp"
#include "http/http_request.hpp"
#include "config/server/server_config.hpp"
#include "io/utils.hpp"
#include "router.hpp"
#include "logger.hpp"
#include "string/string_parser.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <iostream>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <set>
#include <string>
#include <ctime>
#include <fcntl.h>
#include <arpa/inet.h>
#include "webserv.hpp"

static const int MAX_CONNECTIONS = 1000;
static const int MAX_CONNECTIONS_PER_IP = 50;

ConnectionManager::~ConnectionManager()
{
	for (auto &pair : _connections)
	{
		if (pair.second)
			delete pair.second;
	}
	_connections.clear();

	for (auto &pair : _tls_connections)
	{
		if (pair.second)
			delete pair.second;
	}
	_tls_connections.clear();

	for (auto &it : _listener_sockets)
	{
		delete (it);
	}
	for (auto &it : _tls_listener_sockets)
	{
		delete (it);
	}
	DEBUG_LOG("All connections cleaned up");
}

bool ConnectionManager::hasActiveConnections() const
{
	return !_listener_sockets.empty() || !_tls_listener_sockets.empty();
}

void ConnectionManager::addListener(Socket *socket, const std::string &addr, const std::string &port)
{
	_listener_sockets.push_back(socket);
	_listener_fd_to_addr_port[socket->getFd()] = std::make_pair(addr, port);
}

void ConnectionManager::addTlsListener(TlsSocket *socket, const std::string &addr, const std::string &port)
{
	_tls_listener_sockets.push_back(socket);
	_tls_listener_fd_to_addr_port[socket->getFd()] = std::make_pair(addr, port);
}

void ConnectionManager::pollConnections()
{
	std::vector<pollfd> poll_fds;
	time_t current_time = time(NULL);

	// Clean up regular connections marked for closing or timeout
	for (std::map<int, Connection *>::iterator it = _connections.begin(); it != _connections.end();)
	{
		if (it->second->getState() == CLOSING || it->second->isTimeOut(current_time))
		{
			DEBUG_LOG("Connection closed [fd: " << it->first << "]");
			_parsers.erase(it->first);
			delete it->second;
			it = _connections.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Clean up TLS connections marked for closing or timeout
	for (std::map<int, TlsConnection *>::iterator it = _tls_connections.begin(); it != _tls_connections.end();)
	{
		if (it->second->getState() == CLOSING || it->second->isTimeOut(current_time))
		{
			DEBUG_LOG("TLS connection closed [fd: " << it->first << "]");
			_parsers.erase(it->first);
			delete it->second;
			it = _tls_connections.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Build poll array for listener sockets
	for (size_t i = 0; i < _listener_sockets.size(); ++i)
	{
		poll_fds.push_back((pollfd){_listener_sockets[i]->getFd(), POLLIN, 0});
	}

	// Build poll array for TLS listener sockets
	for (size_t i = 0; i < _tls_listener_sockets.size(); ++i)
	{
		poll_fds.push_back((pollfd){_tls_listener_sockets[i]->getFd(), POLLIN, 0});
	}

	// Build poll array for client connections
	for (std::map<int, Connection *>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		short events = 0;
		if (it->second->getState() == READING)
			events = POLLIN;
		else if (it->second->getState() == WRITTING)
			events = POLLOUT;
		poll_fds.push_back((pollfd){it->first, events, 0});
	}

	// Build poll array for TLS client connections
	for (std::map<int, TlsConnection *>::iterator it = _tls_connections.begin(); it != _tls_connections.end(); ++it)
	{
		short events = 0;
		State state = it->second->getState();
		if (state == READING)
			events = POLLIN;
		else if (state == WRITTING)
			events = POLLOUT;
		else if (state == TLS_HANDSHAKE_READING)
			events = POLLIN;
		else if (state == TLS_HANDSHAKE_WRITING)
			events = POLLOUT;
		poll_fds.push_back((pollfd){it->first, events, 0});
	}

	if (poll_fds.empty())
		return;

	int n = poll(poll_fds.data(), poll_fds.size(), POLL_TIMEOUT);
	if (n < 0)
	{
		if (errno != EINTR)
			DEBUG_LOG("poll() error: " << strerror(errno));
		return;
	}

	for (size_t i = 0; i < poll_fds.size(); i++)
	{
		int fd = poll_fds[i].fd;

		// Check if it's a regular listener socket
		bool is_listener = false;
		for (size_t j = 0; j < _listener_sockets.size(); ++j)
		{
			if (_listener_sockets[j]->getFd() == fd)
			{
				is_listener = true;
				if (poll_fds[i].revents & POLLIN)
				{
					// Connection rate limiting
					int total_connections = _connections.size() + _tls_connections.size();
					if (total_connections >= MAX_CONNECTIONS)
					{
						DEBUG_LOG("Connection rejected: max connections (" << MAX_CONNECTIONS << ") reached");
						sockaddr_storage client_addr;
						socklen_t addr_len = sizeof(client_addr);
						int client_fd = _listener_sockets[j]->accept_connection(client_addr, addr_len);
						if (client_fd > 0) close(client_fd);
						break;
					}

					std::map<int, std::pair<std::string, std::string> >::iterator lit = _listener_fd_to_addr_port.find(fd);
					if (lit != _listener_fd_to_addr_port.end())
					{
						sockaddr_storage client_addr;
						socklen_t addr_len = sizeof(client_addr);
						int client_fd = _listener_sockets[j]->accept_connection(client_addr, addr_len);
						if (client_fd > 0)
						{
							// Extract client IP and check per-IP limit
							char client_ip[INET6_ADDRSTRLEN] = {0};
						int inet_result = 0;
						if (client_addr.ss_family == AF_INET)
							inet_result = (inet_ntop(AF_INET, &((struct sockaddr_in *)&client_addr)->sin_addr, client_ip, sizeof(client_ip)) != NULL) ? 1 : 0;
						else if (client_addr.ss_family == AF_INET6)
							inet_result = (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&client_addr)->sin6_addr, client_ip, sizeof(client_ip)) != NULL) ? 1 : 0;
						if (!inet_result)
							continue;  // Skip if conversion failed

						// Count connections from this IP
						int ip_conn_count = 1;  // Count this incoming connection
						for (std::map<int, Connection *>::const_iterator it = _connections.begin(); it != _connections.end(); ++it)
						{
							sockaddr_storage existing_addr;
							socklen_t existing_addr_len = sizeof(existing_addr);
							if (getpeername(it->first, (struct sockaddr *)&existing_addr, &existing_addr_len) == 0)
							{
								char existing_ip[INET6_ADDRSTRLEN] = {0};
								int inet_result_existing = 0;
								if (existing_addr.ss_family == AF_INET)
									inet_result_existing = (inet_ntop(AF_INET, &((struct sockaddr_in *)&existing_addr)->sin_addr, existing_ip, sizeof(existing_ip)) != NULL) ? 1 : 0;
								else if (existing_addr.ss_family == AF_INET6)
									inet_result_existing = (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&existing_addr)->sin6_addr, existing_ip, sizeof(existing_ip)) != NULL) ? 1 : 0;
								if (!inet_result_existing)
									continue;  // Skip if conversion failed
								
								if (std::string(existing_ip) == std::string(client_ip))
									ip_conn_count++;
							}
						}

							try
							{
								makeNonBlocking(client_fd);
								Connection *conn = new Connection(client_fd, client_addr, addr_len, lit->second.first, lit->second.second);
								_connections[client_fd] = conn;
								_parsers[client_fd] = HttpParser();
								const ServerConfig *server = _router.getServerForListen(lit->second.first, lit->second.second);
								if (server && server->max_body_size > 0)
									_parsers[client_fd].setMaxBodyBytes(server->max_body_size);
								DEBUG_LOG("New connection [fd: " << client_fd << "] from " << client_ip << " (default server: " << (server ? server->server_name : "none") << ")");
							}
							catch (const std::exception &e)
							{
								DEBUG_LOG("Failed to setup new connection [fd: " << client_fd << "]: " << e.what());
								close(client_fd);
							}
						}
					}
				}
				break;
			}
		}

		if (is_listener)
			continue;

		// Check if it's a TLS listener socket
		is_listener = false;
		for (size_t j = 0; j < _tls_listener_sockets.size(); ++j)
		{
			if (_tls_listener_sockets[j]->getFd() == fd)
			{
				is_listener = true;
				if (poll_fds[i].revents & POLLIN)
				{
					// Connection rate limiting
					int total_connections = _connections.size() + _tls_connections.size();
					if (total_connections >= MAX_CONNECTIONS)
					{
						DEBUG_LOG("TLS connection rejected: max connections (" << MAX_CONNECTIONS << ") reached");
						sockaddr_storage client_addr;
						socklen_t addr_len = sizeof(client_addr);
						SSL *ssl = _tls_listener_sockets[j]->accept_connection_tls(client_addr, addr_len);
						if (ssl) SSL_free(ssl);
						break;
					}

					std::map<int, std::pair<std::string, std::string> >::iterator lit = _tls_listener_fd_to_addr_port.find(fd);
					if (lit != _tls_listener_fd_to_addr_port.end())
					{
						sockaddr_storage client_addr;
						socklen_t addr_len = sizeof(client_addr);
						SSL *ssl = _tls_listener_sockets[j]->accept_connection_tls(client_addr, addr_len);
					if (ssl)
						{
							// Extract client IP and check per-IP limit
							char client_ip[INET6_ADDRSTRLEN] = {0};
						int inet_result = 0;
						if (client_addr.ss_family == AF_INET)
							inet_result = (inet_ntop(AF_INET, &((struct sockaddr_in *)&client_addr)->sin_addr, client_ip, sizeof(client_ip)) != NULL) ? 1 : 0;
						else if (client_addr.ss_family == AF_INET6)
							inet_result = (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&client_addr)->sin6_addr, client_ip, sizeof(client_ip)) != NULL) ? 1 : 0;
						if (!inet_result)
							continue;  // Skip if conversion failed

						// Count connections from this IP (both regular and TLS)
						int ip_conn_count = 1;  // Count this incoming connection
						
						// Count from regular connections
						for (std::map<int, Connection *>::const_iterator it = _connections.begin(); it != _connections.end(); ++it)
						{
							sockaddr_storage existing_addr;
							socklen_t existing_addr_len = sizeof(existing_addr);
							if (getpeername(it->first, (struct sockaddr *)&existing_addr, &existing_addr_len) == 0)
							{
								char existing_ip[INET6_ADDRSTRLEN] = {0};
								int inet_result_existing = 0;
								if (existing_addr.ss_family == AF_INET)
									inet_result_existing = (inet_ntop(AF_INET, &((struct sockaddr_in *)&existing_addr)->sin_addr, existing_ip, sizeof(existing_ip)) != NULL) ? 1 : 0;
								else if (existing_addr.ss_family == AF_INET6)
									inet_result_existing = (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&existing_addr)->sin6_addr, existing_ip, sizeof(existing_ip)) != NULL) ? 1 : 0;
								if (!inet_result_existing)
									continue;  // Skip if conversion failed
								
								if (std::string(existing_ip) == std::string(client_ip))
									ip_conn_count++;
							}
						}
						
						// Count from TLS connections
						for (std::map<int, TlsConnection *>::const_iterator it = _tls_connections.begin(); it != _tls_connections.end(); ++it)
						{
							sockaddr_storage existing_addr;
							socklen_t existing_addr_len = sizeof(existing_addr);
							if (getpeername(it->first, (struct sockaddr *)&existing_addr, &existing_addr_len) == 0)
							{
								char existing_ip[INET6_ADDRSTRLEN] = {0};
								int inet_result_existing = 0;
								if (existing_addr.ss_family == AF_INET)
									inet_result_existing = (inet_ntop(AF_INET, &((struct sockaddr_in *)&existing_addr)->sin_addr, existing_ip, sizeof(existing_ip)) != NULL) ? 1 : 0;
								else if (existing_addr.ss_family == AF_INET6)
									inet_result_existing = (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&existing_addr)->sin6_addr, existing_ip, sizeof(existing_ip)) != NULL) ? 1 : 0;
								if (!inet_result_existing)
									continue;  // Skip if conversion failed
								
								if (std::string(existing_ip) == std::string(client_ip))
									ip_conn_count++;
							}
						}

							// Get the actual client fd from the ssl connection
							int client_fd = SSL_get_fd(ssl);
						if (client_fd != -1)
							{
								try
								{
									makeNonBlocking(client_fd);
									TlsConnection *conn = new TlsConnection(client_fd, client_addr, addr_len, lit->second.first, lit->second.second, ssl);
									_tls_connections[client_fd] = conn;
									_parsers[client_fd] = HttpParser();
									const ServerConfig *server = _router.getServerForListen(lit->second.first, lit->second.second);
									if (server && server->max_body_size > 0)
										_parsers[client_fd].setMaxBodyBytes(server->max_body_size);
									DEBUG_LOG("New TLS connection [fd: " << client_fd << "] from " << client_ip << " (default server: " << (server ? server->server_name : "none") << ")");
								}
								catch (const std::exception &e)
								{
									DEBUG_LOG("Failed to setup new TLS connection [fd: " << client_fd << "]: " << e.what());
									SSL_free(ssl);
									close(client_fd);
								}
							}
						}
					}
				}
				break;
			}
		}

		if (is_listener)
			continue;

		// Handle regular connections
		std::map<int, Connection *>::iterator cit = _connections.find(fd);
		if (cit != _connections.end())
		{
			if (poll_fds[i].revents & (POLLERR | POLLHUP))
			{
				_parsers.erase(fd);
				delete cit->second;
				_connections.erase(cit);
				continue;
			}
			if (poll_fds[i].revents == 0)
				continue;

			if (poll_fds[i].revents & POLLIN)
			{
				std::map<int, HttpParser>::iterator pit = _parsers.find(fd);
				if (pit == _parsers.end())
					continue;
				std::string buf = cit->second->readBuffer();
				cit->second->setLastUpdate();

				if (buf.empty())
					continue;

				HttpParser::Result r = pit->second.feed(buf.data(), buf.size());

				// After parsing headers, update limit based on location
				if (r.state == HttpParser::READING_BODY || r.state == HttpParser::COMPLETE)
				{
					const HttpRequest &req = pit->second.getRequest();
					std::string host;
					HttpRequest::HeaderMap::const_iterator host_it = req.headers.find("host");
					if (host_it != req.headers.end())
						host = string_parser::splitOnce(host_it->second, ':').first;

					const ServerConfig *server = _router.selectServer(host, cit->second->getListenAddr(), cit->second->getListenPort());
					if (server)
					{
						const LocationConfig *loc = _router.matchLocation(*server, req.path);
						if (loc && loc->max_body_size > 0)
							pit->second.setMaxBodyBytes(loc->max_body_size);
						else if (server->max_body_size > 0)
							pit->second.setMaxBodyBytes(server->max_body_size);

						r = pit->second.validateBodySize();
					}

					// Re-run feed with empty data to trigger the new limit check
					if (r.state == HttpParser::READING_BODY)
						r = pit->second.feed(NULL, 0);
				}

				if (r.state == HttpParser::COMPLETE)
				{
					const HttpRequest &req = pit->second.getRequest();
					std::string listen_addr = cit->second->getListenAddr();
					std::string listen_port = cit->second->getListenPort();
					std::string response = _handleRequest(req, listen_addr, listen_port);
					cit->second->setWriteBuffer(response);
					cit->second->setState(WRITTING);
					pit->second.reset();
				}
				else if (r.state == HttpParser::ERROR)
				{
					HttpResponse err = HttpResponseFactory::buildError(pit->second.getRequest(), nullptr, r.statusCode, r.shouldClose);
					cit->second->setWriteBuffer(err.serialize());
					cit->second->setState(WRITTING);
					if (r.shouldClose)
						cit->second->setCloseAfterWrite(true);
					pit->second.reset();
				}
			}
			else if (poll_fds[i].revents & POLLOUT)
			{
				cit->second->writeData();
				cit->second->setLastUpdate();
			}
			continue;
		}

		// Handle TLS connections
		std::map<int, TlsConnection *>::iterator tcit = _tls_connections.find(fd);
		if (tcit != _tls_connections.end())
		{
			if (poll_fds[i].revents & (POLLERR | POLLHUP))
			{
				_parsers.erase(fd);
				delete tcit->second;
				_tls_connections.erase(tcit);
				continue;
			}
			if (poll_fds[i].revents == 0)
				continue;

			// Handle TLS handshake
			if (tcit->second->getState() == TLS_HANDSHAKE_READING || tcit->second->getState() == TLS_HANDSHAKE_WRITING)
			{
				int handshake_result = tcit->second->performHandshake();
				if (handshake_result < 0)
				{
					// Handshake failed - log SSL error details
					if (tcit->second->getSSL())
					{
						unsigned long err = ERR_get_error();
						if (err != 0)
						{
							char error_buffer[256] = {0};
							ERR_error_string_n(err, error_buffer, sizeof(error_buffer));
							DEBUG_LOG("TLS handshake failed [fd: " << fd << "]: " << error_buffer);
						}
						else
						{
							DEBUG_LOG("TLS handshake failed [fd: " << fd << "]: unknown error");
						}
					}
					_parsers.erase(fd);
					delete tcit->second;
					_tls_connections.erase(tcit);
					continue;
				}
				if (handshake_result == 1)
				{
					// Handshake complete
					tcit->second->setHandshakeComplete(true);
				}
				continue;
			}

			// Regular read after handshake
			if (poll_fds[i].revents & POLLIN)
			{
				std::map<int, HttpParser>::iterator pit = _parsers.find(fd);
				if (pit == _parsers.end())
					continue;

				std::string buf = tcit->second->readBuffer();
				tcit->second->setLastUpdate();

				if (buf.empty())
					continue;

				HttpParser::Result r = pit->second.feed(buf.data(), buf.size());

				// After parsing headers, update limit based on location
				if (r.state == HttpParser::READING_BODY || r.state == HttpParser::COMPLETE)
				{
					const HttpRequest &req = pit->second.getRequest();
					std::string host;
					HttpRequest::HeaderMap::const_iterator host_it = req.headers.find("host");
					if (host_it != req.headers.end())
						host = string_parser::splitOnce(host_it->second, ':').first;

					const ServerConfig *server = _router.selectServer(host, tcit->second->getListenAddr(), tcit->second->getListenPort());
					if (server)
					{
						const LocationConfig *loc = _router.matchLocation(*server, req.path);
						if (loc && loc->max_body_size > 0)
							pit->second.setMaxBodyBytes(loc->max_body_size);
						else if (server->max_body_size > 0)
							pit->second.setMaxBodyBytes(server->max_body_size);

						r = pit->second.validateBodySize();
					}

					if (r.state == HttpParser::READING_BODY)
						r = pit->second.feed(NULL, 0);
				}

				if (r.state == HttpParser::COMPLETE)
				{
					const HttpRequest &req = pit->second.getRequest();
					std::string listen_addr = tcit->second->getListenAddr();
					std::string listen_port = tcit->second->getListenPort();
					std::string response = _handleRequest(req, listen_addr, listen_port);
					tcit->second->setWriteBuffer(response);
					tcit->second->setState(WRITTING);
					pit->second.reset();
				}
				else if (r.state == HttpParser::ERROR)
				{
					HttpResponse err = HttpResponseFactory::buildError(pit->second.getRequest(), nullptr, r.statusCode, r.shouldClose);
					tcit->second->setWriteBuffer(err.serialize());
					tcit->second->setState(WRITTING);
					if (r.shouldClose)
						tcit->second->setCloseAfterWrite(true);
					pit->second.reset();
				}
			}
			else if (poll_fds[i].revents & POLLOUT)
			{
				tcit->second->writeData();
				tcit->second->setLastUpdate();
			}
			continue;
		}
	}
}
