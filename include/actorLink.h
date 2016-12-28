#ifndef ACTOR_LINK_H__
#define ACTOR_LINK_H__

#include <messageQueue.h>

class LinkApi {
public:
	virtual ~LinkApi() = default;
	virtual StatusCode postSync(int code, std::vector<unsigned char> params = std::vector<unsigned char>()) = 0;
	virtual void post(int code, std::vector<unsigned char> params = std::vector<unsigned char>()) = 0;
	virtual std::string getName(void) const = 0;
	virtual LinkApi *getActorLink() = 0;
protected:
	LinkApi() = default;
};

using GenericActorPtr = std::shared_ptr<LinkApi>;


class ActorLink : public LinkApi {
public:
	ActorLink(std::string name, std::shared_ptr<MessageQueue> queue) : name(name), queue(std::move(queue)) {}
	~ActorLink() = default;
	StatusCode postSync(int code, std::vector<unsigned char> params = std::vector<unsigned char>()) { return queue->postSync(MessageType::COMMAND_MESSAGE, code, params); }
	void post(int code, std::vector<unsigned char> params = std::vector<unsigned char>()) { queue->post(MessageType::COMMAND_MESSAGE, code, params); };
	std::string getName(void) const { return name; }
	LinkApi *getActorLink() { return this; }
private:
	const std::string name;
	std::shared_ptr<MessageQueue> queue;
};



#endif
