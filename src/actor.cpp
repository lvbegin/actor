#include <actor.h>

Actor::Actor(std::function<actorReturnCode(int)> body)  : body(body), thread([this]() { actorBody(this->body); }) { }

Actor::~Actor() { stopThread(); };

std::future<actorReturnCode> Actor::putMessage(int i) {
	std::unique_lock<std::mutex> l(mutexQueue);

	struct message  m(i);
	auto future = m.promise.get_future();
	q.push(std::move(m));
	condition.notify_one();
	return future;
}

actorReturnCode Actor::postSync(int i) { return putMessage(i).get(); }

void Actor::post(int i) { putMessage(i); }

struct Actor::message Actor::getMessage(void) {
	std::unique_lock<std::mutex> l(mutexQueue);

	condition.wait(l, [this]() { return !q.empty();});
	auto message = std::move(q.front());
	q.pop();
	return message;
}

void Actor::actorBody(std::function<actorReturnCode(int)> body) {
	while (true) {
		struct Actor::message message(getMessage());
		if (COMMAND_SHUTDOWN == message.command) {
			message.promise.set_value(actorReturnCode::shutdown);
			return;
		}
		switch (body(message.command)) {
		case actorReturnCode::ok:
			message.promise.set_value(actorReturnCode::ok);
			break;
		case actorReturnCode::shutdown:
			message.promise.set_value(actorReturnCode::shutdown);
			return;
		default:
			message.promise.set_value(actorReturnCode::error);
			//notify parent.
			return;
		}
	}
}

void Actor::restart(void) {
	stopThread();
	thread = std::thread([this]() { actorBody(body); });
}

void Actor::stopThread(void) {
	post(COMMAND_SHUTDOWN);
	if (thread.joinable())
		thread.join();
}
