#include <actor.h>
#include <actorController.h>

ActorController::ActorController() = default;
ActorController::~ActorController() = default;

void ActorController::addActor(ActorRef actor) { actors.push_back(actor); }
void ActorController::restartActor(const std::string &name) { (*find(name))->restart(); }
void ActorController::restartAll() { std::for_each(actors.begin(), actors.end(), [](ActorRef &actor){ actor->restart();} );}
void ActorController::removeActor(const std::string &name) { actors.erase(find(name)); }

std::vector<ActorRef>::iterator ActorController::find(const std::string & name) {
	auto it = std::find_if(actors.begin(), actors.end(), [&name](ActorRef &actors) { return (0 == name.compare(actors->getName())); });
	if (actors.end() == it)
		THROW(std::runtime_error, "Actor not found");
	return it;
}

