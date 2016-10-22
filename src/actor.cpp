#include <actor.h>

Actor::Actor(std::function<actorReturnCode(int)> body)  : body(body), thread([&body, this]() { actorBody(body); }) { }

Actor::~Actor() {
	if (thread.joinable())
		thread.join();
};

std::future<Actor::actorReturnCode> Actor::putMessage(int i) {
	std::unique_lock<std::mutex> l(mutexQueue);

	struct message  *m = new message(i);
	q.push(m);
	condition.notify_one();
	return m->promise.get_future();
}

Actor::actorReturnCode Actor::postSync(int i) { return putMessage(i).get(); }

void Actor::post(int i) { putMessage(i); }

std::unique_ptr<struct Actor::message> Actor::getMessage(void) {
	std::unique_lock<std::mutex> l(mutexQueue);

	condition.wait(l, [this]() { return !q.empty();});
	std::unique_ptr<struct message> message(q.front());
	q.pop();
	return message;
}

void Actor::actorBody(std::function<actorReturnCode(int)> body) {
	while (true) {
		std::unique_ptr<struct Actor::message> message(getMessage());
		if (COMMAND_SHUTDOWN == message->command) {
			message->promise.set_value(actorReturnCode::shutdown);
			return;
		}
		switch (body(message->command)) {
		case actorReturnCode::ok:
			message->promise.set_value(actorReturnCode::ok);
			break;
		case actorReturnCode::shutdown:
			message->promise.set_value(actorReturnCode::shutdown);
			return;
		default:
			message->promise.set_value(actorReturnCode::error);
			//notify parent.
			return;
		}
	}
}

void Actor::restart(void) {
	post(COMMAND_SHUTDOWN);
	if (thread.joinable())
		thread.join();
	thread = std::thread([this]() { actorBody(body); });
}
