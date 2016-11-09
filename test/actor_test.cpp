#include <actor.h>
#include <proxyClient.h>
#include <proxyServer.h>
#include <actorRegistry.h>
#include <clientSocket.h>
#include <cstdlib>
#include <iostream>

#include <unistd.h>


static int basicActorTest(void) {
	std::cout << "basicActorTest" << std::endl;
	Actor a([](int i) { /* do something */ return actorReturnCode::ok; });
	sleep(2);
	auto val = a.postSync(1);
	if (actorReturnCode::ok != val) {
		std::cout << "post failure" << std::endl;
		return 1;
	}
	return 0;
}

void executeSeverProxy(uint16_t port) {
	Actor a([](int i) { /* do something */ return actorReturnCode::ok; });
	proxyServer server(a, ServerSocket::getConnection(port));
}


static int proxyTest(void) {
	std::cout << "proxyTest" << std::endl;
	static const uint16_t port = 4010;
	std::thread t(executeSeverProxy, port);
	sleep(2);
	proxyClient client(ClientSocket::openHostConnection("localhost", port));
	client.post(AbstractActor::COMMAND_SHUTDOWN);
	t.join();
	return 0;
}

static int proxyRestartTest(void) {
	std::cout << "proxyRestartTest" << std::endl;
	static const uint16_t port = 4003;
	static const int command = 0x33;
	std::thread t(executeSeverProxy, port);
	sleep(2);
	proxyClient client(ClientSocket::openHostConnection("localhost", port));
	int NbError = (actorReturnCode::ok == client.postSync(command)) ? 0 : 1;
	client.restart();
	NbError += (actorReturnCode::ok == client.postSync(command)) ? 0 : 1;
	NbError +=  (actorReturnCode::shutdown == client.postSync(AbstractActor::COMMAND_SHUTDOWN)) ? 0 : 1;
	t.join();
	return NbError;
}

static int registryConnectTest(void) {
	std::cout << "registryConnectTest" << std::endl;
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name"), port);
	sleep(1);
	Connection c = ClientSocket::openHostConnection("localhost", port);
//	c.writeString(std::string("dummy name"));
//
	return 0;
}

class ActorTest : public AbstractActor {
public:
	ActorTest() = default;
	virtual ~ActorTest() = default;
	virtual actorReturnCode postSync(int i) {return actorReturnCode::ok; };
	virtual void post(int i) {};
	virtual void restart(void) {};
};

static int registryAddActorTest(void) {
	std::cout << "registryAddAtorTest" << std::endl;
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name"), port);
	AbstractActor *a = new ActorTest();

	registry.registerActor("my actor", *a);
	return 0;
}

static int registryAddActorAndRemoveTest(void) {
	std::cout << "registryAddActorAndRemoveTest" << std::endl;
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name"), port);
	AbstractActor *a = new ActorTest();

	registry.registerActor("my actor", *a);
	registry.unregisterActor("my actor");
	try {
	    registry.unregisterActor("my actor");
	    return 1;
	} catch (std::runtime_error e) { }
	a = new ActorTest();
	registry.registerActor("my actor", *a);
	return 0;
}

static int registryAddReferenceTest(void) {
	std::cout << "registryAddReferenceTest" << std::endl;
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const uint16_t port1 = 6001;
	static const uint16_t port2 = 6002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);
	sleep(1);
	std::string name = registry1.addReference("localhost", port2);
	return name == name2 ? 0 : 1;
}

static int registeryAddActorAndFindItBackTest() {
	std::cout << "registeryAddActorAndFindItBackTest" << std::endl;

	static const std::string actorName("my actor");
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name1"), port);

	Actor *a = new Actor([](int i) { /* do something */ return actorReturnCode::ok; });
	registry.registerActor(std::string(actorName), *a);

	std::shared_ptr<AbstractActor> b = registry.getActor(actorName);
	return (a == b.get()) ? 0 : 1;
}

static int registeryFindUnknownActorTest() {
	std::cout << "registeryFindUnknownActorTest" << std::endl;

	static const std::string actorName("my actor");
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name1"), port);

	Actor *a = new Actor([](int i) { /* do something */ return actorReturnCode::ok; });
	registry.registerActor(std::string(actorName), *a);

	std::shared_ptr<AbstractActor> b = registry.getActor(std::string("wrong name"));
	return (nullptr == b.get()) ? 0 : 1;
}

static int findActorFromOtherRegistryTest() {
#if 1
	std::cout << "findActorFromOtherRegistryTest" << std::endl;
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string actorName("my actor");
	static const uint16_t port1 = 6001;
	static const uint16_t port2 = 6002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);
	sleep(1);
	std::string name = registry1.addReference("localhost", port2);
	Actor *a = new Actor([](int i) { /* do something */ return actorReturnCode::ok; });
	registry2.registerActor(actorName, *a);
	auto actor = registry2.getActor(actorName);
#endif
	return nullptr != actor.get() ? 0 : 1;
}

int main() {
	int nbFailure = basicActorTest();
	nbFailure += proxyTest();
	nbFailure += proxyRestartTest();
	nbFailure += registryConnectTest();
	nbFailure += registryAddActorTest();
	nbFailure += registryAddActorAndRemoveTest();
	nbFailure += registryAddReferenceTest();
	nbFailure += registeryAddActorAndFindItBackTest();
	nbFailure += registeryFindUnknownActorTest();
	nbFailure += findActorFromOtherRegistryTest();
	std::cout << ((nbFailure) ? "Failure" : "Success") << std::endl;
	return (nbFailure) ? EXIT_FAILURE : EXIT_SUCCESS;
}
