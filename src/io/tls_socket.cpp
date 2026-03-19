#include "io/tls_socket.hpp"
#include "io/socket.hpp"

TlsSocket::TlsSocket(const std::string port, const std::string ip, SSL_CTX *ssl_ctx)
	: fd(-1), ssl_ctx(ssl_ctx), is_tls(ssl_ctx != nullptr)
{
	struct addrinfo *result = nullptr;

	try
	{
		getAddrInfo(port, ip, result);
		createSocket(result);
		bindingSocket(result);
		startListening();

		if (result)
			freeaddrinfo(result);
	}
	catch (const std::exception &e)
	{
		if (result)
			freeaddrinfo(result);
		throw;
	}
}

TlsSocket::~TlsSocket()
{
	if (fd != -1)
		close(fd);
}

TlsSocket &TlsSocket::operator=(const TlsSocket &other)
{
	if (this != &other)
	{
		if (fd != -1)
			close(fd);
		fd = other.fd;
		ssl_ctx = other.ssl_ctx;
		is_tls = other.is_tls;
	}
	return *this;
}

int TlsSocket::getFd() const
{
	return fd;
}

bool TlsSocket::isTLS() const
{
	return is_tls;
}

SSL_CTX *TlsSocket::getSslCtx() const
{
	return ssl_ctx;
}

void TlsSocket::getAddrInfo(const std::string port, const std::string ip, struct addrinfo *&result)
{
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status = getaddrinfo(ip.empty() ? nullptr : ip.c_str(), port.c_str(), &hints, &result);
	if (status != 0)
	{
		throw std::runtime_error(std::string("getaddrinfo error: ") + gai_strerror(status));
	}
}

void TlsSocket::createSocket(struct addrinfo *result)
{
	for (struct addrinfo *p = result; p != nullptr; p = p->ai_next)
	{
		fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (fd == -1)
			continue;

		int opt = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			close(fd);
			fd = -1;
			continue;
		}

		break;
	}

	if (fd == -1)
		throw std::runtime_error("Could not create socket");
}

void TlsSocket::bindingSocket(struct addrinfo *result)
{
	for (struct addrinfo *p = result; p != nullptr; p = p->ai_next)
	{
		if (bind(fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(fd);
			fd = -1;
			continue;
		}
		break;
	}

	if (fd == -1)
		throw std::runtime_error("Could not bind socket");
}

void TlsSocket::startListening()
{
	if (listen(fd, BACKLOG) == -1)
		throw std::runtime_error("Listen failed");
}

SSL *TlsSocket::accept_connection_tls(sockaddr_storage &client_addr, socklen_t &addr_len)
{
	if (!is_tls || !ssl_ctx)
		return nullptr;

	int client_fd = ::accept(fd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_fd == -1)
		return nullptr;

	SSL *ssl = SSL_new(ssl_ctx);
	if (!ssl)
	{
		close(client_fd);
		return nullptr;
	}

	if (!SSL_set_fd(ssl, client_fd))
	{
		SSL_free(ssl);
		close(client_fd);
		return nullptr;
	}

	// Set non-blocking mode
	SSL_set_accept_state(ssl);

	return ssl;
}

int TlsSocket::accept_connection(sockaddr_storage &client_addr, socklen_t &addr_len)
{
	int client_fd = ::accept(fd, (struct sockaddr *)&client_addr, &addr_len);
	return client_fd;
}
