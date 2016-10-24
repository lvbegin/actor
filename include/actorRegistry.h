#ifndef ACTOR_REGISTRY_H__
#define ACTOR_REGISTRY_H__

#include <cstdint>
#include <thread>
#include <memory>
#include <vector>
#include <map>
#include <mutex>

#include <abstractActor.h>
#include <socket.h>

class ActorRegistry {
public:
	ActorRegistry(uint16_t port);
	~ActorRegistry();
	void addReference(std::string host, uint16_t port);
	void registerActor(std::string name, abstractActor &actor);
	void unregisterActor(std::string name);
private:
	std::vector<Socket> others;
	std::map<std::string, std::unique_ptr<abstractActor>> actors;
	std::mutex othersMutex;
	std::mutex actorsMutex;
	std::thread t;

	void registryBody(Socket &s);
};

#endif
