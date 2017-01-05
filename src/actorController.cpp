#include <actorController.h>
#include <exception.h>
#include <command.h>

#include <algorithm>

ActorController::ActorController() = default;
ActorController::~ActorController() = default;

void ActorController::add(uint32_t id, std::shared_ptr<MessageQueue> actorLink) {
	std::unique_lock<std::mutex> l(mutex);

	actors.insert(std::pair<uint32_t, std::shared_ptr<MessageQueue>>(id, std::move(actorLink)));
}
void ActorController::remove(uint32_t id) {
	std::unique_lock<std::mutex> l(mutex);

	actors.erase(id);
}
void ActorController::restartOne(uint32_t id) const {
	std::unique_lock<std::mutex> l(mutex);

	auto it = actors.find(id);
	if (actors.end() != it)
		restart(it->second);
}
void ActorController::restartAll() const {
	std::unique_lock<std::mutex> l(mutex);

	std::for_each(actors.begin(), actors.end(), [](const std::pair<uint32_t, std::shared_ptr<MessageQueue>> &e) { restart(e.second);} );
}

void ActorController::restart(const std::shared_ptr<MessageQueue> &link) { link->post(MessageType::COMMAND_MESSAGE, Command::COMMAND_RESTART); }
