#ifndef ACTOR_REGISTRY_H__
#define ACTOR_REGISTRY_H__

#include <cstdint>
#include <thread>
#include <memory>

#include <abstractActor.h>
#include <serverSocket.h>
#include <sharedMap.h>

class ActorRegistry {
public:
	ActorRegistry(uint16_t port);
	~ActorRegistry();
	void addReference(std::string registryName, std::string host, uint16_t port);
	void removeReference(std::string registryName);
	void registerActor(std::string name, abstractActor &actor);
	void unregisterActor(std::string name);
private:
	SharedMap<std::string, Connection> others;
	SharedMap<std::string, std::unique_ptr<abstractActor>> actors;
	std::thread t;

	void registryBody(ServerSocket &s);
};

#endif
