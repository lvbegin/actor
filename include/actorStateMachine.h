#ifndef ACTOR_STATE_MACHINE_H__
#define ACTOR_STATE_MACHINE_H__

#include <mutex>
#include <condition_variable>

class ActorStateMachine {
public:
	enum class ActorState { RUNNING, RESTARTING, STOPPED, };
	ActorStateMachine();
	~ActorStateMachine();
	void moveTo(ActorState newState);
	bool isIn(ActorState state);
private:
	std::mutex mutex;
	std::condition_variable stateChanged;
	ActorState state;
};

#endif
