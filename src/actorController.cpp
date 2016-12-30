#include <actorController.h>
#include <exception.h>
#include <command.h>

#include <algorithm>

ActorController::ActorController() = default;
ActorController::~ActorController() = default;

void ActorController::add(std::string name, std::shared_ptr<MessageQueue> actorLink) {
	std::unique_lock<std::mutex> l(mutex);

	actors.insert(std::pair<std::string, std::shared_ptr<MessageQueue>>(std::move(name), std::move(actorLink)));
}
void ActorController::remove(const std::string &name) {
	std::unique_lock<std::mutex> l(mutex);

	actors.erase(name);
}
void ActorController::restartOne(const std::string &name) const {
	std::unique_lock<std::mutex> l(mutex);

	auto it = actors.find(name);
	if (actors.end() != it)
		restart(it->second);
}
void ActorController::restartAll() const {
	std::unique_lock<std::mutex> l(mutex);

	std::for_each(actors.begin(), actors.end(), [](const std::pair<std::string, std::shared_ptr<MessageQueue>> &e) { restart(e.second);} );
}

void ActorController::restart(const std::shared_ptr<MessageQueue> &link) { link->post(MessageType::COMMAND_MESSAGE, Command::COMMAND_RESTART); }
