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
	Connection getNextConnection(void);
private:
	int acceptHost(void);
	static int acceptOneConnection(int sockfd);
	static int serverSocket(uint16_t port);

	int acceptFd;
	uint16_t port;
};

#endif
