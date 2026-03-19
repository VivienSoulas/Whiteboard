#include "io/tls_connection.hpp"
#include "io/connection.hpp"
#include <openssl/ssl.h>
#include <unistd.h>

TlsConnection::TlsConnection(int fd, sockaddr_storage &client_addr, socklen_t &addr_len,
							 const std::string &listen_addr, const std::string &listen_port,
							 SSL *ssl)
	: fd(fd), client_addr(client_addr), addr_len(addr_len), state(ssl ? TLS_HANDSHAKE_READING : READING),
	  read_buffer(""), write_buffer(""), bytes_written(0), last_update(time(nullptr)),
	  close_after_write(false), listen_addr(listen_addr), listen_port(listen_port),
	  ssl(ssl), is_tls(ssl != nullptr), handshake_complete(false)
{
}

TlsConnection::~TlsConnection()
{
	if (ssl)
	{
		SSL_shutdown(ssl);
		SSL_free(ssl);
	}
	if (fd != -1)
		close(fd);
}

int TlsConnection::getFd() const
{
	return fd;
}

State TlsConnection::getState()
{
	return state;
}

void TlsConnection::setState(State new_state)
{
	state = new_state;
}

void TlsConnection::setCloseAfterWrite(bool close)
{
	close_after_write = close;
}

void TlsConnection::setLastUpdate()
{
	last_update = time(nullptr);
}

bool TlsConnection::isTimeOut(time_t current_time)
{
	return (current_time - last_update) > CONNECTION_TIMEOUT;
}

std::string TlsConnection::readBuffer()
{
	char buffer[BUFFER] = {0};
	int bytes_read = 0;

	if (is_tls && ssl)
	{
		bytes_read = SSL_read(ssl, buffer, BUFFER);
		if (bytes_read < 0)
		{
			int ssl_error = SSL_get_error(ssl, bytes_read);
			if (ssl_error == SSL_ERROR_WANT_READ)
				return "";
			else if (ssl_error == SSL_ERROR_WANT_WRITE)
			{
				state = WRITTING;
				return "";
			}
		}
		else if (bytes_read == 0)
		{
			state = CLOSING;
		}
	}
	else
	{
		bytes_read = read(fd, buffer, BUFFER);
		if (bytes_read < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return "";
		}
		else if (bytes_read == 0)
		{
			state = CLOSING;
		}
	}

	if (bytes_read > 0)
	{
		read_buffer.append(buffer, bytes_read);
		setLastUpdate();
	}

	return read_buffer;
}

void TlsConnection::setWriteBuffer(std::string buffer)
{
	write_buffer = buffer;
	bytes_written = 0;
}

void TlsConnection::writeData()
{
	if (write_buffer.empty())
	{
		if (close_after_write)
			state = CLOSING;
		else
			state = READING;
		bytes_written = 0;
		return;
	}

	int bytes_to_write = write_buffer.length() - bytes_written;
	int written = 0;

	if (is_tls && ssl)
	{
		written = SSL_write(ssl, write_buffer.c_str() + bytes_written, bytes_to_write);
		if (written < 0)
		{
			int ssl_error = SSL_get_error(ssl, written);
			if (ssl_error == SSL_ERROR_WANT_WRITE)
				return;
			else if (ssl_error == SSL_ERROR_WANT_READ)
			{
				state = READING;
				return;
			}
		}
	}
	else
	{
		written = write(fd, write_buffer.c_str() + bytes_written, bytes_to_write);
		if (written < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
		}
	}

	if (written > 0)
	{
		bytes_written += written;
		setLastUpdate();

		if (bytes_written >= write_buffer.length())
		{
			if (close_after_write)
				state = CLOSING;
			else
				state = READING;
			read_buffer.clear();
			write_buffer.clear();
			bytes_written = 0;
		}
	}
}

const std::string &TlsConnection::getReadBuffer() const
{
	return read_buffer;
}

const std::string &TlsConnection::getListenAddr() const
{
	return listen_addr;
}

const std::string &TlsConnection::getListenPort() const
{
	return listen_port;
}

bool TlsConnection::isTLS() const
{
	return is_tls;
}

bool TlsConnection::isHandshakeComplete() const
{
	return handshake_complete;
}

void TlsConnection::setHandshakeComplete(bool complete)
{
	handshake_complete = complete;
}

SSL *TlsConnection::getSSL() const
{
	return ssl;
}

int TlsConnection::performHandshake()
{
	if (!is_tls || !ssl)
		return 1;

	int ret = SSL_accept(ssl);
	if (ret == 1)
	{
		handshake_complete = true;
		state = READING;
		return 1;
	}

	int ssl_error = SSL_get_error(ssl, ret);
	if (ssl_error == SSL_ERROR_WANT_READ)
	{
		state = TLS_HANDSHAKE_READING;
		return 0;
	}
	else if (ssl_error == SSL_ERROR_WANT_WRITE)
	{
		state = TLS_HANDSHAKE_WRITING;
		return 0;
	}

	return -1;
}
