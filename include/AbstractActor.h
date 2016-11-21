#ifndef ABSTRACT_ACTOR_H__
#define ABSTRACT_ACTOR_H__

#include <rc.h>

#include <vector>

class AbstractActor {
public:
	virtual ~AbstractActor() = default;
	virtual returnCode postSync(int i, std::vector<unsigned char> params = std::vector<unsigned char>()) = 0;
	virtual void post(int i, std::vector<unsigned char> params = std::vector<unsigned char>()) = 0;
	virtual void restart(void) = 0;
	static const uint32_t COMMAND_SHUTDOWN = 0;
protected:
	AbstractActor() { };

};

#endif
