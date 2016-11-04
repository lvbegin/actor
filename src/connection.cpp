#include <connection.h>

#include <stdexcept>
#include <arpa/inet.h>

Connection::Connection(int fd) : fd(fd) {
	FD_ZERO(&set);
	FD_SET(fd, &set);
}

Connection::~Connection() {
	if (-1 != fd)
		close(fd);
}

Connection::Connection(Connection &&connection) : fd(-1) { std::swap(fd, connection.fd); }

void Connection::writeInt(uint32_t hostValue) {
	const uint32_t sentValue = htonl(hostValue);
	writeBytes(&sentValue, sizeof(sentValue));
}

uint32_t Connection::readInt(void) {
	uint32_t value;
	readBytes(&value, sizeof(value));
	return ntohl(value);
}

void Connection::writeString(std::string hostValue) {
	const size_t nbBytesToWrite = hostValue.size() + 1;
	writeInt(nbBytesToWrite); //check max size
	writeBytes(hostValue.c_str(), nbBytesToWrite);
}

std::string Connection::readString(void) {
	const size_t size = readInt();
	char string[size]; //check max size
	readBytes(string, size);
	if (0 != string[size-1])
		throw std::runtime_error("Connection: non NULL-terminated string value");
	return std::string(string);
}


void Connection::writeBytes(const void *buffer, size_t count) {
	if (-1 == fd)
		throw std::runtime_error("Connection: invalid writeByte");
	const char *ptr = static_cast<const char *>(buffer);
	for (size_t nbTotalWritten = 0; nbTotalWritten < count; ) {
		const ssize_t nbWritten = write(fd, ptr + nbTotalWritten, count - nbTotalWritten);
		if (-1 == nbWritten && !(EINTR == errno))
			throw std::runtime_error("Connection: send bytes failed");
		nbTotalWritten += nbWritten;
	}
}

void Connection::readBytes(void *buffer, size_t count, int timeoutInSeconds) {
	if (-1 == fd)
		throw std::runtime_error("Connection: invalid writeByte");
	struct timeval timeout { timeoutInSeconds, 0 };
	switch(select(fd + 1, &set, NULL, NULL, &timeout)) {
		case 0:
			throw std::runtime_error("Connection: timeout on read");
		case -1:
			throw std::runtime_error("Connection: error while waiting for read.");
		default:
			readBytesNonBlocking(buffer, count);
	}
}

void Connection::readBytesNonBlocking(void *buffer, size_t count) {
	static const int timeoutOnRead = 60;
	if (-1 == fd)
		throw std::runtime_error("Connection: invalid writeByte");
	char *ptr = static_cast<char *>(buffer);
	struct timeval timeout { timeoutOnRead, 0 };
	for (size_t nbTotalRead = 0; nbTotalRead < count; ) {
		switch (select(fd + 1, &set, NULL, NULL, &timeout)) {
			case 0:
				throw std::runtime_error("Connection: reading failed");
			case -1:
				throw std::runtime_error("Connection: error while reading.");
			default: {
				const ssize_t nbRead = read(fd, ptr + nbTotalRead, count - nbTotalRead);
				if (0 >= nbRead)
					throw std::runtime_error("Connection: read bytes failed");
				nbTotalRead += nbRead;
			}
		}
	}
}
