#ifndef TLS_SOCKET_HPP
#define TLS_SOCKET_HPP

#include <openssl/ssl.h>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <unistd.h>

#define BACKLOG 1024

class TlsSocket
{
private:
	int fd;
	SSL_CTX *ssl_ctx;
	bool is_tls;

public:
	TlsSocket(const std::string port, const std::string ip, SSL_CTX *ssl_ctx = nullptr);
	TlsSocket(const TlsSocket &other) = delete;
	~TlsSocket();
	TlsSocket &operator=(const TlsSocket &other);

	int getFd() const;
	bool isTLS() const;
	SSL_CTX *getSslCtx() const;

	void getAddrInfo(const std::string port, const std::string ip, struct addrinfo *&result);
	void createSocket(struct addrinfo *result);
	void bindingSocket(struct addrinfo *result);
	void startListening();

	SSL *accept_connection_tls(sockaddr_storage &client_addr, socklen_t &addr_len);
	int accept_connection(sockaddr_storage &client_addr, socklen_t &addr_len);
};

#endif // TLS_SOCKET_HPP
