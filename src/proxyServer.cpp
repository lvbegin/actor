#include <proxyServer.h>

#include <stdexcept>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <iostream>


proxyServer::proxyServer() = default;
proxyServer::~proxyServer() { t.join(); /*should not do it if thread not created */ };


static int serverSocket(uint16_t port);
static int getConnection(int sockFd);


void proxyServer::start(abstractActor &actor, uint16_t port) {
	t = std::thread([&actor, port]() {  startThread(actor, port); });
}

void proxyServer::startThread(abstractActor &actor, uint16_t port) {
	const int sock = serverSocket(port);
	const int connection = getConnection(sock);
	close(sock);
	while (true) {
		int32_t type;
		int32_t command;
		read(connection, &type, sizeof(type));
		switch (ntohl(type))
		{
			case postType::Async:
				read(connection, &command, sizeof(command));
				actor.post(ntohl(command));
				break;
			case postType::Sync: {
				read(connection, &command, sizeof(command));
				const abstractActor::actorReturnCode rc = actor.postSync(ntohl(command));
				write(connection, &rc, sizeof(rc)); //check rc: what happen if write fails?
				break;
			}
			case postType::Restart:
				actor.restart(); //check rc ?
				break;

			default:
				continue;
		}
		if (abstractActor::COMMAND_SHUTDOWN == command) {
			close(connection);
			return;
		}
	}
}

static int getConnection(int sockfd) {
	struct sockaddr_in client_addr {};
	socklen_t client_len = sizeof(client_addr);
	const int newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
	if (-1 == newsockfd)
		throw std::runtime_error("proxyServer: accept failed");
	close(sockfd);
	return newsockfd;
}

static int serverSocket(uint16_t port) {
	struct sockaddr_in serv_addr;
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
