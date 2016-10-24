#include <actorRegistry.h>
#include <socket.h>

#include <functional>

static void threadBody(uint16_t port, std::function<void(Socket &s)> body);

ActorRegistry::ActorRegistry(uint16_t port) : t([this, port]() {  threadBody(port, [this](Socket &s) { registryBody(s); }); }) { }

static void threadBody(uint16_t port, std::function<void(Socket &s)> body) {
	auto s = std::make_unique<Socket>(port);
	body(*s);
}

ActorRegistry::~ActorRegistry() {
	t.join();
}

void ActorRegistry::registryBody(Socket &s) {

	auto connection = s.getNextConnection();
	others.push_back(connection);
}

void ActorRegistry::addReference(std::string host, uint16_t port) {
	Socket s(host, port);
	s.establishConnection();
	std::unique_lock<std::mutex> l(othersMutex);
	others.push_back(s);
}

void ActorRegistry::registerActor(std::string name, abstractActor &actor) {
	std::unique_lock<std::mutex> l(actorsMutex);
	if (actors.end() != actors.find(name))
		throw std::runtime_error("actorRegistry: actor already exist");
	actors[name] = std::unique_ptr<abstractActor>(&actor);
}

void ActorRegistry::unregisterActor(std::string name) {
	std::unique_lock<std::mutex> l(actorsMutex);
	actors.erase(name);
}
