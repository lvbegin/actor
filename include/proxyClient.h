#ifndef PROXY_CLIENT_H__
#define PROXY_CLIENT_H__

#include <AbstractActor.h>
#include <connection.h>
#include <string>

class proxyClient : public AbstractActor {
public:
	proxyClient(std::string host, uint16_t port);
	~proxyClient();
	actorReturnCode postSync(int i);
	void post(int i);
	void restart(void);
private:
	Connection connection;
};

#endif
