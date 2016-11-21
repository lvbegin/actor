#include <proxyContainer.h>

#include <tuple>

std::atomic<int> ProxyContainer::proxyId { 0 };

ProxyContainer::ProxyContainer() : executor( [this](int id, const std::vector<unsigned char> &not_used) { return (this->deleteProxy(id), returnCode::ok);}) {}

ProxyContainer::~ProxyContainer() = default;

int ProxyContainer::newproxyId(void) { return proxyId++; }

void ProxyContainer::createNewProxy(std::shared_ptr<AbstractActor> actor, Connection connection) {
	const auto id = newproxyId();
	const auto terminate = [this, id]() { this->executor.post(id); };
	proxies.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(actor, std::move(connection), terminate));
}

void ProxyContainer::deleteProxy(int i) { proxies.erase(i); }
