#include <clientSocket.h>
#include <exception.h>

#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <memory.h>

Connection ClientSocket::openHostConnection(std::string host, uint16_t port) {
	sockaddr_in sin = toSockAddr(host, port);
	return openHostConnection(sin);
}

Connection ClientSocket::openHostConnection(struct sockaddr_in &sin) {
	const int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == fd)
		THROW(std::runtime_error, "socket creation failed.");
	if (-1 == connect(fd, reinterpret_cast<sockaddr*>(&sin), sizeof(struct sockaddr_in)))
		THROW(std::runtime_error, "cannot connect.");
	return Connection(fd);
}

struct sockaddr_in ClientSocket::toSockAddr(std::string host, uint16_t port) {
	sockaddr_in sin;
	hostent *he = gethostbyname(host.c_str()); //not thread safe ...
	if (nullptr == he)
		THROW(std::runtime_error, "cannot convert hostname.");
	memcpy(&sin.sin_addr.s_addr, he->h_addr, he->h_length);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	return sin;
}
