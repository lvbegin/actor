#ifndef CONNECTION_H__
#define CONNECTION_H__

#include <cstdint>
#include <unistd.h>
#include <string>

class Connection {
public:
	Connection();
	Connection(int fd);
	~Connection();
	Connection(const Connection &connection) = delete;
	Connection &operator=(const Connection &connection) = delete;
	Connection(Connection &&connection);
	Connection &operator=(Connection &&connection);
	void writeInt(uint32_t hostValue);
	uint32_t readInt(void);
	void writeString(std::string hostValue);
	std::string readString(void);
	void writeBytes(const void *buffer, size_t count);
	void readBytes(void *buffer, size_t count, int timeout = 5);
private:
	void readBytesNonBlocking(void *buffer, size_t count);

	int fd;
	fd_set set;
};

#endif
