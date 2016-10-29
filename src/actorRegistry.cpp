#include <actorRegistry.h>
#include <clientSocket.h>
#include <serverSocket.h>
#include <functional>

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body);

ActorRegistry::ActorRegistry(std::string name, uint16_t port) : name(name),
					t([this, port]() {  threadBody(port, [this](ServerSocket &s) { registryBody(s); }); }) { }

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body) {
	auto s = std::make_unique<ServerSocket>(port);
	body(*s);
}

ActorRegistry::~ActorRegistry() { t.join(); }

void ActorRegistry::registryBody(ServerSocket &s) {
	auto connection = s.acceptOneConnection();
	auto name = connection.readString();
	others.insert(std::move(name), std::move(connection)); //LOLO: should receive first the name
}

void ActorRegistry::addReference(std::string registryName, std::string host, uint16_t port) {
	auto connection = ClientSocket::openHostConnection(host, port);
	connection.writeString(name);
	others.insert(registryName, std::move(connection));
}

void ActorRegistry::removeReference(std::string registryName) { others.erase(registryName); }

void ActorRegistry::registerActor(std::string name, abstractActor &actor) {
	actors.insert(name, std::move(std::unique_ptr<abstractActor>(&actor)));
}

void ActorRegistry::unregisterActor(std::string name) { actors.erase(name); }
