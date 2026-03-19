#ifndef TLS_CONNECTION_HPP
#define TLS_CONNECTION_HPP

#include "io/connection.hpp"
#include <openssl/ssl.h>
#include <string>
#include <sys/socket.h>
#include <ctime>
#include <unistd.h>

#ifndef BUFFER
#define BUFFER 4096
#endif

#ifndef CONNECTION_TIMEOUT
#define CONNECTION_TIMEOUT 120 // 120 seconds (for connection timeout check)
#endif

class TlsConnection
{
private:
	int fd;
	sockaddr_storage client_addr;
	socklen_t addr_len;
	State state;
	std::string read_buffer;
	std::string write_buffer;
	size_t bytes_written;
	time_t last_update;
	bool close_after_write;
	std::string listen_addr;
	std::string listen_port;
	SSL *ssl;
	bool is_tls;
	bool handshake_complete;

public:
	TlsConnection(int fd, sockaddr_storage &client_addr, socklen_t &addr_len,
				  const std::string &listen_addr, const std::string &listen_port,
				  SSL *ssl = nullptr);
	TlsConnection(const TlsConnection &other) = delete;
	~TlsConnection();
	TlsConnection &operator=(const TlsConnection &other) = delete;

	int getFd() const;
	State getState();
	void setState(State new_state);
	void setCloseAfterWrite(bool close);

	void setLastUpdate();
	bool isTimeOut(time_t current_time);

	std::string readBuffer();
	void setWriteBuffer(std::string buffer);
	void writeData();

	const std::string &getReadBuffer() const;
	const std::string &getListenAddr() const;
	const std::string &getListenPort() const;

	bool isTLS() const;
	bool isHandshakeComplete() const;
	void setHandshakeComplete(bool complete);
	SSL *getSSL() const;

	int performHandshake();
};

#endif // TLS_CONNECTION_HPP
