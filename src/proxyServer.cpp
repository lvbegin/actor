#include <proxyServer.h>
#include <socket.h>

#include <stdexcept>
#include <memory>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


proxyServer::proxyServer(abstractActor &actor, uint16_t port) : t([&actor, port]() {  startThread(actor, port); }){ }
proxyServer::~proxyServer() { t.join(); };

void proxyServer::startThread(abstractActor &actor, uint16_t port) {
	auto s = std::make_unique<Socket>(port);
	s->establishConnection();
	while (true) {
		uint32_t type;
		uint32_t command;
		s->readBytes(&type, sizeof(type));
		switch (ntohl(type)) {
			case postType::Async:
				command = s->readInt();
				actor.post(command);
				break;
			case postType::Sync: {
				command = s->readInt();
				s->writeInt(static_cast<uint32_t>(actor.postSync(command)));
				break;
			}
			case postType::Restart:
				actor.restart();
				break;

			default:
				continue;
		}
		if (abstractActor::COMMAND_SHUTDOWN == command)
			return;
	}
}
