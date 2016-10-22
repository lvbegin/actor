#include <socket.h>
#include <stdexcept>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <unistd.h>

static int serverSocket(uint16_t port);
static int getConnection(int sockFd);


Socket::Socket(uint16_t port) : fd(-1), type(socketType::Server), port(port) { }

Socket::Socket(std::string host, uint16_t port) : fd(-1), type(socketType::Client), port(port), host(host) { }

Socket::~Socket() {
	if (-1 != fd)
		close(fd);
}
void Socket::establishConnection(void) {
	switch (type) {
	case socketType::Client:
		connectHost();
		break;
	case socketType::Server:
		accept();
		break;
	default:
		throw std::runtime_error("unexpected case.");
	}
}

void Socket::connectHost(void) {
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == fd)
		throw std::runtime_error("proxyClient: socket creation failed");
	sockaddr_in sin;
	hostent *he = gethostbyname(host.c_str());
	if (nullptr == he)
		throw std::runtime_error("proxyClient: cannot convert hostname");
	memcpy(&sin.sin_addr.s_addr, he->h_addr, he->h_length);
	printf(" %d %d %d %d\n", ((char *)&sin.sin_addr.s_addr)[0], ((char *)&sin.sin_addr.s_addr)[1], ((char *)&sin.sin_addr.s_addr)[2], ((char *)&sin.sin_addr.s_addr)[3]);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if (-1 == connect(fd, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)))
		throw std::runtime_error("proxyClient: cannot connect");
}

void Socket::accept(void) {
	const int sock = serverSocket(port);
	fd = getConnection(sock);
	close(sock);
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
		const ssize_t nbWritten = write(fd, ptr + nbTotalWritten, count - nbTotalWritten);
		if (-1 == nbWritten && !(EINTR == errno))
			throw std::runtime_error("send bytes to proxy server failed");
		nbTotalWritten += nbWritten;
	}
}

void Socket::readBytes(void *buffer, size_t count) {
	char *ptr = static_cast<char *>(buffer);
	for (size_t nbTotalRead = 0; nbTotalRead < count; ) {
		const ssize_t nbRead = read(fd, ptr + nbTotalRead, count - nbTotalRead);
		if (0 >= nbRead)
			throw std::runtime_error("send bytes to proxy server failed");
		nbTotalRead += nbRead;
	}
}

static int getConnection(int sockfd) {
	struct sockaddr_in client_addr { };
	socklen_t client_len = sizeof(client_addr);
	const int newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
	if (-1 == newsockfd)
		throw std::runtime_error("proxyServer: accept failed");
	close(sockfd);
	return newsockfd;
}

static int serverSocket(uint16_t port) {
	struct sockaddr_in serv_addr { };
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
		throw std::runtime_error("proxyServer: cannot create socket");
	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		throw std::runtime_error("proxyServer: setsockopt(SO_REUSEADDR) failed");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, reinterpret_cast<struct sockaddr *> (&serv_addr), sizeof(serv_addr)) < 0)
		throw std::runtime_error("proxyServer: cannot bind socker");
	if (-1 == listen(sockfd,5))
		throw std::runtime_error("proxyServer: cannot listen");
	return sockfd;
}
