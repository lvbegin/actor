#include <proxyClient.h>
#include <proxyServer.h>

#include <stdexcept>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

proxyClient::proxyClient() = default;
proxyClient::~proxyClient() = default;

#include <iostream>
#include <cstdio>
#include <cstring>


void proxyClient::start(std::string host, uint16_t port) {
	s.reset(new Socket(host, port));
	s->establishConnection();
}


abstractActor::actorReturnCode proxyClient::postSync(int i) {
	const uint32_t type = htonl(postType::Sync);
	const uint32_t command = htonl(i);
	s->writeBytes(&type, sizeof(type));
	s->writeBytes(&command, sizeof(command));

	abstractActor::actorReturnCode rc;
	s->readBytes(&rc, sizeof(rc));
	return rc;
}
void proxyClient::post(int i) {
	const uint32_t type = htonl(postType::Async);
	const uint32_t command = htonl(i);
	s->writeBytes(&type, sizeof(type));
	s->writeBytes(&command, sizeof(command));
}

void proxyClient::restart() {
	const uint32_t type = htonl(postType::Restart);
	s->writeBytes(&type, sizeof(type));
}
