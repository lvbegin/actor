#ifndef DESCRIPTOR_WAIT_H__
#define DESCRIPTOR_WAIT_H__

#include <sys/select.h>
#include <stdexcept>

template <typename E1, typename E2>
void waitForRead(int &fd, fd_set *set, struct timeval *timeout) {
	if (-1 == fd)
		throw std::runtime_error(std::string(__func__ ) + std::string(": invalid fd."));
	switch(select(fd + 1, set, NULL, NULL, timeout)) {
		case 0:
			throw E1(std::string(__func__ ) + std::string(": timeout on read."));
		case -1:
			throw E2(std::string(__func__ ) + std::string(": error while waiting for read."));
		default:
			return ;
	}
}

template <typename E1, typename E2>
void waitForRead(int &fd, fd_set *set, int timeoutInSeconds) {
	if (-1 == fd)
		throw std::runtime_error(std::string(__func__ ) + ": invalid fd.");
	struct timeval timeout { timeoutInSeconds, 0 };
	waitForRead<E1, E2>(fd, set, &timeout);
}

#endif
