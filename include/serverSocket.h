#ifndef SOCKET_H__
#define SOCKET_H__

#include <cstdint>
#include <cstddef>
#include <string>

#include <connection.h>

class ServerSocket {
public:
	static Connection getConnection(int port);
	ServerSocket(uint16_t port);

	ServerSocket(const ServerSocket &s) = delete;
	ServerSocket &operator=(const ServerSocket &s) = delete;

	ServerSocket(ServerSocket&& s);
	ServerSocket &operator=(ServerSocket&& s);

	~ServerSocket();
	Connection acceptOneConnection(void);
private:
	static int listenOnSocket(uint16_t port);
	void closeSocket(void);
	int acceptFd;
};

#endif
