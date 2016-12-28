#include <actorStateMachine.h>
#include <exception.h>

ActorStateMachine::ActorStateMachine() : state(ActorState::RUNNING) { }
ActorStateMachine::~ActorStateMachine() = default;

#include <iostream>

void ActorStateMachine::moveTo(ActorState newState) {
	std::unique_lock<std::mutex> l(mutex);

	switch (newState) {
		case ActorState::RUNNING:
			if (ActorState::STOPPED == state || ActorState::RUNNING == state)
				THROW(std::runtime_error, "actor cannot move to running state.");
			state = newState;
			stateChanged.notify_one();
			break;
		case ActorState::RESTARTING:
			if (ActorState::STOPPED == state || ActorState::RESTARTING == state)
				THROW(std::runtime_error, "actor cannot move to restarting state.");
			state = newState;
			break;
		case ActorState::STOPPED:
//			std::cout << "state= " << (this->state == ActorState::RUNNING ? "RUNNING" :
//										this->state == ActorState::RESTARTING ? "RESTARTING" :
//												"STOPPED")
//			  	  	  << std::endl;
			if (ActorState::STOPPED == state)
				THROW(std::runtime_error, "actor cannot move to stopped state.");
			stateChanged.wait(l, [this]() { return ActorState::RUNNING == this->state; });
			state = newState;
			break;
	default:
		THROW(std::runtime_error, "unknown new Actor State.");
	}
}

void ActorStateMachine::ensureState(ActorState state) {
	if (!(this->state == state))
		THROW(std::runtime_error, "bad Actor state machine state.");
}
