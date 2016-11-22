#include <executor.h>


Executor::Executor(std::function<returnCode(int, std::vector<unsigned char>)> body)  : body(body), thread([this]() { executorBody(this->body); }) { }

Executor::~Executor() {
	post(COMMAND_SHUTDOWN);
	if (thread.joinable())
		thread.join();
};

std::future<returnCode> Executor::putMessage(int i, std::vector<unsigned char> params) {
	std::unique_lock<std::mutex> l(mutexQueue);

	struct message  m(i, std::move(params));
	auto future = m.promise.get_future();
	q.push(std::move(m));
	condition.notify_one();
	return future;
}

returnCode Executor::postSync(int i, std::vector<unsigned char> params) { return putMessage(i, params).get(); }

void Executor::post(int i, std::vector<unsigned char> params) { putMessage(i, params); }


struct Executor::message Executor::getMessage(void) {
	std::unique_lock<std::mutex> l(mutexQueue);

	condition.wait(l, [this]() { return !q.empty();});
	auto message = std::move(q.front());
	q.pop();
	return message;
}

void Executor::executorBody(std::function<returnCode(int, std::vector<unsigned char>)> body) {
	while (true) {
		struct Executor::message message(getMessage());
		if (COMMAND_SHUTDOWN == message.command) {
			message.promise.set_value(returnCode::shutdown);
			return;
		}
		switch (body(message.command, std::move(message.params))) {
		case returnCode::ok:
			message.promise.set_value(returnCode::ok);
			break;
		case returnCode::shutdown:
			message.promise.set_value(returnCode::shutdown);
			return;
		default:
			message.promise.set_value(returnCode::error);
			//notify parent.
			return;
		}
	}
}
