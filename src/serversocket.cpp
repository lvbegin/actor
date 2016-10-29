#include <stdexcept>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <serverSocket.h>

#include <unistd.h>

static int acceptOneConnection(int sockfd);
static int serverSocket(uint16_t port);

ServerSocket::ServerSocket(uint16_t port) : acceptFd(-1), port(port) { }

ServerSocket::ServerSocket(ServerSocket&& s) { *this = std::move(s); }

ServerSocket &ServerSocket::operator=(ServerSocket&& s) {
	this->acceptFd = s.acceptFd;
	this->port = s.port;
	s.acceptFd = -1;
	return *this;
}

ServerSocket::~ServerSocket() {
	if (-1 != acceptFd)
		close(acceptFd);
}

Connection ServerSocket::getNextConnection(void) { return Connection(acceptHost()); }

Connection ServerSocket::getConnection(int port) { return ServerSocket(port).getNextConnection(); }

int ServerSocket::acceptHost(void) {
	acceptFd = (-1 == acceptFd) ? serverSocket(port) : acceptFd;
	return acceptOneConnection(acceptFd);
}

static int acceptOneConnection(int sockfd) {
	struct sockaddr_in client_addr { };
	socklen_t client_len = sizeof(client_addr);
	const int newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
	if (-1 == newsockfd)
		throw std::runtime_error("ServerSocket: accept failed");
	close(sockfd);
	return newsockfd;
}

static int serverSocket(uint16_t port) {
	struct sockaddr_in serv_addr { };
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
		throw std::runtime_error("ServerSocket: cannot create socket");
	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		throw std::runtime_error("ServerSocket: setsockopt(SO_REUSEADDR) failed");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, reinterpret_cast<struct sockaddr *> (&serv_addr), sizeof(serv_addr)) < 0)
		throw std::runtime_error("ServerSocket: cannot bind socker");
	if (-1 == listen(sockfd,5))
		throw std::runtime_error("ServerSocket: cannot listen");
	return sockfd;
}
