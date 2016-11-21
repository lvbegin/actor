#ifndef PROXY_CLIENT_H__
#define PROXY_CLIENT_H__

#include <AbstractActor.h>
#include <connection.h>

#include <string>

class proxyClient : public AbstractActor {
public:
	proxyClient(Connection connection);
	~proxyClient();

	returnCode postSync(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	void post(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	void restart(void);
private:
	Connection connection;
};

#endif
