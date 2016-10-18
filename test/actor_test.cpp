#include <actor.h>
#include <proxyClient.h>
#include <proxyServer.h>

#include <cstdlib>
#include <iostream>

#include <unistd.h>


static int basicActorTest(void) {
	Actor a([](int i) { /* do something */ return Actor::actorReturnCode::ok; });
	sleep(2);
	auto val = a.postSync(1);
	if (Actor::actorReturnCode::ok != val) {
		std::cout << "post failure" << std::endl;
		return 1;
	}
	a.post(Actor::COMMAND_SHUTDOWN);
	return 0;
}

void executeSeverProxy(uint16_t port) {
	Actor a([](int i) { /* do something */ return Actor::actorReturnCode::ok; });
	proxyServer server;
	server.start(a, port);
}

static int proxyTest(void) {
	static const uint16_t port = 4005;
	std::thread t(executeSeverProxy, port);
	sleep(2);
	proxyClient client;
	client.start("localhost", port);
	client.post(abstractActor::COMMAND_SHUTDOWN);
	t.join();
	return 0;
}


static int proxyRestartTest(void) {
	static const uint16_t port = 4003;
	static const int command = 0x33;
	Actor a([](int i) { /* do something */ return Actor::actorReturnCode::ok; });
	proxyServer server;
	server.start(a, port);
	sleep(2);
	proxyClient client;
	client.start("localhost", port);
	int NbError = (abstractActor::actorReturnCode::ok == client.postSync(command)) ? 0 : 1;
	client.restart();
	NbError += (abstractActor::actorReturnCode::ok == client.postSync(command)) ? 0 : 1;
	NbError +=  (abstractActor::actorReturnCode::shutdown == client.postSync(abstractActor::COMMAND_SHUTDOWN)) ? 0 : 1;
	return NbError;
}


int main() {
	int nbFailure = basicActorTest();
	nbFailure += proxyTest();
	nbFailure += proxyRestartTest();
	return (nbFailure) ? EXIT_FAILURE : EXIT_SUCCESS;
}
