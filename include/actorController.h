#ifndef ACTOR_CONTROLLER_H__
#define ACTOR_CONTROLLER_H__

#include <exception.h>

#include <memory>
#include <algorithm>

class Actor;
using ActorRef = std::shared_ptr<Actor>;

class ActorController {
public:
	ActorController();
	~ActorController();

	void addActor(ActorRef actor);
	void restartActor(const std::string &name);
	void restartAll();
	void removeActor(const std::string &name);

private:
	std::vector<ActorRef>::iterator find(const std::string & name);

	std::vector<ActorRef> actors;
};

#endif
