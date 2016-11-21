#include <proxyClient.h>
#include <proxyServer.h>
#include <clientSocket.h>

proxyClient::proxyClient(Connection connection) : connection(std::move(connection)) {}

proxyClient::~proxyClient() = default;

returnCode proxyClient::postSync(int i, std::vector<unsigned char> params) {
	connection.writeInt(postType::Sync).writeInt(i);
	return connection.readInt<returnCode>();
}

void proxyClient::post(int i, std::vector<unsigned char> params) { connection.writeInt(postType::Async).writeInt(i); }

void proxyClient::restart() { connection.writeInt(postType::Restart); }
