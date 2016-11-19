#ifndef PROXY_SERVER_H__
#define PROXY_SERVER_H__

#include <AbstractActor.h>
#include <connection.h>

#include <thread>

class ProxyContainer;

enum class postType : uint32_t { Sync, Async, Restart, } ;

class proxyServer {
public:
	proxyServer(std::shared_ptr<AbstractActor> actor, Connection connection, std::function<void(void)> notifyTerminate);
	~proxyServer();

	proxyServer(const proxyServer &p) = delete;
	proxyServer &operator=(const proxyServer &p) = delete;
	proxyServer &operator=(proxyServer &&p) = delete;
	proxyServer(proxyServer &&p) = delete;

private:
	std::thread t;
	static void startThread(std::shared_ptr<AbstractActor> actor, Connection connection, std::function<void(void)> notifyTerminate);
};

#endif
