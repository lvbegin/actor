#ifndef ACTOR_API_H__
#define ACTOR_API_H__

#include <rc.h>

#include <vector>
#include <memory>

class LinkApi {
public:
	virtual StatusCode postSync(int code, std::vector<unsigned char> params = std::vector<unsigned char>()) = 0;
	virtual void post(int code, std::vector<unsigned char> params = std::vector<unsigned char>()) = 0;
protected:
	LinkApi() = default;
	virtual ~LinkApi() = default;
};

using ActorLink = std::shared_ptr<LinkApi>;

#endif
