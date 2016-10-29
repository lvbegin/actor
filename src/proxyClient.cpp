#include <proxyClient.h>
#include <proxyServer.h>
#include <clientSocket.h>

proxyClient::proxyClient(std::string host, uint16_t port) : connection(ClientSocket::connectHost(host, port)) { }
proxyClient::~proxyClient() = default;

actorReturnCode proxyClient::postSync(int i) {
	connection.writeInt(postType::Sync);
	connection.writeInt(i);
	return static_cast<actorReturnCode>(connection.readInt());
}

void proxyClient::post(int i) {
	connection.writeInt(postType::Async);
	connection.writeInt(i);
}

void proxyClient::restart() { connection.writeInt(postType::Restart); }
