#include <socket.h>
#include <stdexcept>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <unistd.h>

static int getConnection(int sockfd);
static int serverSocket(uint16_t port);

Socket::Socket(uint16_t port) : acceptFd(-1), connectionFd(-1), type(socketType::Server), port(port) { }

Socket::Socket(std::string host, uint16_t port) : acceptFd(-1), connectionFd(-1), type(socketType::Client), port(port), host(host) { }

Socket::Socket(uint16_t port, int fd) : acceptFd(-1), connectionFd(fd), type(socketType::Server), port(port) { }

Socket::Socket(Socket&& s) { *this = std::move(s); }

Socket &Socket::operator=(Socket&& s) {
	this->acceptFd = s.acceptFd;
	this->connectionFd = s.connectionFd;
	this->type = s.type;
	this->port = s.port;
	this->host = s.host;
	s.acceptFd = -1;
	s.connectionFd = -1;
	return *this;
}


Socket::~Socket() {
	if (-1 != acceptFd)
		close(acceptFd);
	if (-1 != connectionFd)
		close(connectionFd);
}

void Socket::establishConnection(void) {
	switch (type) {
	case socketType::Client:
		connectHost();
		break;
	case socketType::Server:
		acceptHost();
		close(acceptFd);
		acceptFd = -1;
		break;
	default:
		throw std::runtime_error("Socket: unexpected case.");
	}
}

#include <iostream>
Socket Socket::getNextConnection(void) {
	if (socketType::Server != type)
		throw std::runtime_error("Socket: implemented only for server socket");
	acceptHost();
	Socket s(port, connectionFd);
	connectionFd = -1;
	return s;
}

void Socket::connectHost(void) {
	connectionFd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == connectionFd)
		throw std::runtime_error("Socket: socket creation failed");
	sockaddr_in sin;
	hostent *he = gethostbyname(host.c_str());
	if (nullptr == he)
		throw std::runtime_error("Socket: cannot convert hostname");
	memcpy(&sin.sin_addr.s_addr, he->h_addr, he->h_length);
	printf(" %d %d %d %d\n", ((char *)&sin.sin_addr.s_addr)[0], ((char *)&sin.sin_addr.s_addr)[1], ((char *)&sin.sin_addr.s_addr)[2], ((char *)&sin.sin_addr.s_addr)[3]);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if (-1 == connect(connectionFd, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)))
		throw std::runtime_error("Socket: cannot connect");
}

void Socket::acceptHost(void) {
	acceptFd = (-1 == acceptFd) ? serverSocket(port) : acceptFd;
	connectionFd = getConnection(acceptFd);
}

void Socket::writeInt(uint32_t hostValue) {
	const uint32_t sentValue = htonl(hostValue);
	writeBytes(&sentValue, sizeof(sentValue));
}

uint32_t Socket::readInt(void) {
	uint32_t value;
	readBytes(&value, sizeof(value));
	return ntohl(value);
}

void Socket::writeBytes(const void *buffer, size_t count) {
	const char *ptr = static_cast<const char *>(buffer);
	for (size_t nbTotalWritten = 0; nbTotalWritten < count; ) {
		const ssize_t nbWritten = write(connectionFd, ptr + nbTotalWritten, count - nbTotalWritten);
		if (-1 == nbWritten && !(EINTR == errno))
			throw std::runtime_error("Socket: send bytes failed");
		nbTotalWritten += nbWritten;
	}
}

void Socket::readBytes(void *buffer, size_t count) {
	char *ptr = static_cast<char *>(buffer);
	for (size_t nbTotalRead = 0; nbTotalRead < count; ) {
		const ssize_t nbRead = read(connectionFd, ptr + nbTotalRead, count - nbTotalRead);
		if (0 >= nbRead)
			throw std::runtime_error("Socket: read bytes failed");
		nbTotalRead += nbRead;
	}
}

static int getConnection(int sockfd) {
	struct sockaddr_in client_addr { };
	socklen_t client_len = sizeof(client_addr);
	const int newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
	if (-1 == newsockfd)
		throw std::runtime_error("Socket: accept failed");
	close(sockfd);
	return newsockfd;
}

static int serverSocket(uint16_t port) {
	struct sockaddr_in serv_addr { };
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
		throw std::runtime_error("Socket: cannot create socket");
	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		throw std::runtime_error("Socket: setsockopt(SO_REUSEADDR) failed");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, reinterpret_cast<struct sockaddr *> (&serv_addr), sizeof(serv_addr)) < 0)
		throw std::runtime_error("Socket: cannot bind socker");
	if (-1 == listen(sockfd,5))
		throw std::runtime_error("Socket: cannot listen");
	return sockfd;
}
