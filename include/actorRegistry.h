#ifndef ACTOR_REGISTRY_H__
#define ACTOR_REGISTRY_H__

#include <AbstractActor.h>
#include <cstdint>
#include <thread>
#include <memory>

#include <serverSocket.h>
#include <sharedMap.h>

using actorPtr = std::shared_ptr<AbstractActor>;

class ActorRegistry {
public:
	ActorRegistry(std::string name, uint16_t port);
	~ActorRegistry();
	std::string addReference(std::string host, uint16_t port);
	void removeReference(std::string registryName);
	void registerActor(std::string name, AbstractActor &actor);
	void unregisterActor(std::string name);
	actorPtr  getActor(std::string name);
private:
	std::string name;
	SharedMap<std::string, Connection> others;
	SharedMap<std::string, std::shared_ptr<AbstractActor>> actors;
	std::thread t;
	bool terminated;

	void registryBody(ServerSocket &s);
};

#endif
