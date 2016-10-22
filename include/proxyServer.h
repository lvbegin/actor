#ifndef PROXY_SERVER_H__
#define PROXY_SERVER_H__

#include <abstractActor.h>

#include <thread>

enum postType : uint32_t { Sync, Async, Restart, } ;

class proxyServer {
public:
	proxyServer(abstractActor &actor, uint16_t port);
	~proxyServer();
private:
	std::thread t;
	static void startThread(abstractActor &actor, uint16_t port);
};

#endif
