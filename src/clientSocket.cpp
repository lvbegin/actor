#include <clientSocket.h>

#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <memory.h>

Connection ClientSocket::openHostConnection(std::string host, uint16_t port) {
	const int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == fd)
		throw std::runtime_error("ClientSocket: socket creation failed");
	sockaddr_in sin;
	hostent *he = gethostbyname(host.c_str()); //not thread safe ...
	if (nullptr == he)
		throw std::runtime_error("ClientSocket: cannot convert hostname");
	memcpy(&sin.sin_addr.s_addr, he->h_addr, he->h_length);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if (-1 == connect(fd, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)))
		throw std::runtime_error("ClientSocket: cannot connect");
	return Connection(fd);
}
