#ifndef EXECUTOR_H__
#define EXECUTOR_H__

#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <queue>

#include <rc.h>

class Executor {
public:
	Executor(std::function<returnCode(int, std::vector<unsigned char>)> body);
	~Executor();

	Executor(const Executor &a) = delete;
	Executor &operator=(const Executor &a) = delete;
	returnCode postSync(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	void post(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	static const uint32_t COMMAND_SHUTDOWN = 0;
private:
	struct message {
		int command;
		std::vector<unsigned char> params;
		std::promise<returnCode> promise;
		message(int c, std::vector<unsigned char> params) : command(c), params(params) {}
	};

	std::future<returnCode> putMessage(int i, std::vector<unsigned char> params);
	void executorBody(std::function<returnCode(int, std::vector<unsigned char>)> body);
	message getMessage(void);
	std::mutex mutexQueue;
	std::condition_variable condition;
	std::function<returnCode(int, std::vector<unsigned char>)> body; //why needed ?
	std::queue<message> q;
	std::thread thread;

};


#endif
