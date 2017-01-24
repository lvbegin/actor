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
#include <proxyServer.h>
#include <actorRegistry.h>
#include <clientSocket.h>
#include <commandValue.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "../include/proxyClient.h"


static int basicActorTest(void) {
	static const uint32_t command = 0xaa;
	static const int answer = 0x22;
	const auto link = std::make_shared<MessageQueue>("queue for reply");
	const Actor a("actor name", [](int i, const RawData &, const ActorLink &link) { link->post(answer); return StatusCode::ok; });
	a.post(command, link);
	return (answer == link->get().code) ? 0 : 1;
}

static int basicActorWithParamsTest(void) {
	static const std::string paramValue("Hello World");
	const RawData params(paramValue.begin(), paramValue.end());
	static const int answer = 0x22;
	static const int badAnswer = 0x88;
	const auto link = std::make_shared<MessageQueue>("queue for reply");

	const Actor a("actor name", [](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
				if (0 == paramValue.compare(std::string(params.begin(), params.end()))) {
					link->post(answer);
					return StatusCode::ok;
				}
				else {
					link->post(badAnswer);
					return StatusCode::error;
				}
	});
	a.post(1, params, link);
	return (answer == link->get().code) ? 0 : 1;
}

static int actorSendMessageAndReceiveAnAnswerTest(void) {
	static const std::string paramValue("Hello World");
	const RawData params(paramValue.begin(), paramValue.end());

	const Actor a("actor name", [](int i, const std::vector<unsigned char> &params, const ActorLink &sender) {
				if (0 == paramValue.compare(std::string(params.begin(), params.end())))
					sender->post(0x00);
				else
					sender->post(0x01);
				return StatusCode::ok;
	});
	auto queue = std::make_shared<MessageQueue>("queue for reply");
	a.post(1, params, queue);
	return (0x00 == queue->get().code) ? 0 : 1;
}

static void executeSeverProxy(uint16_t port, int *nbMessages) {
	const Actor actor("actor name", [nbMessages](int i, const RawData &, const ActorLink &) { (*nbMessages)++; return StatusCode::ok; });
	const auto doNothing = []() { };
	const auto DummyGetConnection = [] (std::string) { return ActorLink(); };
	const proxyServer server(actor.getActorLinkRef(), ServerSocket::getConnection(port), doNothing, DummyGetConnection);
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
	ProxyClient client("client name", openOneConnection(port));
	client.post(code);
	client.post(CommandValue::SHUTDOWN);
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
	const Actor a(actorName, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name"), port);

	registry.registerActor(a.getActorLinkRef());

	return 0;
}

static int registryAddActorAndRemoveTest(void) {
	static const uint16_t port = 4001;
	static const std::string actorName("my actor");
	const Actor a(actorName, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name"), port);

	registry.registerActor(a.getActorLinkRef());
	registry.unregisterActor(actorName);
	try {
	    registry.unregisterActor(actorName);
	    return 1;
	} catch (std::out_of_range &e) { }

	const Actor anotherA(actorName, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	registry.registerActor(anotherA.getActorLinkRef());
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

static int registeryAddActorFindItBackAndSendMessageTest() {
	static const int code = 0x11;
	static const int answer = 0x33;
	static const int badAnswer = 0x33;
	static const std::string actorName("my actor");
	static const uint16_t port = 4001;
	auto link = std::make_shared<MessageQueue>("queue for reply");
	const Actor a(actorName, [](int i, const RawData &, const ActorLink &link) {
		if (code == i)
			link->post(answer);
		else
			link->post(badAnswer);
		return StatusCode::ok;
	});
	const auto actorRefLink = a.getActorLinkRef();
	ActorRegistry registry("registry name", port);
	registry.registerActor(actorRefLink);

	const auto b = registry.getActor(actorName);

	if (actorRefLink.get() != b.get())
		return 1;
	b->post(code, link);
	if (answer != link->get().code)
		return 1;
	return 0;
}

static int registeryFindUnknownActorTest() {
	static const std::string actorName("my actor");
	static const uint16_t port = 4001;
	const Actor a(actorName, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name1"), port);
	registry.registerActor(a.getActorLinkRef());

	const ActorLink b = registry.getActor(std::string("wrong name"));
	return (nullptr == b.get()) ? 0 : 1;
}

static int findActorFromOtherRegistryAndSendMessageTest() {
	static const uint32_t dummyCommand = 0x33;
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string actorName("my actor");
	static const uint16_t port1 = 4001;
	static const uint16_t port2 = 4002;
	static const int answer = 0x99;
	static const int badAnswer = 0x11;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);
	auto link = std::make_shared<MessageQueue>("dummy name");
	bool rc = false;
	ensureRegistryStarted(port1);
	ensureRegistryStarted(port2);
	const std::string name = registry1.addReference("localhost", port2);
	if (name2.compare(name))
		return 1;
	const Actor a(actorName, [&rc](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
		if (i == dummyCommand && 0 == params.size()) {
			rc = true;
			link->post(answer);
			return StatusCode::ok;
		}
		else {
			rc = false;
			link->post(badAnswer);
			return StatusCode::error;
		}
	} );

	registry1.registerActor(link);
	registry2.registerActor(a.getActorLinkRef());
	const auto actor = registry1.getActor(actorName);
	if (nullptr == actor.get())
		return 1;
	sleep(10); // ensure that the proxy server does not stop after a timeout when no command is sent.
	actor->post(dummyCommand, link);
	return (answer == link->get().code) ? 0 : 1;
}

static int findActorFromOtherRegistryAndSendWithSenderForwardToAnotherActorMessageTest() {
	static const uint32_t dummyCommand = 0x33;
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string name3("name3");
	static const std::string actorName1("my actor 1");
	static const std::string actorName2("my actor 2");
	static const uint16_t port1 = 4001;
	static const uint16_t port2 = 4002;
	static const uint16_t port3 = 4003;
	static const int answer = 0x99;
	static const int intermedieateCommand = 0x44;
	static const int badAnswer = 0x11;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);
	ActorRegistry registry3(name3, port3);
	auto link = std::make_shared<MessageQueue>("dummy name");
	ensureRegistryStarted(port1);
	ensureRegistryStarted(port2);
	ensureRegistryStarted(port3);

	registry1.addReference("localhost", port2);
	registry1.addReference("localhost", port3);
	registry2.addReference("localhost", port3);

	const Actor actor2(actorName2, [](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
		if (i == intermedieateCommand && 0 == params.size()) {
			link->post(answer);
			return StatusCode::ok;
		}
		else {
			link->post(badAnswer);
			return StatusCode::error;
		}
	} );
	registry3.registerActor(actor2.getActorLinkRef());

	auto linkActor2 = registry3.getActor(actorName2);

	const Actor actor1(actorName1, [linkActor2](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
		if (i == dummyCommand && 0 == params.size()) {
			linkActor2->post(intermedieateCommand, link);
			return StatusCode::ok;
		}
		else {
			link->post(badAnswer);
			return StatusCode::error;
		}
	} );
	registry2.registerActor(actor1.getActorLinkRef());
	registry1.registerActor(link);
	const auto actor = registry2.getActor(actorName1);
	if (nullptr == actor.get())
		return 1;
	actor->post(dummyCommand, link);
	return (answer == link->get().code) ? 0 : 1;
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
	bool rc = false;

	ensureRegistryStarted(port1);
	ensureRegistryStarted(port2);
	const std::string name = registry1.addReference("localhost", port2);
	if (name2.compare(name))
		return 1;
	const Actor a(actorName, [&rc](int i, const std::vector<unsigned char> &params, const ActorLink &) {
		if (i == dummyCommand && 0 == paramValue.compare(std::string(params.begin(), params.end()))) {
			rc = true;
			return StatusCode::ok;
		}
		else {
			rc = false;
			return StatusCode::error;
		}
	} );
	registry2.registerActor(a.getActorLinkRef());
	const auto actor = registry1.getActor(actorName);
	if (nullptr == actor.get())
		return 1;
	actor->post(dummyCommand, std::vector<unsigned char>(paramValue.begin(), paramValue.end()));
	sleep(1); //to be removed once the reply for remote is implemented;
	return rc ? 0 : 1;
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
	const Actor a(actorName, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	registry2.registerActor(a.getActorLinkRef());
	const auto actor = registry1.getActor("unknown actor");
	return nullptr == actor.get() ? 0 : 1;
}

static int initSupervisionTest() {
	Actor supervisor("supervisor", [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	Actor supervised("supervised", [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	supervisor.registerActor(supervised);
	supervisor.unregisterActor(supervised);

	supervisor.post(CommandValue::SHUTDOWN);
	supervised.post(CommandValue::SHUTDOWN);

	return 0;
}


static int unregisterToSupervisorWhenActorDestroyedTest() {
	Actor supervisor("supervisor", [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	auto supervised = std::make_unique<Actor>("supervised", [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	supervisor.registerActor(*supervised.get());

	supervised->post(CommandValue::SHUTDOWN);
	supervised.reset();
	supervisor.post(CommandValue::SHUTDOWN);

	return 0;
}

static int supervisorRestartsActorTest() {
	static bool exceptionThrown = false;
	static int restartCommand = 0x99;
	static int otherCommand = 0xaa;
	static const int answer = 0x99;
	const auto link = std::make_shared<MessageQueue>("queue for reply");
	Actor supervisor("supervisor", [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	Actor supervised("supervised", [](int i, const RawData &, const ActorLink &link) {
		if (i == restartCommand) {
			exceptionThrown = true;
			throw std::runtime_error("some error");
		}
		link->post(answer);
		 return StatusCode::ok;
	 });
	supervisor.registerActor(supervised);
	supervised.post(restartCommand, link);
	supervised.post(otherCommand, link);
	if (answer != link->get().code)
		return 1;
	supervisor.post(CommandValue::SHUTDOWN);
	supervised.post(CommandValue::SHUTDOWN);

	return (exceptionThrown) ? 0 : 1;
}

static int actorNotifiesErrorToSupervisorTest() {
	static int someCommand = 0xaa;
	bool supervisorRestarted = false;
	int supervised1Restarted = 0;
	bool supervised2Restarted = false;
	Actor supervisor("supervisor",
			[](int i, const RawData &, const ActorLink &) { return StatusCode::ok; },
			[&supervisorRestarted](void) { supervisorRestarted = true; } );
	Actor supervised1("supervised1",
			[](int i, const RawData &, const ActorLink &) {
				Actor::notifyError(0x69);
				return StatusCode::ok;
	 	 	 },
			 [&supervised1Restarted](void) { supervised1Restarted++; } );
	Actor supervised2("supervised2",
			[](int i, const RawData &, const ActorLink &) { return StatusCode::ok; },
			[&supervised2Restarted](void) { supervised2Restarted = true; } );

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	supervised1.post(someCommand);
	supervised1.post(someCommand);

	for (int i = 0; i < 5 && !supervised1Restarted; i++)
		sleep(1);
	if (supervisorRestarted || ! (2 == supervised1Restarted) || supervised2Restarted)
		return 1;

	supervisor.post(CommandValue::SHUTDOWN);
	supervised1.post(CommandValue::SHUTDOWN);
	supervised2.post(CommandValue::SHUTDOWN);
	return 0;
}

static int actorDoesNothingIfNoSupervisorTest() {
	static int someCommand = 0xaa;
	Actor supervised("supervised", [](int i, const RawData &, const ActorLink &) {
		Actor::notifyError(0x69);
		return StatusCode::ok;
	 });
	/* ADD A RESTART HOOK TO CHECK THAT IT IS NOT RESTARTED */
	supervised.post(someCommand);
	supervised.post(someCommand);

	supervised.post(CommandValue::SHUTDOWN);

	return 0;
}

static int actorDoesNothingIfNoSupervisorAndExceptionThrownTest() {
	static int someCommand = 0xaa;
	Actor supervised("supervised", [](int i, const RawData &, const ActorLink &) {
		throw std::runtime_error("some error");
		return StatusCode::ok; //remove that ?
	 });
	/* ADD A RESTART HOOK TO CHECK THAT IT IS NOT RESTARTED */
	supervised.post(someCommand);
	supervised.post(someCommand);

	supervised.post(CommandValue::SHUTDOWN);

	return 0;
}

static int restartAllActorBySupervisorTest() {
	static int someCommand = 0xaa;
	bool supervisorRestarted = false;
	bool supervised1Restarted = false;
	bool supervised2Restarted = false;
	Actor supervisor("supervisor",
			[](int i, const RawData &, const ActorLink &) { return StatusCode::ok; },
			[&supervisorRestarted](void) { supervisorRestarted = true; },
			[](void) { return RestartType::RESTART_ALL; });
	Actor supervised1("supervised1",
			[](int i, const RawData &, const ActorLink &) {
				Actor::notifyError(0x69);
				return StatusCode::ok;
	 	 	 },
			 [&supervised1Restarted](void) { supervised1Restarted = true; });
	Actor supervised2("supervised2",
			[](int i, const RawData &, const ActorLink &) { return StatusCode::ok; },
			[&supervised2Restarted](void) { supervised2Restarted = true; });

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	supervised1.post(someCommand);
	supervised1.post(someCommand);
	supervised2.post(someCommand);

	for (int i = 0; i < 5 && !supervised1Restarted && !supervised2Restarted; i++)
		sleep(1);

	if (supervisorRestarted || !supervised1Restarted || !supervised2Restarted)
		return 1;

	supervisor.post(CommandValue::SHUTDOWN);
	supervised1.post(CommandValue::SHUTDOWN);
	supervised2.post(CommandValue::SHUTDOWN);

	return 0;
}

static int executorTest() {
	MessageQueue messageQueue("queue name");
	Executor executor([](MessageType, int, const RawData &, const ActorLink &) { return StatusCode::shutdown; }, messageQueue);
	messageQueue.post(MessageType::COMMAND_MESSAGE, CommandValue::SHUTDOWN);
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

int _runTest(const _test *suite, size_t nbTests) {
	int nbFailure = 0;

	for (size_t i = 0; i < nbTests; i++) {
		std::cout << suite[i].name << ": " << std::flush;
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
	static const _test suite[] = {
			TEST(basicActorTest),
			TEST(basicActorWithParamsTest),
			TEST(proxyTest),
			TEST(actorSendMessageAndReceiveAnAnswerTest),
			TEST(registryConnectTest),
			TEST(registryAddActorTest),
			TEST(registryAddActorAndRemoveTest),
			TEST(registryAddReferenceTest),
			TEST(registryAddReferenceOverrideExistingOneTest),
			TEST(registeryAddActorFindItBackAndSendMessageTest),
			TEST(registeryFindUnknownActorTest),
			TEST(findActorFromOtherRegistryAndSendMessageTest),
			TEST(findActorFromOtherRegistryAndSendWithSenderForwardToAnotherActorMessageTest),
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
