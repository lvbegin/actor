#include <actorRegistry.h>
#include <clientSocket.h>
#include <serverSocket.h>
#include <functional>

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body);

ActorRegistry::ActorRegistry(uint16_t port) : t([this, port]() {  threadBody(port, [this](ServerSocket &s) { registryBody(s); }); }) { }

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body) {
	auto s = std::make_unique<ServerSocket>(port);
	body(*s);
}

ActorRegistry::~ActorRegistry() { t.join(); }

void ActorRegistry::registryBody(ServerSocket &s) {
	Connection connection = s.getNextConnection();
	std::string name = "dummyName";
	others.insert(name, std::move(connection)); //LOLO: should receive first the name
}

void ActorRegistry::addReference(std::string registryName, std::string host, uint16_t port) {
	Connection c = ClientSocket::connectHost(host, port);
	others.insert(registryName, std::move(c));
}

void ActorRegistry::removeReference(std::string registryName) { others.erase(registryName); }

void ActorRegistry::registerActor(std::string name, abstractActor &actor) {
	actors.insert(name, std::move(std::unique_ptr<abstractActor>(&actor)));
}

void ActorRegistry::unregisterActor(std::string name) { actors.erase(name); }
