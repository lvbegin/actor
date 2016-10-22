#include <proxyClient.h>
#include <proxyServer.h>

proxyClient::proxyClient() = default;
proxyClient::~proxyClient() = default;

void proxyClient::start(std::string host, uint16_t port) {
	s.reset(new Socket(host, port));
	s->establishConnection();
}

actorReturnCode proxyClient::postSync(int i) {
	s->writeInt(postType::Sync);
	s->writeInt(i);
	return static_cast<actorReturnCode>(s->readInt());
}
void proxyClient::post(int i) {
	s->writeInt(postType::Async);
	s->writeInt(i);
}

void proxyClient::restart() { s->writeInt(postType::Restart); }
