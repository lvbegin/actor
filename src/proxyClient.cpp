#include <proxyClient.h>
#include <proxyServer.h>
#include <clientSocket.h>

proxyClient::proxyClient(Connection connection) : connection(std::move(connection)) {}

proxyClient::~proxyClient() = default;

actorReturnCode proxyClient::postSync(int i) {
	connection.writeInt(postType::Sync);
	connection.writeInt(i);
	return connection.readInt<actorReturnCode>();
}

void proxyClient::post(int i) {
	connection.writeInt(postType::Async);
	connection.writeInt(i);
}

void proxyClient::restart() { connection.writeInt(postType::Restart); }
