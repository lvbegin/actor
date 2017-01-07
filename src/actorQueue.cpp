#include <actorQueue.h>

ActorQueue::ActorQueue(std::shared_ptr<MessageQueue> queue) : queue(std::move(queue)) { }

ActorQueue::~ActorQueue() = default;

StatusCode ActorQueue::postSync(int code, std::vector<uint8_t> params) const {
	return queue->postSync(MessageType::COMMAND_MESSAGE, code, params);
}

void ActorQueue::post(int code, std::vector<uint8_t> params) const {
	queue->post(MessageType::COMMAND_MESSAGE, code, params);
}

