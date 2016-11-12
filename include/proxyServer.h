#ifndef PROXY_SERVER_H__
#define PROXY_SERVER_H__

#include <AbstractActor.h>
#include <connection.h>

#include <thread>

enum postType : uint32_t { Sync, Async, Restart, } ;

class proxyServer {
public:
	proxyServer(std::shared_ptr<AbstractActor> actor, Connection connection);
	~proxyServer();

	proxyServer(const proxyServer &p) = delete;
	proxyServer &operator=(const proxyServer &p) = delete;
	proxyServer &operator=(proxyServer &&p);
	proxyServer(proxyServer &&p);

private:
	std::thread t;
	static void startThread(std::shared_ptr<AbstractActor> actor, Connection connection);
};

#endif
