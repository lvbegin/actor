#ifndef ABSTRACT_ACTOR_H__
#define ABSTRACT_ACTOR_H__

#include <cstdint>

class abstractActor {
public:
	virtual ~abstractActor() { }
	enum class actorReturnCode  : uint32_t { ok, shutdown, error, };
	virtual actorReturnCode postSync(int i) = 0;
	virtual void post(int i) = 0;
	virtual void restart(void) = 0;
	static const int COMMAND_SHUTDOWN = 0;
protected:
	abstractActor() { };

};

#endif
