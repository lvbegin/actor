#ifndef ACTOR_REGISTRY_H__
#define ACTOR_REGISTRY_H__

#include <AbstractActor.h>
#include <cstdint>
#include <thread>
#include <memory>

#include <serverSocket.h>
#include <sharedMap.h>

class ActorRegistry {
public:
	ActorRegistry(std::string name, uint16_t port);
	~ActorRegistry();
	void addReference(std::string registryName, std::string host, uint16_t port);
	void removeReference(std::string registryName);
	void registerActor(std::string name, AbstractActor &actor);
	void unregisterActor(std::string name);
	template<typename V>
	V * getActor(std::string name) {
		auto actor = actors.find(name);
		if (actors.end() == actor)
			return nullptr;
		else
			return actor->second.get();
	}
private:
	std::string name;
	SharedMap<std::string, Connection> others;
	SharedMap<std::string, std::unique_ptr<AbstractActor>> actors;
	std::thread t;
	bool terminated;

	void registryBody(ServerSocket &s);
};

#endif
