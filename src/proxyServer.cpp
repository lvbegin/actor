#include <proxyServer.h>
#include <serverSocket.h>

#include <arpa/inet.h>

proxyServer::proxyServer(std::shared_ptr<AbstractActor> actor, Connection connection) :
	t([actor, connection {std::move(connection)}]() mutable { startThread(std::move(actor), std::move(connection)); }) { }


proxyServer::proxyServer(proxyServer &&p) {  *this = std::move(p); }
proxyServer &proxyServer::operator=(proxyServer &&p) {
	std::swap(t, p.t);
	return *this;
}

proxyServer::~proxyServer() {
	if (t.joinable())
		t.join();
};

void proxyServer::startThread(std::shared_ptr<AbstractActor> actor, Connection connection) {
	while (true) {
		uint32_t command;
		switch (connection.readInt<postType>()) {
			case postType::Async:
				command = connection.readInt<uint32_t>();
				actor->post(command);
				break;
			case postType::Sync:
				command = connection.readInt<uint32_t>();
				connection.writeInt(actor->postSync(command));
				break;
			case postType::Restart:
				actor->restart();
				continue;
			default:
				continue;
		}
		if (AbstractActor::COMMAND_SHUTDOWN == command)
			return;
	}
}
