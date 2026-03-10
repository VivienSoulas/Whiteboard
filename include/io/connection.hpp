#ifndef CONNECTION_HPP
#define CONNECTION_HPP

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

enum State
{
	READING,
	WRITTING,
	CLOSING
};

class Connection
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

public:
	Connection(int fd, sockaddr_storage &client_addr, socklen_t &addr_len,
			   const std::string &listen_addr, const std::string &listen_port);
	Connection(const Connection &other);
	~Connection();
	Connection &operator=(const Connection &other);

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
};

#endif
