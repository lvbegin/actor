#ifndef ABSTRACT_ACTOR_H__
#define ABSTRACT_ACTOR_H__

#include <rc.h>

class AbstractActor {
public:
	virtual ~AbstractActor() = default;
	virtual returnCode postSync(int i) = 0;
	virtual void post(int i) = 0;
	virtual void restart(void) = 0;
	static const uint32_t COMMAND_SHUTDOWN = 0;
protected:
	AbstractActor() { };

};

#endif
