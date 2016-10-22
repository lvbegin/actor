#ifndef ABSTRACT_ACTOR_H__
#define ABSTRACT_ACTOR_H__

#include <cstdint>

enum class actorReturnCode  : uint32_t { ok, shutdown, error, };

class abstractActor {
public:
	virtual ~abstractActor() { }
	virtual actorReturnCode postSync(int i) = 0;
	virtual void post(int i) = 0;
	virtual void restart(void) = 0;
	static const uint32_t COMMAND_SHUTDOWN = 0;
protected:
	abstractActor() { };

};

#endif
