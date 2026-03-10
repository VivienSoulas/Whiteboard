#include "io/utils.hpp"
#include "logger.hpp"

void makeNonBlocking(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0); // gets current flag
	if (flags < 0)
		throw std::runtime_error("fcntl(F_GETFL) failed\n");

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) // add flag NONBLOCK to the socket
		throw std::runtime_error("fcntl() could not set flags\n");
}
