/* Copyright 2016 Laurent Van Begin
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <actor.h>
#include <proxyClient.h>
#include <proxyServer.h>
#include <actorRegistry.h>
#include <clientSocket.h>
#include <command.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>


static int basicActorTest(void) {
	static const uint32_t command = 0xaa;
	const Actor a([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	const auto val = a.postSync(command);
	if (StatusCode::ok != val) {
		std::cout << "post failure" << std::endl;
		return 1;
	}
	a.postSync(Command::COMMAND_SHUTDOWN);
	return 0;
}

static int basicActorWithParamsTest(void) {
	static const std::string paramValue("Hello World");
	const RawData params(paramValue.begin(), paramValue.end());

	const Actor a([](int i, const std::vector<unsigned char> &params, const ActorLink &) {
				if (0 == paramValue.compare(std::string(params.begin(), params.end())))
					return StatusCode::ok;
				else
					return StatusCode::error;});
	const auto val = a.postSync(1, params);
	if (StatusCode::ok != val) {
		std::cout << "post failure" << std::endl;
		return 1;
	}

	return 0;
}

static int actorSendMessageAndReceiveAnAnswerTest(void) {
	static const std::string paramValue("Hello World");
	const RawData params(paramValue.begin(), paramValue.end());

	const Actor a([](int i, const std::vector<unsigned char> &params, const ActorLink &sender) {
				if (0 == paramValue.compare(std::string(params.begin(), params.end())))
					sender->post(0x00);
				else
					sender->post(0x01);
				return StatusCode::ok;
	});
	auto queue = std::make_shared<MessageQueue>();
	a.post(1, params, queue);
	const auto answer = queue->get();
	return (0x00 == answer.code) ? 0 : 1;
}

static void executeSeverProxy(uint16_t port, int *nbMessages) {
	const Actor actor([nbMessages](int i, const RawData &, const ActorLink &) { (*nbMessages)++; return StatusCode::ok; });
	const auto doNothing = []() { };
	const proxyServer server(actor.getActorLinkRef(), ServerSocket::getConnection(port), doNothing);
}

static Connection openOneConnection(uint16_t port) {
	while (true) {
		try {
			return ClientSocket::openHostConnection("localhost", port);
		} catch (std::exception &e) { }
	}
}

static int proxyTest(void) {
	static const uint16_t port = 4011;
	static const uint32_t code = 0x33;
	int nbMessages { 0 };
	std::thread t(executeSeverProxy, port, &nbMessages);
	proxyClient client(openOneConnection(port));
	client.post(code);
	client.postSync(Command::COMMAND_SHUTDOWN);
	t.join();
	return (1 == nbMessages) ? 0 : 1;
}

static int registryConnectTest(void) {
	static const uint16_t port = 4001;
	const ActorRegistry registry(std::string("name"), port);
	const Connection c = openOneConnection(port);
	return 0;
}

static int registryAddActorTest(void) {
	static const uint16_t port = 4001;
	static const std::string actorName("my actor");
	const Actor a([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name"), port);

	registry.registerActor(actorName, a.getActorLinkRef());

	return 0;
}

static int registryAddActorAndRemoveTest(void) {
	static const uint16_t port = 4001;
	static const std::string actorName("my actor");
	const Actor a([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name"), port);

	registry.registerActor(actorName, a.getActorLinkRef());
	registry.unregisterActor(actorName);
	try {
	    registry.unregisterActor(actorName);
	    return 1;
	} catch (std::runtime_error &e) { }

	const Actor anotherA([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	registry.registerActor(actorName, anotherA.getActorLinkRef());
	return 0;
}

static void ensureRegistryStarted(uint16_t port) { openOneConnection(port); }

static int registryAddReferenceTest(void) {
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const uint16_t port1 = 4001;
	static const uint16_t port2 = 4002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);

	ensureRegistryStarted(port1);
	ensureRegistryStarted(port2);
	const std::string name = registry1.addReference("localhost", port2);
	return name == name2 ? 0 : 1;
}

static int registryAddReferenceOverrideExistingOneTest(void) {
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const uint16_t port1 = 4001;
	static const uint16_t port2 = 4002;
	static const uint16_t port3 = 6003;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);
	ActorRegistry registry3(name1, port3);

	ensureRegistryStarted(port1);
	ensureRegistryStarted(port2);
	registry1.addReference("localhost", port2);
	const std::string name = registry3.addReference("localhost", port2);
	return name == name2 ? 0 : 1;
}

static int registeryAddActorAndFindItBackTest() {
	static const std::string actorName("my actor");
	static const uint16_t port = 4001;
	const Actor a([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	const auto actorRefLink = a.getActorLinkRef();
	ActorRegistry registry(std::string("name1"), port);
	registry.registerActor(actorName, actorRefLink);

	const auto b = registry.getActor(actorName);

	return (actorRefLink.get() == b.get()) ? 0 : 1;
}

static int registeryFindUnknownActorTest() {
	static const std::string actorName("my actor");
	static const uint16_t port = 4001;
	const Actor a([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name1"), port);
	registry.registerActor(actorName, a.getActorLinkRef());

	const ActorLink b = registry.getActor(std::string("wrong name"));
	return (nullptr == b.get()) ? 0 : 1;
}

static int findActorFromOtherRegistryTest() {
	static const uint32_t dummyCommand = 0x33;
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string actorName("my actor");
	static const uint16_t port1 = 4001;
	static const uint16_t port2 = 4002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);

	ensureRegistryStarted(port1);
	ensureRegistryStarted(port2);
	const std::string name = registry1.addReference("localhost", port2);
	if (name2.compare(name))
		return 1;
	const Actor a([](int i, const std::vector<unsigned char> &params, const ActorLink &) {
		if (i == dummyCommand && 0 == params.size())
			return StatusCode::ok;
		else
			return StatusCode::error;} );

	registry2.registerActor(actorName, a.getActorLinkRef());
	const auto actor = registry1.getActor(actorName);
	if (nullptr == actor.get())
		return 1;
	sleep(10); // ensure that the proxy server does not stop after a timeout when no command is sent.
	return (StatusCode::ok == actor->postSync(dummyCommand)) ? 0 : 1;
}

static int findActorFromOtherRegistryAndSendCommandWithParamsTest() {
	static const uint32_t dummyCommand = 0x33;
	static const std::string paramValue("Hello World");
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string actorName("my actor");
	static const uint16_t port1 = 4001;
	static const uint16_t port2 = 4002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);

	ensureRegistryStarted(port1);
	ensureRegistryStarted(port2);
	const std::string name = registry1.addReference("localhost", port2);
	if (name2.compare(name))
		return 1;
	const Actor a([](int i, const std::vector<unsigned char> &params, const ActorLink &) {
		if (i == dummyCommand && 0 == paramValue.compare(std::string(params.begin(), params.end())))
							return StatusCode::ok;
						else
							return StatusCode::error;} );
	registry2.registerActor(actorName, a.getActorLinkRef());
	const auto actor = registry1.getActor(actorName);
	if (StatusCode::ok != actor->postSync(dummyCommand, std::vector<unsigned char>(paramValue.begin(), paramValue.end())))
		return 1;
	actor->postSync(Command::COMMAND_SHUTDOWN);
	return nullptr != actor.get() ? 0 : 1;
}


static int findUnknownActorInMultipleRegistryTest() {
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string actorName("my actor");
	static const uint16_t port1 = 4001;
	static const uint16_t port2 = 4002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);

	ensureRegistryStarted(port1);
	ensureRegistryStarted(port2);
	const std::string name = registry1.addReference("localhost", port2);
	if (name2.compare(name))
		return 1;
	const Actor a([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	registry2.registerActor(actorName, a.getActorLinkRef());
	const auto actor = registry1.getActor("unknown actor");
	return nullptr == actor.get() ? 0 : 1;
}

static int initSupervisionTest() {
	Actor supervisor([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	Actor supervised([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	supervisor.registerActor(supervised);
	supervisor.unregisterActor(supervised);

	supervisor.post(Command::COMMAND_SHUTDOWN);
	supervised.post(Command::COMMAND_SHUTDOWN);

	return 0;
}


static int unregisterToSupervisorWhenActorDestroyedTest() {
	Actor supervisor([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	auto supervised = std::make_unique<Actor>([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	supervisor.registerActor(*supervised.get());

	supervised->post(Command::COMMAND_SHUTDOWN);
	supervised.reset();
	supervisor.post(Command::COMMAND_SHUTDOWN);

	return 0;
}

static int supervisorRestartsActorTest() {
	static bool exceptionThrown = false;
	static int restartCommand = 0x99;
	static int otherCommand = 0xaa;
	Actor supervisor([](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	Actor supervised([](int i, const RawData &, const ActorLink &) {
		if (i == restartCommand) {
			exceptionThrown = true;
			throw std::runtime_error("some error");
		}
		 return StatusCode::ok;
	 });
	supervisor.registerActor(supervised);
	if (StatusCode::error != supervised.postSync(restartCommand))
		std::cout << "error not returned" << std::endl;
	if (StatusCode::ok != supervised.postSync(otherCommand))
		std::cout << "other command not ok" << std::endl;

	supervisor.post(Command::COMMAND_SHUTDOWN);
	supervised.post(Command::COMMAND_SHUTDOWN);

	return (exceptionThrown) ? 0 : 1;
}

static int actorNotifiesErrorToSupervisorTest() {
	static int someCommand = 0xaa;
	bool supervisorRestarted = false;
	bool supervised1Restarted = false;
	bool supervised2Restarted = false;
	Actor supervisor(
			[](int i, const RawData &, const ActorLink &) { return StatusCode::ok; },
			[&supervisorRestarted](void) { supervisorRestarted = true; } );
	Actor supervised1(
			[](int i, const RawData &, const ActorLink &) {
				Actor::notifyError(0x69);
				return StatusCode::ok;
	 	 	 },
			 [&supervised1Restarted](void) { supervised1Restarted = true; } );
	Actor supervised2(
			[](int i, const RawData &, const ActorLink &) { return StatusCode::ok; },
			[&supervised2Restarted](void) { supervised2Restarted = true; } );

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	if (StatusCode::error != supervised1.postSync(someCommand))
		return 1;
	if (StatusCode::error != supervised1.postSync(someCommand))
		return 1;

	for (int i = 0; i < 5 && !supervised1Restarted; i++)
		sleep(1);
	if (supervisorRestarted || !supervised1Restarted || supervised2Restarted)
		return 1;

	supervisor.post(Command::COMMAND_SHUTDOWN);
	supervised1.post(Command::COMMAND_SHUTDOWN);
	supervised2.post(Command::COMMAND_SHUTDOWN);
	return 0;
}

static int actorDoesNothingIfNoSupervisorTest() {
	static int someCommand = 0xaa;
	Actor supervised([](int i, const RawData &, const ActorLink &) {
		Actor::notifyError(0x69);
		return StatusCode::ok;
	 });
	if (StatusCode::error != supervised.postSync(someCommand))
		return 1;
	if (StatusCode::error != supervised.postSync(someCommand))
		return 1;

	supervised.post(Command::COMMAND_SHUTDOWN);

	return 0;
}

static int actorDoesNothingIfNoSupervisorAndExceptionThrownTest() {
	static int someCommand = 0xaa;
	Actor supervised([](int i, const RawData &, const ActorLink &) {
		throw std::runtime_error("some error");
		return StatusCode::ok; //remove that ?
	 });
	if (StatusCode::error != supervised.postSync(someCommand))
		return 1;
	if (StatusCode::error != supervised.postSync(someCommand))
		return 1;

	supervised.post(Command::COMMAND_SHUTDOWN);

	return 0;
}

static int restartAllActorBySupervisorTest() {
	static int someCommand = 0xaa;
	bool supervisorRestarted = false;
	bool supervised1Restarted = false;
	bool supervised2Restarted = false;
	Actor supervisor(
			[](int i, const RawData &, const ActorLink &) { return StatusCode::ok; },
			[&supervisorRestarted](void) { supervisorRestarted = true; },
			[](void) { return RestartType::RESTART_ALL; });
	Actor supervised1(
			[](int i, const RawData &, const ActorLink &) {
				Actor::notifyError(0x69);
				return StatusCode::ok;
	 	 	 },
			 [&supervised1Restarted](void) { supervised1Restarted = true; });
	Actor supervised2(
			[](int i, const RawData &, const ActorLink &) { return StatusCode::ok; },
			[&supervised2Restarted](void) { supervised2Restarted = true; });

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	if (StatusCode::error != supervised1.postSync(someCommand))
		return 1;
	if (StatusCode::error != supervised1.postSync(someCommand))
		return 1;
	if (StatusCode::ok != supervised2.postSync(someCommand))
		return 1;
	for (int i = 0; i < 5 && !supervised1Restarted && !supervised2Restarted; i++)
		sleep(1);

	if (supervisorRestarted || !supervised1Restarted || !supervised2Restarted)
		return 1;

	supervisor.post(Command::COMMAND_SHUTDOWN);
	supervised1.post(Command::COMMAND_SHUTDOWN);
	supervised2.post(Command::COMMAND_SHUTDOWN);

	return 0;
}

static int executorTest() {
	MessageQueue messageQueue;
	Executor executor([](MessageType, int, const RawData &, const ActorLink &) { return StatusCode::shutdown; }, messageQueue);
	messageQueue.post(MessageType::COMMAND_MESSAGE, Command::COMMAND_SHUTDOWN);
	return 0;
}

static int serializationTest() {
	static const uint32_t expected_value = 0x11223344;

	const auto s = UniqueId::serialize(expected_value);
	return (expected_value == UniqueId::unserialize(s)) ? 0 : 1;
}

struct _test{
	const char *name;
	int (*test)(void);
	_test(const char *name, int (*test)(void)) : name(name), test(test) { }
};

#define TEST(t) _test(#t, t)

#define runTest(suite) _runTest(suite, sizeof(suite)/sizeof(_test))

int _runTest(_test *suite, size_t nbTests) {
	int nbFailure = 0;

	for (size_t i = 0; i < nbTests; i++) {
		std::cout << suite[i].name << ": ";
		if (0 < suite[i].test()) {
			nbFailure ++;
			std::cout << "FAILURE" << std::endl;
		}
		else {
			std::cout << "OK" << std::endl;
		}
	}
	return nbFailure;
}

int main() {
	_test suite[] = {
			TEST(basicActorTest),
			TEST(basicActorWithParamsTest),
			TEST(proxyTest),
			TEST(actorSendMessageAndReceiveAnAnswerTest),
			TEST(registryConnectTest),
			TEST(registryAddActorTest),
			TEST(registryAddActorAndRemoveTest),
			TEST(registryAddReferenceTest),
			TEST(registryAddReferenceOverrideExistingOneTest),
			TEST(registeryAddActorAndFindItBackTest),
			TEST(registeryFindUnknownActorTest),
			TEST(findActorFromOtherRegistryTest),
			TEST(findActorFromOtherRegistryAndSendCommandWithParamsTest),
			TEST(findUnknownActorInMultipleRegistryTest),
			TEST(initSupervisionTest),
			TEST(unregisterToSupervisorWhenActorDestroyedTest),
			TEST(supervisorRestartsActorTest),
		 	TEST(actorNotifiesErrorToSupervisorTest),
			TEST(actorDoesNothingIfNoSupervisorTest),
			TEST(actorDoesNothingIfNoSupervisorAndExceptionThrownTest),
			TEST(restartAllActorBySupervisorTest),
			TEST(executorTest),
			TEST(serializationTest),
	};

	const auto nbFailure = runTest(suite);
	if (nbFailure > 0)
		return (std::cout << nbFailure << " tests failed." << std::endl, EXIT_FAILURE);
	else
		return (std::cout << "Success!" << std::endl, EXIT_SUCCESS);
}
