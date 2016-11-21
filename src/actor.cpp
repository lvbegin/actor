#include <actor.h>

Actor::Actor(std::function<returnCode(int, const std::vector<unsigned char> &)> body)  : body(body), executor(new Executor(body)) { }

Actor::~Actor() = default;

returnCode Actor::postSync(int i, std::vector<unsigned char> params) { return executor->postSync(i, params); }

void Actor::post(int i, std::vector<unsigned char> params) { executor->post(i, params); }

void Actor::restart(void) { executor.reset(new Executor(body)); }
