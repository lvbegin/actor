#ifndef PROXY_SERVER_H__
#define PROXY_SERVER_H__

#include <abstractActor.h>

#include <cstdint>
#include <thread>

enum postType : uint32_t { Sync, Async, Restart, } ;


class proxyServer {
public:
	proxyServer();
	~proxyServer();
	void start(abstractActor &actor, uint16_t port);

private:
	std::thread t;
	static void startThread(abstractActor &actor, uint16_t port);
};

#endif
