#include "io/connection_manager.hpp"
#include "io/connection.hpp"
#include "http/http_parser.hpp"
#include "http/http_response_factory.hpp"
#include "http/http_request.hpp"
#include "config/server/server_config.hpp"
#include "io/utils.hpp"
#include "router.hpp"
#include "logger.hpp"
#include "string/string_parser.hpp"
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
#include "webserv.hpp"

ConnectionManager::~ConnectionManager()
{
	for (auto &pair : _connections)
	{
		if (pair.second) // Connection objects own their fd; destructor closes it
			delete pair.second;
	}
	_connections.clear();
	for (auto &it : _listener_sockets)
	{
		delete (it); // Socket destructor closes the fd
	}
	DEBUG_LOG("All connections cleaned up");
}

bool ConnectionManager::hasActiveConnections() const
{
	return !_listener_sockets.empty();
}

void ConnectionManager::addListener(Socket *socket, const std::string &addr, const std::string &port)
{
	_listener_sockets.push_back(socket);
	_listener_fd_to_addr_port[socket->getFd()] = std::make_pair(addr, port);
}


void ConnectionManager::pollConnections()
{
	std::vector<pollfd> poll_fds;
	time_t current_time = time(NULL);

	// Clean up connections marked for closing or timeout
	for (std::map<int, Connection *>::iterator it = _connections.begin(); it != _connections.end();)
	{
		if (it->second->getState() == CLOSING || it->second->isTimeOut(current_time))
		{
			DEBUG_LOG("Connection closed [fd: " << it->first << "]");
			_parsers.erase(it->first);
			delete it->second; // Connection destructor closes the fd
			it = _connections.erase(it);
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

	// Build poll array for client connections only
	for (std::map<int, Connection *>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		short events = 0;
		if (it->second->getState() == READING)
			events = POLLIN;
		else if (it->second->getState() == WRITTING)
			events = POLLOUT;
		poll_fds.push_back((pollfd){it->first, events, 0});
	}

	if (poll_fds.empty())
		return;

	int n = poll(poll_fds.data(), poll_fds.size(), POLL_TIMEOUT);
	if (n < 0)
	{
		return;
	}

	for (size_t i = 0; i < poll_fds.size(); i++)
	{
		int fd = poll_fds[i].fd;

		// Check if it's a listener socket
		bool is_listener = false;
		for (size_t j = 0; j < _listener_sockets.size(); ++j)
		{
			if (_listener_sockets[j]->getFd() == fd)
			{
				is_listener = true;
				if (poll_fds[i].revents & POLLIN)
				{
					std::map<int, std::pair<std::string, std::string> >::iterator lit = _listener_fd_to_addr_port.find(fd);
					if (lit != _listener_fd_to_addr_port.end())
					{
						sockaddr_storage client_addr;
						socklen_t addr_len = sizeof(client_addr);
						int client_fd = _listener_sockets[j]->accept_connection(client_addr, addr_len);
						if (client_fd > 0)
						{
							try
							{
								makeNonBlocking(client_fd);
								Connection *conn = new Connection(client_fd, client_addr, addr_len, lit->second.first, lit->second.second);
								_connections[client_fd] = conn;
								_parsers[client_fd] = HttpParser();
								const ServerConfig *server = _router.getServerForListen(lit->second.first, lit->second.second);
								if (server && server->max_body_size > 0)
									_parsers[client_fd].setMaxBodyBytes(server->max_body_size);
								DEBUG_LOG("New connection [fd: " << client_fd << "] (default server: " << (server ? server->server_name : "none") << ")");
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

		if (poll_fds[i].revents & (POLLERR | POLLHUP))
		{
			std::map<int, Connection *>::iterator cit = _connections.find(fd);
			if (cit != _connections.end())
			{
				_parsers.erase(fd);
				delete cit->second; // Connection destructor closes the fd
				_connections.erase(cit);
			}
			continue;
		}
		if (poll_fds[i].revents == 0)
			continue;

		if (poll_fds[i].revents & POLLIN)
		{
			std::map<int, Connection *>::iterator cit = _connections.find(fd);
			std::map<int, HttpParser>::iterator pit = _parsers.find(fd);
			if (cit == _connections.end() || pit == _parsers.end())
				continue;
			std::string buf = cit->second->readBuffer();
			cit->second->setLastUpdate();
			
			if (buf.empty())
				continue;

			HttpParser::Result r = pit->second.feed(buf.data(), buf.size());
			
			// After parsing headers (transition to READING_BODY or COMPLETE), update limit based on location
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
				
				// Re-run feed with empty data to trigger the new limit check if we just switched state
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
				HttpResponse err = HttpResponseFactory::buildError(pit->second.getRequest(), r.statusCode, r.shouldClose);
				cit->second->setWriteBuffer(err.serialize());
				cit->second->setState(WRITTING);
				if (r.shouldClose)
					cit->second->setCloseAfterWrite(true);
				pit->second.reset();
			}
		}
		else if (poll_fds[i].revents & POLLOUT)
		{
			std::map<int, Connection *>::iterator cit = _connections.find(fd);
			if (cit != _connections.end())
			{
				cit->second->writeData();
				cit->second->setLastUpdate();
			}
		}
	}
}
