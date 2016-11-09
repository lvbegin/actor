#include <proxyServer.h>
#include <serverSocket.h>

#include <arpa/inet.h>

proxyServer::proxyServer(AbstractActor &actor, Connection connection) : connection(std::move(connection)),
t([&actor, this]() {  startThread(actor); }){ }

proxyServer::proxyServer(proxyServer &&p) { *this = std::move(p); }
proxyServer &proxyServer::operator=(proxyServer &&p) {
	connection = std::move(p.connection);
	t = std::move(p.t);
	return *this;
}


proxyServer::~proxyServer() { t.join(); };

void proxyServer::startThread(AbstractActor &actor) {
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
