#include <actorRegistry.h>
#include <clientSocket.h>
#include <serverSocket.h>
#include <proxyClient.h>
#include <functional>

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body);

ActorRegistry::ActorRegistry(std::string name, uint16_t port) : name(name),
					t([this, port]() {  threadBody(port, [this](ServerSocket &s) { registryBody(s); }); }),
					terminated(false) { }

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body) {
	auto s = std::make_unique<ServerSocket>(port);
	body(*s);
}

ActorRegistry::~ActorRegistry() {
	terminated = true;
	t.join();
}

void ActorRegistry::registryBody(ServerSocket &s) {
	while (!terminated) {
		try {
			struct sockaddr_in client_addr {};
			auto connection = s.acceptOneConnection(2, &client_addr);
			switch (static_cast<ActorRegistry::registryCommand_t>(connection.readInt())) {
				case registryCommand_t::REGISTER_REGISTRY:
					registryAddresses.insert(connection.readString(), client_addr);
					connection.writeString(this->name);
					break;
				case registryCommand_t::SEARCH_ACTOR:
					try {
						auto actor = actors.find(connection.readString());
						connection.writeInt(1);
						proxies.push_back(proxyServer(*actor.get(), std::move(connection))); //ok and now when to remove ?
					} catch (std::out_of_range e) {
						connection.writeInt(0);
					}
					break;
				default:
					THROW(std::runtime_error, "unknown case.");
			}
		}
		catch (std::exception e) { }
	}
}

std::string ActorRegistry::addReference(std::string host, uint16_t port) {
	auto connection = ClientSocket::openHostConnection(host, port);
	connection.writeInt(static_cast<uint32_t>(registryCommand_t::REGISTER_REGISTRY));
	connection.writeString(name);
	//should read status...and react in consequence: what if failure returned?
	std::string otherName = connection.readString();
	registryAddresses.insert(otherName, ClientSocket::toSockAddr(host, port));
	return otherName;
}

void ActorRegistry::removeReference(std::string registryName) { registryAddresses.erase(registryName); }

void ActorRegistry::registerActor(std::string name, AbstractActor &actor) { actors.insert(name, std::move(actorPtr(&actor))); }

void ActorRegistry::unregisterActor(std::string name) { actors.erase(name); }

actorPtr  ActorRegistry::getActor(std::string name) {
	try {
		return getLocalActor(name);
	} catch (std::out_of_range e) {
		return getOutsideActor(name);
	}
}

actorPtr ActorRegistry::getLocalActor(std::string &name) { return actors.find(name); }

actorPtr ActorRegistry::getOutsideActor(std::string &name) {
	actorPtr actor;
	registryAddresses.for_each([&actor, &name](std::pair<const std::string, struct sockaddr_in> &c) {
		auto connection = ClientSocket::openHostConnection(c.second);
		connection.writeInt(static_cast<uint32_t>(registryCommand_t::SEARCH_ACTOR));
		connection.writeString(name);
		if (1 == connection.readInt())
			actor.reset(new proxyClient(std::move(connection)));
	});
	return actor;
}
