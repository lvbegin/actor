#include <proxyClient.h>
#include <proxyServer.h>

#include <stdexcept>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

static void writeNbBytes(int fd, const void *buffer, size_t count);

proxyClient::proxyClient() : fd(-1) { }
proxyClient::~proxyClient() {
	if (-1 != fd)
		close(fd);
}

#include <iostream>
#include <cstdio>
#include <cstring>


void proxyClient::start(std::string host, uint16_t port) {
	if (-1 != fd)
		throw std::runtime_error("proxyClient: already started");
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


abstractActor::actorReturnCode proxyClient::postSync(int i) {
	const uint32_t type = htonl(postType::Sync);
	const uint32_t command = htonl(i);
	writeNbBytes(fd, &type, sizeof(type));
	writeNbBytes(fd, &command, sizeof(command));

	abstractActor::actorReturnCode rc;
	if (-1 == read(fd, &rc, sizeof(rc)))
		throw std::runtime_error("proxyClient: cannot read return code");
	return rc;
}
void proxyClient::post(int i) {
	const uint32_t type = htonl(postType::Async);
	const uint32_t command = htonl(i);
	writeNbBytes(fd, &type, sizeof(type));
	writeNbBytes(fd, &command, sizeof(command));
}

void proxyClient::restart() {
	const uint32_t type = htonl(postType::Restart);
	writeNbBytes(fd, &type, sizeof(type));
}

static void writeNbBytes(int fd, const void *buffer, size_t count)
{
	const char *ptr = static_cast<const char *>(buffer);
	for (size_t nbTotalWritten = 0; nbTotalWritten < count; ) {
		const ssize_t nbWritten = write(fd, ptr + nbTotalWritten, count - nbTotalWritten);
		if (-1 == nbWritten && !(EINTR == errno))
			throw std::runtime_error("send bytes to proxy server failed");
		nbTotalWritten += nbWritten;
	}
}
