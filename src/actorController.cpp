#include <actor.h>
#include <actorController.h>

ActorController::ActorController() = default;
ActorController::~ActorController() = default;

void ActorController::addActor(std::shared_ptr<Actor> actor) { actors.push_back(actor); }
void ActorController::restartActor(const std::string &name) { (*find(name))->restart(); }
void ActorController::restartAll() { std::for_each(actors.begin(), actors.end(), [](std::shared_ptr<Actor> &actor){ actor->restart();} );}
void ActorController::removeActor(const std::string &name) { actors.erase(find(name)); }

std::vector<std::shared_ptr<Actor>>::iterator ActorController::find(const std::string & name) {
	auto it = std::find_if(actors.begin(), actors.end(), [&name](std::shared_ptr<Actor> &actors) { return (0 == name.compare(actors->getName())); });
	if (actors.end() == it)
		THROW(std::runtime_error, "Actor not found");
	return it;
}

