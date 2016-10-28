#include <actorRegistry.h>
#include <socket.h>

#include <functional>

static void threadBody(uint16_t port, std::function<void(Socket &s)> body);

ActorRegistry::ActorRegistry(uint16_t port) : t([this, port]() {  threadBody(port, [this](Socket &s) { registryBody(s); }); }) { }

static void threadBody(uint16_t port, std::function<void(Socket &s)> body) {
	auto s = std::make_unique<Socket>(port);
	body(*s);
}

ActorRegistry::~ActorRegistry() { t.join(); }

void ActorRegistry::registryBody(Socket &s) {
	Socket connection = s.getNextConnection();
	std::string name = "dummyName";
	others.insert(name, std::move(connection)); //LOLO: should receive first the name
}

void ActorRegistry::addReference(std::string registryName, std::string host, uint16_t port) {
	Socket s(host, port);
	s.establishConnection();
	others.insert(registryName, std::move(s));
}

void ActorRegistry::removeReference(std::string registryName) { others.erase(registryName); }

void ActorRegistry::registerActor(std::string name, abstractActor &actor) {
	actors.insert(name, std::move(std::unique_ptr<abstractActor>(&actor)));
}

void ActorRegistry::unregisterActor(std::string name) { actors.erase(name); }
