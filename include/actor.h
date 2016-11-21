#ifndef ACTOR_H__
#define ACTOR_H__

#include <AbstractActor.h>
#include <executor.h>

#include <functional>
#include <memory>

class Actor : public AbstractActor {
public:
	Actor(std::function<returnCode(int, const std::vector<unsigned char> &)> body);
	~Actor();

	Actor(const Actor &a) = delete;
	Actor &operator=(const Actor &a) = delete;
	returnCode postSync(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	void post(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	void restart(void);
private:
	std::function<returnCode(int, const std::vector<unsigned char> &)> body;
	std::unique_ptr<Executor> executor;
};

#endif
