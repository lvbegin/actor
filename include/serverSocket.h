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
	Connection acceptOneConnection(int timeout = 2, struct sockaddr_in *client_addr = NULL);
private:
	static int listenOnSocket(uint16_t port);
	void closeSocket(void);
	int acceptFd;
	fd_set set;
};

#endif
