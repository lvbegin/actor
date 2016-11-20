#include <actor.h>

Actor::Actor(std::function<returnCode(int)> body)  : body(body), executor(new Executor(body)) { }

Actor::~Actor() { stopThread(); };

returnCode Actor::postSync(int i) { return executor->postSync(i); }

void Actor::post(int i) { executor->post(i); }

void Actor::restart(void) {
	stopThread();
//	thread = std::thread([this]() { actorBody(body); });
	executor.reset(new Executor(body));
}

void Actor::stopThread(void) { executor.reset(nullptr); }
