#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <sys/socket.h> // for socket, bind, listend and accept
#include <netdb.h>		// for getaddrinfo
#include <stdexcept>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <unistd.h>

#define BACKLOG 1024

class Socket
{
private:
	int fd;

public:
	Socket(const std::string port, const std::string ip);
	Socket(const Socket &other) = delete;
	~Socket();
	Socket &operator=(const Socket &other);

	int getFd() const;

	void getAddrInfo(const std::string port, const std::string ip, struct addrinfo *&result);
	void createSocket(struct addrinfo *result);
	void bindingSocket(struct addrinfo *result);
	void startListening();

	int accept_connection(sockaddr_storage &client_addr, socklen_t &addr_len);
};

#endif
