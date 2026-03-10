#include "io/socket.hpp"
#include "io/utils.hpp"
#include "logger.hpp"

Socket::Socket(const std::string port, const std::string ip)
{
	struct addrinfo *result = nullptr;

	try
	{
		getAddrInfo(port, ip, result);
		createSocket(result);
		makeNonBlocking(fd);
		bindingSocket(result);
		startListening();
		freeaddrinfo(result);
		result = nullptr;
	}
	catch (...)
	{
		if (result != nullptr)
			freeaddrinfo(result);
		if (fd > 0)
			close(fd);
		throw;
	}
}

Socket::~Socket()
{
	if (fd > 0)
		close(fd);
}

Socket &Socket::operator=(const Socket &other)
{
	if (this != &other)
		fd = other.fd;
	return (*this);
}

int Socket::getFd() const
{
	return (fd);
}

void Socket::getAddrInfo(const std::string port, const std::string ip, struct addrinfo *&result)
{
	struct addrinfo hints = {};

	hints.ai_family = AF_UNSPEC; // workds for IPv4 && IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int res = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
	if (res < 0)
		throw std::runtime_error("getaddrinfo() failed");
}

// connected with TCP (safer protocol than UDP)
void Socket::createSocket(struct addrinfo *result)
{
	fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (fd < 0)
		throw std::runtime_error("socket() failed");

	// Allow immediate port reuse (avoid having to wait 60s before reconnecting to same port)
	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
}

// bind socket with Ip address
void Socket::bindingSocket(struct addrinfo *result)
{
	int binding = bind(fd, result->ai_addr, result->ai_addrlen);
	if (binding < 0)
	{
		std::string msg = "bind() failed";
		if (errno != 0)
		{
			msg += ": ";
			msg += strerror(errno);
		}
		throw std::runtime_error(msg);
	}
}

void Socket::startListening()
{
	int listening;

	listening = listen(fd, BACKLOG);
	if (listening < 0)
		throw std::runtime_error("listen() failed");
}

int Socket::accept_connection(sockaddr_storage &client_addr, socklen_t &addr_len)
{
	int connection_fd;

	connection_fd = accept(fd, (struct sockaddr *)&client_addr, &addr_len);
	if (connection_fd < 0)
	{
		DEBUG_LOG("Failed to accept connection: " << strerror(errno));
	}
	return (connection_fd);
}
