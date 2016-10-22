#ifndef PROXY_CLIENT_H__
#define PROXY_CLIENT_H__

#include <abstractActor.h>
#include <socket.h>
#include <cstdint>
#include <string>
#include <memory>

class proxyClient : public abstractActor {
public:
	proxyClient();
	~proxyClient();
	void start(std::string host, uint16_t port);
	actorReturnCode postSync(int i);
	void post(int i);
	void restart(void);
private:
	std::unique_ptr<Socket> s;
};

#endif
