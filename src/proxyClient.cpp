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
	s->writeInt(postType::Sync);
	s->writeInt(i);
	return static_cast<abstractActor::actorReturnCode>(s->readInt());
}
void proxyClient::post(int i) {
	s->writeInt(postType::Async);
	s->writeInt(i);
}

void proxyClient::restart() {
	s->writeInt(postType::Restart);
}
