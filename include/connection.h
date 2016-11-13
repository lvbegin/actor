#ifndef CONNECTION_H__
#define CONNECTION_H__

#include <cstdint>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

class Connection {
public:
	Connection();
	Connection(int fd);
	~Connection();
	Connection(const Connection &connection) = delete;
	Connection &operator=(const Connection &connection) = delete;
	Connection(Connection &&connection);
	Connection &operator=(Connection &&connection);
	template<typename T>
	void writeInt(T hostValue) {
		const uint32_t sentValue = htonl(static_cast<uint32_t>(hostValue));
		writeBytes(&sentValue, sizeof(sentValue));
	}

	template<typename T>
	T readInt(void) {
		uint32_t value;
		readBytes(&value, sizeof(value));
		return static_cast<T>(ntohl(value));
	}
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
