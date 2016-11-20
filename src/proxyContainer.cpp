#include <proxyContainer.h>

#include <tuple>

std::atomic<int> ProxyContainer::proxyId { 0 };

ProxyContainer::ProxyContainer() : executor( [this](int command) -> returnCode { return this->containerBody(command); } ) {}

ProxyContainer::~ProxyContainer() = default;

int ProxyContainer::newproxyId(void) { return proxyId++; }

void ProxyContainer::createNewProxy(std::shared_ptr<AbstractActor> actor, Connection connection) {
	const auto id = newproxyId();
	const auto terminate = [this, id]() { this->executor.post(id); };
	proxies.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(actor, std::move(connection), terminate));
}

void ProxyContainer::deleteProxy(int i) { proxies.erase(i); }

returnCode ProxyContainer::containerBody(int i)
{
	deleteProxy(i);
	return returnCode::ok;
}
