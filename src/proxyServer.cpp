#include <proxyServer.h>
#include <serverSocket.h>

proxyServer::proxyServer(AbstractActor &actor, uint16_t port) : t([&actor, port]() {  startThread(actor, port); }){ }
proxyServer::~proxyServer() { t.join(); };

void proxyServer::startThread(AbstractActor &actor, uint16_t port) {
	auto connection = ServerSocket::getConnection(port);
	while (true) {
		uint32_t command;
		switch (connection.readInt()) {
			case postType::Async:
				command = connection.readInt();
				actor.post(command);
				break;
			case postType::Sync:
				command = connection.readInt();
				connection.writeInt(static_cast<uint32_t>(actor.postSync(command)));
				break;
			case postType::Restart:
				actor.restart();
				continue;
			default:
				continue;
		}
		if (AbstractActor::COMMAND_SHUTDOWN == command)
			return;
	}
}
