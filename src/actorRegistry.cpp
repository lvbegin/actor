#include <actorRegistry.h>
#include <clientSocket.h>
#include <serverSocket.h>
#include <proxyServer.h>
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
			auto connection = s.acceptOneConnection();
			auto otherName = connection.readString();
			connection.writeString(this->name);
			//others.insert(std::move(name), std::move(connection));
			//this is for simplification
			auto actorName = connection.readString();
			try {
				auto actor = actors.find(actorName).get();
				auto p = new proxyServer(*actor, std::move(connection)); //what to do with that proxyServer ...
				delete p;
				connection.writeInt(1);
			} catch (std::out_of_range e) {
				connection.writeInt(0);
				continue;
			}
		}
		catch (std::exception e) { }
	}
	return ;
}

std::string ActorRegistry::addReference(std::string host, uint16_t port) {
	auto connection = ClientSocket::openHostConnection(host, port);
	connection.writeString(name);
	//should read status...and react in consequence: what if failure returned?
	std::string otherName = connection.readString();
	others.insert(otherName, std::move(connection));
	return otherName;
}

void ActorRegistry::removeReference(std::string registryName) { others.erase(registryName); }

void ActorRegistry::registerActor(std::string name, AbstractActor &actor) {
	actors.insert(name, std::move(std::unique_ptr<AbstractActor>(&actor)));
}

void ActorRegistry::unregisterActor(std::string name) { actors.erase(name); }

actorPtr  ActorRegistry::getActor(std::string name) {
	try {
		return actors.find(name);
	} catch (std::out_of_range e) {
		return std::shared_ptr<AbstractActor>();
	}
}
