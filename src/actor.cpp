#include <actor.h>

Actor::Actor(std::function<returnCode(int)> body)  : body(body), executor(new Executor(body)) { }

Actor::~Actor() = default;

returnCode Actor::postSync(int i) { return executor->postSync(i); }

void Actor::post(int i) { executor->post(i); }

void Actor::restart(void) { executor.reset(new Executor(body)); }
