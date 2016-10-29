#ifndef CLIENT_SOCKET_H__
#define CLIENT_SOCKET_H__

#include <string>
#include <cstdint>

#include <connection.h>

class ClientSocket {
public:
	ClientSocket() = delete;
	~ClientSocket() = delete;

	static Connection openHostConnection(std::string host, uint16_t port);
private:
	uint16_t port;
	std::string host;
};


#endif
