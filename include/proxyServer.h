#ifndef PROXY_SERVER_H__
#define PROXY_SERVER_H__

#include <AbstractActor.h>
#include <connection.h>

#include <thread>

enum postType : uint32_t { Sync, Async, Restart, } ;

class proxyServer {
public:
	proxyServer(AbstractActor &actor, Connection connection);
	~proxyServer();
private:
	Connection connection;
	std::thread t;
	static void startThread(AbstractActor &actor, uint16_t port);
	void startThread(AbstractActor &actor);
};

#endif
