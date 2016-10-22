#ifndef SOCKET_H__
#define SOCKET_H__

#include <cstdint>
#include <cstddef>
#include <string>

class Socket {
public:
	Socket(uint16_t port);
	Socket(std::string host, uint16_t port);
	~Socket();
	void establishConnection(void);
	void writeInt(uint32_t hostValue);
	uint32_t readInt(void);
	void writeBytes(const void *buffer, size_t count);
	void readBytes(void *buffer, size_t count);
private:
	void connectHost(void);
	void accept(void);
	enum class socketType {Client, Server, };
	int fd;
	socketType type;
	uint16_t port;
	std::string host;
};
#endif
