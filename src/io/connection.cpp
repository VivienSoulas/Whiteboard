#include "io/connection.hpp"
#include "logger.hpp"
#include <cstring>

#define MAX_WRITE_BUFFER (10 * 1024 * 1024)  // 10MB

Connection::Connection(int fd, sockaddr_storage &client_addr, socklen_t &addr_len,
					   const std::string &listen_addr, const std::string &listen_port)
	: fd(fd), client_addr(client_addr), addr_len(addr_len), state(READING), bytes_written(0),
	  last_update(time(NULL)), close_after_write(false), listen_addr(listen_addr), listen_port(listen_port)
{
}

Connection::~Connection()
{
	if (fd > 0)
		close(fd);
}

int Connection::getFd() const
{
	return (fd);
}

State Connection::getState()
{
	return (state);
}

void Connection::setState(State new_state)
{
	state = new_state;
}

void Connection::setCloseAfterWrite(bool close)
{
	close_after_write = close;
}

void Connection::setLastUpdate()
{
	last_update = time(NULL);
}

bool Connection::isTimeOut(time_t current_time)
{
	if ((current_time - last_update) >= CONNECTION_TIMEOUT)
		return (true);
	return (false);
}

std::string Connection::readBuffer()
{
	char temp[BUFFER] = {};

	ssize_t bytes_received = recv(fd, temp, BUFFER, 0);
	if (bytes_received > 0)
	{
		DEBUG_LOG("Connection [fd: " << fd << "] read " << bytes_received << " bytes");
		if (bytes_received > 0)
		{
			read_buffer.reserve(read_buffer.size() + (BUFFER * 2));
			read_buffer.append(temp, bytes_received);
		}
		return std::string(temp, bytes_received);
	}
	else
	{
		if (bytes_received < 0)
			DEBUG_LOG("Connection [fd: " << fd << "] error: " << strerror(errno));
		else
			DEBUG_LOG("Connection [fd: " << fd << "] closed by peer");
		setState(CLOSING); 
		return "";
	}
}

void Connection::setWriteBuffer(std::string buffer)
{
	if (buffer.size() > MAX_WRITE_BUFFER)
	{
		// Implement backpressure instead of immediately closing
		// Pause reading to prevent further data accumulation
		DEBUG_LOG("Write buffer approaching limit, implementing backpressure");
		state = WRITTING;
		write_buffer = buffer;
		bytes_written = 0;
		return;
	}
	if (buffer.empty())
		setState(READING);

	write_buffer = buffer;
	bytes_written = 0;
}

// send the data to write starting from last data sent till data ends
void Connection::writeData()
{
	ssize_t sent = send(fd, write_buffer.c_str() + bytes_written, write_buffer.size() - bytes_written, 0);
	if (sent > 0)
	{
		bytes_written += sent;
		DEBUG_LOG("Connection [fd: " << fd << "] sent " << sent << " bytes (total " << bytes_written << "/" << write_buffer.size() << ")");
		if (bytes_written >= write_buffer.size()) // resets, all data have been transmitted
		{
			read_buffer.clear();
			write_buffer.clear();
			bytes_written = 0;
			if (close_after_write)
				setState(CLOSING);
			else
				setState(READING); // default for http 1.1
		}
	}
	else {
		DEBUG_LOG("Connection [fd: " << fd << "] send failed (errno: " << strerror(errno) << ")");
		setState(CLOSING);
	}
}

const std::string &Connection::getReadBuffer() const
{
	return (read_buffer);
}

const std::string &Connection::getListenAddr() const { return listen_addr; }
const std::string &Connection::getListenPort() const { return listen_port; }
