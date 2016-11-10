#include <actorRegistry.h>
#include <clientSocket.h>
#include <serverSocket.h>
#include <functional>

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body);

ActorRegistry::ActorRegistry(std::string name, uint16_t port) : name(name),
					t([this, port]() {  threadBody(port, [this](ServerSocket &s) { registryBody(s); }); }),
					terminated(false) { }

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body) {
	auto s = std::make_unique<ServerSocket>(port);
	body(*s);
}

ActorRegistry::~ActorRegistry() {
	terminated = true;
	t.join();
}

#include <iostream>
void ActorRegistry::registryBody(ServerSocket &s) {
	while (!terminated) {
		try {
			auto connection = s.acceptOneConnection();
			auto otherName = connection.readString();
			connection.writeString(this->name);
			//others.insert(std::move(name), std::move(connection));
			//this is for simplification
			auto actorName = connection.readString();
			try {
				std::cout << "received search request" << std::endl;
				auto actor = actors.find(actorName).get();
				std::cout << "actor found" << std::endl;
				connection.writeInt(1);
				std::cout << "push_back" << std::endl;
				proxyServer(*actor, std::move(connection));
//				proxies.push_back(proxyServer(*actor, std::move(connection))); //ok and now when to remove ?
				std::cout << "done" << std::endl;
			} catch (std::out_of_range e) {
				connection.writeInt(0);
				continue;
			}
		}
		catch (std::exception e) { }
	}
	return ;
}

std::string ActorRegistry::addReference(std::string host, uint16_t port) {
	auto connection = ClientSocket::openHostConnection(host, port);
	connection.writeString(name);
	//should read status...and react in consequence: what if failure returned?
	std::string otherName = connection.readString();
	others.insert(otherName, std::move(connection));
	return otherName;
}

void ActorRegistry::removeReference(std::string registryName) { others.erase(registryName); }

void ActorRegistry::registerActor(std::string name, AbstractActor &actor) {
	actors.insert(name, std::move(std::shared_ptr<AbstractActor>(&actor)));
}

void ActorRegistry::unregisterActor(std::string name) { actors.erase(name); }

#include <proxyClient.h>
actorPtr  ActorRegistry::getActor(std::string name) {
	try {
		return actors.find(name);
	} catch (std::out_of_range e) {
		actorPtr actor;
		std::cout << "start search" << std::endl;
		others.for_each([&actor, &name](std::pair<const std::string, Connection> &c) {
			std::cout << "yet anoterh registry" << std::endl;
			c.second.writeString(name);
			std::cout << "name sent" << std::endl;
			if (1 == c.second.readInt())
			{
				std::cout << "ok, found" << std::endl;
				actor.reset(new proxyClient(std::move(c.second)));
				std::cout << "ok, proxy Client created" << std::endl;
			}
		});
		return actor;
	}
}
