#ifndef DESCRIPTOR_WAIT_H__
#define DESCRIPTOR_WAIT_H__

#include <exception.h>

#include <sys/select.h>
#include <stdexcept>

template <typename E1, typename E2>
void waitForRead(int &fd, fd_set *set, struct timeval *timeout) {
	if (-1 == fd)
		THROW(std::runtime_error, "invalid fd.");
	switch(select(fd + 1, set, NULL, NULL, timeout)) {
		case 0:
			THROW(E1, "timeout on read.");
		case -1:
			THROW(E2, "error while waiting for read.");
		default:
			return ;
	}
}

template <typename E1, typename E2>
void waitForRead(int &fd, fd_set *set, int timeoutInSeconds) {
	if (-1 == fd)
		THROW(std::runtime_error, "invalid fd.");
	struct timeval timeout { timeoutInSeconds, 0 };
	waitForRead<E1, E2>(fd, set, &timeout);
}

#endif
