#ifndef ACTOR_CONTROLLER_H__
#define ACTOR_CONTROLLER_H__

#include <exception.h>

#include <memory>
#include <algorithm>

class Actor;

class ActorController {
public:
	ActorController();
	~ActorController();

	void addActor(std::shared_ptr<Actor> actor);
	void restartActor(const std::string &name);
	void restartAll();
	void removeActor(const std::string &name);

private:
	std::vector<std::shared_ptr<Actor>>::iterator find(const std::string & name);

	std::vector<std::shared_ptr<Actor>> actors;
};

#endif
