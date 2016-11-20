#ifndef ACTOR_H__
#define ACTOR_H__

#include <AbstractActor.h>
#include <executor.h>

#include <functional>
#include <memory>

class Actor : public AbstractActor {
public:
	Actor(std::function<returnCode(int)> body);
	~Actor();

	Actor(const Actor &a) = delete;
	Actor &operator=(const Actor &a) = delete;
	returnCode postSync(int i);
	void post(int i);
	void restart(void);
private:
	std::function<returnCode(int)> body;
	std::unique_ptr<Executor> executor;
	void stopThread(void);
//	struct message {
//		int command;
//		std::promise<actorReturnCode> promise;
//		message(int c) : command(c) {}
//	};
//
//	std::future<actorReturnCode> putMessage(int i);
//	void actorBody(std::function<actorReturnCode(int)> body);
//	message getMessage(void);
//	std::mutex mutexQueue;
//	std::condition_variable condition;
//	std::queue<message> q;
//	std::thread thread;
//
};

#endif
