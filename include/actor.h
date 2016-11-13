#ifndef ACTOR_H__
#define ACTOR_H__

#include <AbstractActor.h>
#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <queue>

class Actor : public AbstractActor {
public:
	Actor(std::function<actorReturnCode(int)> body);
	~Actor();

	Actor(const Actor &a) = delete;
	Actor &operator=(const Actor &a) = delete;
	actorReturnCode postSync(int i);
	void post(int i);
	void restart(void);
private:
	struct message {
		int command;
		std::promise<actorReturnCode> promise;
		message(int c) : command(c) {}
	};

	std::future<actorReturnCode> putMessage(int i);
	void actorBody(std::function<actorReturnCode(int)> body);
	message getMessage(void);
	void stopThread(void);
	std::mutex mutexQueue;
	std::condition_variable condition;
	std::function<actorReturnCode(int)> body;
	std::queue<message> q;
	std::thread thread;

};

#endif
