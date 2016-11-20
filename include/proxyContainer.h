#ifndef PROXY_CONTAINER_H__
#define PROXY_CONTAINER_H__

#include <executor.h>
#include <sharedMap.h>
#include <proxyServer.h>

#include <atomic>

class ProxyContainer {
public:
	ProxyContainer();
	~ProxyContainer();
	void createNewProxy(std::shared_ptr<AbstractActor> actor, Connection connection);
	void deleteProxy(int i);
private:
	SharedMap<int, proxyServer> proxies;
	Executor executor;
	static std::atomic<int> proxyId;
	 int newproxyId(void);
};

#endif
