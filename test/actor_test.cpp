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

#include <proxyClient.h>
#include <proxyServer.h>
#include <actor.h>
#include <actorRegistry.h>
#include <clientSocket.h>
#include <commandValue.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>


static int basicActorTest(void) {
	static const uint32_t command = 0xaa;
	static const int ANSWER = 0x22;
	const auto link = std::make_shared<MessageQueue>();
	const Actor a("actor name", [](int i, const RawData &, const ActorLink &link) { link->post(ANSWER); return StatusCode::ok; });
	a.post(command, link);
	return (ANSWER == link->get().code) ? 0 : 1;
}

static int basicActorWithParamsTest(void) {
	static const std::string paramValue("Hello World");
	const RawData params(paramValue.begin(), paramValue.end());
	static const int OK_ANSWER = 0x22;
	static const int NOK_ANSWER = 0x88;
	const auto link = std::make_shared<MessageQueue>();

	const Actor a("actor name", [](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
				if (0 == paramValue.compare(std::string(params.begin(), params.end())))
					link->post(OK_ANSWER);
				else
					link->post(NOK_ANSWER);
				return StatusCode::ok;
	});
	a.post(1, params, link);
	return (OK_ANSWER == link->get().code) ? 0 : 1;
}

static int actorSendMessageAndReceiveAnAnswerTest(void) {
	static const std::string paramValue("Hello World");
	static const uint32_t OK_ANSWER = 0x00;
	static const uint32_t NOK_ANSWER = 0x01;
	const RawData params(paramValue.begin(), paramValue.end());

	const Actor a("actor name", [](int i, const std::vector<unsigned char> &params, const ActorLink &sender) {
				if (0 == paramValue.compare(std::string(params.begin(), params.end())))
					sender->post(OK_ANSWER);
				else
					sender->post(NOK_ANSWER);
				return StatusCode::ok;
	});
	auto queue = std::make_shared<MessageQueue>();
	a.post(1, params, queue);
	return (OK_ANSWER == queue->get().code) ? 0 : 1;
}

static void executeSeverProxy(uint16_t port, int *nbMessages) {
	const Actor actor("actor name", [nbMessages](int i, const RawData &, const ActorLink &) {
		(*nbMessages)++;
		return StatusCode::ok;
	});
	const auto doNothing = []() { };
	const auto DummyGetConnection = [] (std::string) { return ActorLink(); };
	const ProxyServer server(actor.getActorLinkRef(), ServerSocket::getConnection(port), doNothing, DummyGetConnection);
}

static Connection openOneConnection(uint16_t port) {
	while (true) {
		try {
			return ClientSocket::openHostConnection("localhost", port);
		} catch (std::exception &e) { }
	}
}

static void waitMessageProcessed(int &nbMessages) {
	for (int nbAttempt = 0; 0 == nbMessages && 5 > nbAttempt; nbAttempt++) {
		sleep(1);
	}
}
static int proxyTest(void) {
	static const uint16_t PORT = 4011;
	static const uint32_t CODE = 0x33;
	int nbMessages { 0 };
	std::thread t(executeSeverProxy, PORT, &nbMessages);
	ProxyClient client("client name", openOneConnection(PORT));
	client.post(CODE);
	waitMessageProcessed(nbMessages);
	client.post(CommandValue::SHUTDOWN);
	t.join();
	return (1 == nbMessages) ? 0 : 1;
}

static int registryConnectTest(void) {
	static const uint16_t PORT = 4001;
	const ActorRegistry registry(std::string("name"), PORT);
	const Connection c = openOneConnection(PORT);
	return 0;
}

static int registryAddActorTest(void) {
	static const uint16_t PORT = 4001;
	static const std::string ACTOR_NAME("my actor");
	const Actor a(ACTOR_NAME, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name"), PORT);

	registry.registerActor(a.getActorLinkRef());

	return 0;
}

static int registryAddActorAndRemoveTest(void) {
	static const uint16_t PORT = 4001;
	static const std::string ACTOR_NAME("my actor");
	const Actor a(ACTOR_NAME, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name"), PORT);

	registry.registerActor(a.getActorLinkRef());
	registry.unregisterActor(ACTOR_NAME);
	try {
	    registry.unregisterActor(ACTOR_NAME);
	    return 1;
	} catch (std::out_of_range &e) { }

	const Actor anotherA(ACTOR_NAME, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	registry.registerActor(anotherA.getActorLinkRef());
	return 0;
}

static void ensureRegistryStarted(uint16_t port) { openOneConnection(port); }

static int registryAddReferenceTest(void) {
	static const std::string NAME1("name1");
	static const std::string NAME2("name2");
	static const uint16_t PORT1 = 4001;
	static const uint16_t port2 = 4002;
	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, port2);

	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(port2);
	const std::string name = registry1.addReference("localhost", port2);
	return NAME2 == name ? 0 : 1;
}

static int registryAddReferenceOverrideExistingOneTest(void) {
	static const std::string NAME1("name1");
	static const std::string NAME2("name2");
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	static const uint16_t PORT3 = 6003;
	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, PORT2);
	ActorRegistry registry3(NAME1, PORT3);

	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	registry1.addReference("localhost", PORT2);
	const std::string name = registry3.addReference("localhost", PORT2);
	return NAME2 == name ? 0 : 1;
}

static int registeryAddActorFindItBackAndSendMessageTest() {
	static const int COMMAND = 0x11;
	static const int OK_ANSWER = 0x33;
	static const int NOK_ANSWER = 0x88;
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT = 4001;
	const auto link = std::make_shared<MessageQueue>();
	const Actor a(ACTOR_NAME, [](int i, const RawData &, const ActorLink &link) {
		if (COMMAND == i)
			link->post(OK_ANSWER);
		else
			link->post(NOK_ANSWER);
		return StatusCode::ok;
	});
	const auto actorRefLink = a.getActorLinkRef();
	ActorRegistry registry("registry name", PORT);
	registry.registerActor(actorRefLink);

	const auto b = registry.getActor(ACTOR_NAME);

	if (actorRefLink.get() != b.get())
		return 1;
	b->post(COMMAND, link);
	return (OK_ANSWER == link->get().code) ? 0 : 1;
}

static int registeryFindUnknownActorTest() {
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT = 4001;
	const Actor a(ACTOR_NAME, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	ActorRegistry registry(std::string("name1"), PORT);
	registry.registerActor(a.getActorLinkRef());

	const ActorLink b = registry.getActor(std::string("wrong name"));
	return (nullptr == b.get()) ? 0 : 1;
}

static int findActorFromOtherRegistryAndSendMessageTest() {
	static const uint32_t DUMMY_COMMAND = 0x33;
	static const std::string NAME1("name1");
	static const std::string NAME2("name2");
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	static const int OK_ANSWER = 0x99;
	static const int NOK_ANSWER = 0x11;
	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, PORT2);
	auto link = std::make_shared<MessageQueue>("link name needed because the actor is remote");
	bool rc = false;
	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	const std::string name = registry1.addReference("localhost", PORT2);
	if (NAME2.compare(name))
		return 1;
	const Actor a(ACTOR_NAME, [&rc](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
		if (i == DUMMY_COMMAND && 0 == params.size()) {
			rc = true;
			link->post(OK_ANSWER);
			return StatusCode::ok;
		}
		else {
			rc = false;
			link->post(NOK_ANSWER);
			return StatusCode::error;
		}
	} );

	registry1.registerActor(link);
	registry2.registerActor(a.getActorLinkRef());
	const auto actor = registry1.getActor(ACTOR_NAME);
	if (nullptr == actor.get())
		return 1;
	sleep(10); // ensure that the proxy server does not stop after a timeout when no command is sent.
	actor->post(DUMMY_COMMAND, link);
	return (OK_ANSWER == link->get().code) ? 0 : 1;
}

static int findActorFromOtherRegistryAndSendWithSenderForwardToAnotherActorMessageTest() {
	static const uint32_t DUMMY_COMMAND = 0x33;
	static const std::string NAME1("name1");
	static const std::string NAME2("name2");
	static const std::string NAME3("name3");
	static const std::string ACTOR_NAME1("my actor 1");
	static const std::string ACTOR_NAME2("my actor 2");
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	static const uint16_t PORT3 = 4003;
	static const int OK_ANSWER = 0x99;
	static const int NOK_ANSWER = 0x11;
	static const int INTERMEDIATE_COMMAND = 0x44;
	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, PORT2);
	ActorRegistry registry3(NAME3, PORT3);
	auto link = std::make_shared<MessageQueue>("link name needed because the actor is remote");
	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	ensureRegistryStarted(PORT3);

	registry1.addReference("localhost", PORT2);
	registry1.addReference("localhost", PORT3);
	registry2.addReference("localhost", PORT3);

	const Actor actor2(ACTOR_NAME2, [](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
		if (i == INTERMEDIATE_COMMAND && 0 == params.size()) {
			link->post(OK_ANSWER);
			return StatusCode::ok;
		}
		else {
			link->post(NOK_ANSWER);
			return StatusCode::error;
		}
	} );
	registry3.registerActor(actor2.getActorLinkRef());

	auto linkActor2 = registry3.getActor(ACTOR_NAME2);

	const Actor actor1(ACTOR_NAME1, [linkActor2](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
		if (i == DUMMY_COMMAND && 0 == params.size()) {
			linkActor2->post(INTERMEDIATE_COMMAND, link);
			return StatusCode::ok;
		}
		else {
			link->post(NOK_ANSWER);
			return StatusCode::error;
		}
	} );
	registry2.registerActor(actor1.getActorLinkRef());
	registry1.registerActor(link);
	const auto actor = registry1.getActor(ACTOR_NAME1);
	if (nullptr == actor.get())
		return 1;
	actor->post(DUMMY_COMMAND, link);
	return (OK_ANSWER == link->get().code) ? 0 : 1;
}

static int findActorFromOtherRegistryAndSendCommandWithParamsTest() {
	static const uint32_t DUMMY_COMMAND = 0x33;
	static const std::string PARAM_VALUE("Hello World");
	static const std::string NAME1("name1");
	static const std::string NAME2("name2");
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	static const int OK_ANSWER = 0x99;
	static const int NOK_ANSWER = 0x11;
	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, PORT2);
	auto link = std::make_shared<MessageQueue>("link name needed because the actor is remote");

	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	const std::string name = registry1.addReference("localhost", PORT2);
	if (NAME2.compare(name))
		return 1;
	const Actor a(ACTOR_NAME, [](int i, const std::vector<unsigned char> &params, const ActorLink &link) {
		if (i == DUMMY_COMMAND && 0 == PARAM_VALUE.compare(std::string(params.begin(), params.end())))
			link->post(OK_ANSWER);
		else
			link->post(NOK_ANSWER);
		return StatusCode::ok;
	} );
	registry2.registerActor(a.getActorLinkRef());
	registry1.registerActor(link);
	const auto actor = registry1.getActor(ACTOR_NAME);
	if (nullptr == actor.get())
		return 1;
	actor->post(DUMMY_COMMAND, std::vector<unsigned char>(PARAM_VALUE.begin(), PARAM_VALUE.end()), link);
	return OK_ANSWER == link->get().code ? 0 : 1;
}


static int findUnknownActorInMultipleRegistryTest() {
	static const std::string NAME1("name1");
	static const std::string NAME2("name2");
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, PORT2);

	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	const std::string name = registry1.addReference("localhost", PORT2);
	if (NAME2.compare(name))
		return 1;
	const Actor a(ACTOR_NAME, [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
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
	static const int RESTART_COMMAND = 0x99;
	static const int OTHER_COMMAND = 0xaa;
	static const int OK_ANSWER = 0x99;
	static bool exceptionThrown = false;
	const auto link = std::make_shared<MessageQueue>("queue for reply");
	Actor supervisor("supervisor", [](int i, const RawData &, const ActorLink &) { return StatusCode::ok; });
	Actor supervised("supervised", [](int i, const RawData &, const ActorLink &link) {
		if (i == RESTART_COMMAND) {
			exceptionThrown = true;
			throw std::runtime_error("some error");
		}
		link->post(OK_ANSWER);
		 return StatusCode::ok;
	 });
	supervisor.registerActor(supervised);
	supervised.post(RESTART_COMMAND, link);
	supervised.post(OTHER_COMMAND, link);
	if (OK_ANSWER != link->get().code)
		return 1;
	supervisor.post(CommandValue::SHUTDOWN);
	supervised.post(CommandValue::SHUTDOWN);

	return (exceptionThrown) ? 0 : 1;
}

static int actorNotifiesErrorToSupervisorTest() {
	static const int SOME_COMMAND = 0xaa;
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

	supervised1.post(SOME_COMMAND);
	supervised1.post(SOME_COMMAND);

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
	static const int SOME_COMMAND = 0xaa;
	bool supervisorRestarted = false;

	auto supervised = std::make_shared<Actor>("supervised", [](int i, const RawData &, const ActorLink &) {
		Actor::notifyError(0x69);
		return StatusCode::ok;
	 },
	[&supervisorRestarted](){ supervisorRestarted = true; });

	supervised->post(SOME_COMMAND);
	supervised->post(SOME_COMMAND);

	supervised->post(CommandValue::SHUTDOWN);
	supervised.reset();
	return !supervisorRestarted ? 0 : 1;
}

static int actorDoesNothingIfNoSupervisorAndExceptionThrownTest() {
	static const int SOME_COMMAND = 0xaa;
	bool supervisorRestarted = false;

	auto supervised = std::make_shared<Actor>("supervised", [](int i, const RawData &, const ActorLink &) {
		throw std::runtime_error("some error");
		return StatusCode::ok;
	 },
	[&supervisorRestarted](){ supervisorRestarted = true; });
	supervised->post(SOME_COMMAND);
	supervised->post(SOME_COMMAND);

	supervised->post(CommandValue::SHUTDOWN);
	supervised.reset();
	return !supervisorRestarted ? 0 : 1;
}

static int restartAllActorBySupervisorTest() {
	static const int SOME_COMMAND = 0xaa;
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

	supervised1.post(SOME_COMMAND);
	supervised1.post(SOME_COMMAND);
	supervised2.post(SOME_COMMAND);

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
	MessageQueue messageQueue;
	Executor executor([](MessageType, int, const RawData &, const ActorLink &) { return StatusCode::shutdown; }, messageQueue);
	messageQueue.post(MessageType::COMMAND_MESSAGE, CommandValue::SHUTDOWN);
	return 0;
}

static int serializationTest() {
	static const uint32_t EXPECTED_VALUE = 0x11223344;

	const auto s = UniqueId::serialize(EXPECTED_VALUE);
	return (EXPECTED_VALUE == UniqueId::unserialize(s)) ? 0 : 1;
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
