#ifndef ACTOR_REGISTRY_H__
#define ACTOR_REGISTRY_H__

#include <AbstractActor.h>
#include <cstdint>
#include <thread>
#include <memory>
#include <vector>

#include <netinet/in.h>

#include <proxyContainer.h>
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
	enum class RegistryCommand : uint32_t { REGISTER_REGISTRY, SEARCH_ACTOR, };
	std::string name;
	bool terminated;
	SharedMap<std::string, struct sockaddr_in> registryAddresses;
	SharedMap<std::string, std::shared_ptr<AbstractActor>> actors;
	//std::vector<proxyServer> proxies;
	ProxyContainer proxies;
	std::thread t;

	void registryBody(ServerSocket &s);
	actorPtr getLocalActor(std::string &name);
	actorPtr getOutsideActor(std::string &name);
};

#endif
