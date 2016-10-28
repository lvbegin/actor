#ifndef SOCKET_H__
#define SOCKET_H__

#include <cstdint>
#include <cstddef>
#include <string>

class Socket {
public:
	Socket(uint16_t port);
	Socket(std::string host, uint16_t port);

	Socket(const Socket &s) = delete;
	Socket &operator=(const Socket &s) = delete;

	Socket(Socket&& s);
	Socket &operator=(Socket&& s);

	~Socket();
	void establishConnection(void);
	Socket getNextConnection(void);
	void writeInt(uint32_t hostValue);
	uint32_t readInt(void);
	void writeBytes(const void *buffer, size_t count);
	void readBytes(void *buffer, size_t count);
private:
	Socket(uint16_t port, int fd);
	void connectHost(void);
	void acceptHost(void);
	enum class socketType {Client, Server, };
	int acceptFd;
	int connectionFd;
	socketType type;
	uint16_t port;
	std::string host;
};

#endif
