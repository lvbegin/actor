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
#include "test.h"
#include <actor/actor.h>
#include <actor/rawData.h>
#include <actor/commandMap.h>
#include <actor/actorRegistry.h>
#include <actor/link.h>
#include <private/proxyClient.h>
#include <private/actorCommand.h>
#include <private/proxyServer.h>
#include <private/clientSocket.h>
#include <private/commandValue.h>
#include <private/serverSocket.h>
#include <private/executor.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>

static void actorCommandHasReservedCodeTest(void) {
	static const uint32_t wrongCommand = 0xaa;
	static const commandMap commands[] = {
			{wrongCommand, [](Context &, const RawData &, const SharedSenderLink &link) { return StatusCode::OK;}},
			{ 0, NULL},
	};
	assert_exception(std::runtime_error, ActorCommand c(commands));
}

static void basicActorTest(void) {
	static const int ANSWER = 0x22;
	static const uint32_t command = 0xaa | CommandValue::COMMAND_FLAG;
	static const commandMap commands[] = {
			{command, [](Context &, const RawData &, const SharedSenderLink &link) { return (link->post(ANSWER), StatusCode::OK);}},
			{ 0, NULL},

	};
	const auto link = Link::create();
	const Actor a("actor name", CommandExecutor(commands));
	a.post(command, link);
	assert_eq(ANSWER,  link->get().code);
}

static void basicActorWithParamsTest(void) {
	static const std::string paramValue("Hello World");
	static const uint32_t command = 1 | CommandValue::COMMAND_FLAG;
	static const int OK_ANSWER = 0x22;
	static const int NOK_ANSWER = 0x88;
	static const commandMap commands[] = {
			{command, [](Context &, const RawData &params, const SharedSenderLink &link) {
				if (0 == paramValue.compare(params.toString()))
					link->post(OK_ANSWER);
				else
					link->post(NOK_ANSWER);
				return StatusCode::OK;
				}},
			{ 0, NULL},
	};

	const auto link = Link::create();

	const Actor a("actor name", CommandExecutor(commands));
	a.post(command, paramValue, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void actorSendMessageAndReceiveAnAnswerTest(void) {
	static const std::string paramValue("Hello World");
	static const uint32_t OK_ANSWER = 0x00;
	static const uint32_t NOK_ANSWER = 0x01;
	static const uint32_t command = 1 | CommandValue::COMMAND_FLAG;
	static const commandMap commands[] = {
			{command, [](Context &, const RawData &params, const SharedSenderLink &link) {
				if (0 == paramValue.compare(params.toString()))
					link->post(OK_ANSWER);
				else
					link->post(NOK_ANSWER);
				return StatusCode::OK;
				}},
			{ 0, NULL},
	};

	const Actor a("actor name", CommandExecutor(commands));
	auto queue = Link::create();
	a.post(command, paramValue, queue);
	assert_eq(OK_ANSWER, queue->get().code);
}

static void actorSendMessageAndDoNptReceiveAnAnswerTest(void) {
	static const std::string paramValue("Hello World");
	static const uint32_t OK_ANSWER = 0x00;
	static const uint32_t NOK_ANSWER = 0x01;
	static const uint32_t command = 1 | CommandValue::COMMAND_FLAG;
	static const commandMap commands[] = {
			{command, [](Context &, const RawData &params, const SharedSenderLink &link) {
				return StatusCode::OK;
				}},
			{ 0, NULL},
	};

	const Actor a("actor name", CommandExecutor(commands));
	auto queue = Link::create();
	a.post(command, paramValue, queue);
	auto answer = queue->get(2000);
	assert_false(answer.isValid());
}

static void actorSendUnknownCommandAndReceiveErrorCodeTest(void) {
	static const std::string paramValue("Hello World");

	const Actor a("actor name", CommandExecutor());
	auto queue = Link::create();
	a.post(1, paramValue, queue);
	assert_eq(CommandValue::UNKNOWN_COMMAND, queue->get().code);
}

static void actorSendUnknownCommandCodeTest(void) {
	static const std::string paramValue("Hello World");

	const Actor a("actor name", CommandExecutor());
	a.post(1, paramValue);
}

static void executeSeverProxy(uint16_t port, int *nbMessages) {
	static const uint32_t CODE = 0x33 | CommandValue::COMMAND_FLAG;
	static const commandMap commands[] = {
			{CODE, [nbMessages](Context &, const RawData &, const SharedSenderLink &) { return ((*nbMessages)++, StatusCode::OK); }},
			{ 0, NULL},
	};

	const Actor actor("actor name", CommandExecutor(commands));
	const auto doNothing = []() { };
	const auto DummyGetConnection = [] (std::string) { return SharedSenderLink(); };
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
static void proxyTest(void) {
	static const uint16_t PORT = 4011;
	static const uint32_t CODE = 0x33 | CommandValue::COMMAND_FLAG;
	int nbMessages { 0 };
	std::thread t(executeSeverProxy, PORT, &nbMessages);
	ProxyClient client("client name", openOneConnection(PORT));
	client.post(CODE);
	waitMessageProcessed(nbMessages);
	client.post(CommandValue::SHUTDOWN);
	t.join();
	assert_eq(1, nbMessages);
}

static void registryConnectTest(void) {
	static const uint16_t PORT = 4001;
	const ActorRegistry registry(std::string("name"), PORT);
	const Connection c = openOneConnection(PORT);
}

static void registryAddActorTest(void) {
	static const uint16_t PORT = 4001;
	static const std::string ACTOR_NAME("my actor");
	static const uint32_t command = 1 | CommandValue::COMMAND_FLAG;
	static const commandMap commands[] = {
			{command, [](Context &, const RawData &, const SharedSenderLink &) { return StatusCode::OK;}},
			{ 0, NULL},
	};

	const Actor a(ACTOR_NAME, CommandExecutor(commands));
	ActorRegistry registry(std::string("name"), PORT);

	registry.registerActor(a.getActorLinkRef());
}

static void registryAddActorAndRemoveTest(void) {
	static const uint16_t PORT = 4001;
	static const std::string ACTOR_NAME("my actor");
	const Actor a(ACTOR_NAME, CommandExecutor());
	ActorRegistry registry(std::string("name"), PORT);

	registry.registerActor(a.getActorLinkRef());
	registry.unregisterActor(ACTOR_NAME);
	assert_exception(std::out_of_range, registry.unregisterActor(ACTOR_NAME));

	const Actor anotherA(ACTOR_NAME, CommandExecutor());
	registry.registerActor(anotherA.getActorLinkRef());
}

static void ensureRegistryStarted(uint16_t port) { openOneConnection(port); }

static void registryAddReferenceTest(void) {
	static const std::string NAME1("name1");
	static const std::string NAME2("name2");
	static const uint16_t PORT1 = 4001;
	static const uint16_t port2 = 4002;
	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, port2);

	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(port2);
	const std::string name = registry1.addReference("localhost", port2);
	assert_eq(NAME2, name);
}

static void registryAddReferenceOverrideExistingOneTest(void) {
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
    assert_eq(NAME2, name);
}

static void registeryAddActorFindItBackAndSendMessageTest() {
	static const Command COMMAND = 0x11 | CommandValue::COMMAND_FLAG;
	static const int OK_ANSWER = 0x33;
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT = 4001;
	static const commandMap commands[] = {
			{COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { return (link->post(OK_ANSWER), StatusCode::OK); } },
			{ 0, NULL},
	};

	const auto link = Link::create();
	const Actor a(ACTOR_NAME, CommandExecutor(commands));
	const auto actorRefLink = a.getActorLinkRef();
	ActorRegistry registry("registry name", PORT);
	registry.registerActor(actorRefLink);

	const auto b = registry.getActor(ACTOR_NAME);

	assert_eq(actorRefLink.get(), b.get());
	b->post(COMMAND, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void registeryFindUnknownActorTest() {
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT = 4001;
	const Actor a(ACTOR_NAME, CommandExecutor());
	ActorRegistry registry(std::string("name1"), PORT);
	registry.registerActor(a.getActorLinkRef());

	const SharedSenderLink b = registry.getActor(std::string("wrong name"));
	assert_eq(nullptr, b.get());
}

static void findActorFromOtherRegistryAndSendMessageTest() {
	static const Command DUMMY_COMMAND = 0x33 | CommandValue::COMMAND_FLAG;
	static const std::string NAME1("name1");
	static const std::string NAME2("name2");
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	static const int OK_ANSWER = 0x99;
	static const int NOK_ANSWER = 0x11;
	bool rc = false;
	static const commandMap commands[] = {
			{DUMMY_COMMAND, [&rc](Context &, const RawData &params, const SharedSenderLink &link) {
				if (0 == params.size()) {
					rc = true;
					link->post(OK_ANSWER);
					return StatusCode::OK;
				}
				else {
					rc = false;
					link->post(NOK_ANSWER);
					return StatusCode::ERROR;
				}
			} },
			{ 0, NULL},
	};

	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, PORT2);
	auto link = Link::create("link name needed because the actor is remote");
	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	const std::string name = registry1.addReference("localhost", PORT2);
	assert_eq (NAME2, name);
	const Actor a(ACTOR_NAME, CommandExecutor(commands) );

	registry1.registerActor(link);
	registry2.registerActor(a.getActorLinkRef());
	const auto actor = registry1.getActor(ACTOR_NAME);
	assert_true (nullptr != actor.get());

	sleep(10); // ensure that the proxy server does not stop after a timeout when no command is sent.
	actor->post(DUMMY_COMMAND, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void findActorFromOtherRegistryAndSendWithSenderForwardToAnotherActorMessageTest() {
	static const Command DUMMY_COMMAND = 0x33 | CommandValue::COMMAND_FLAG;
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
	static const Command INTERMEDIATE_COMMAND = 0x44 | CommandValue::COMMAND_FLAG;
	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, PORT2);
	ActorRegistry registry3(NAME3, PORT3);
	auto link = Link::create("link name needed because the actor is remote");
	static const commandMap commandsActor2[] = {
			{INTERMEDIATE_COMMAND, [](Context &, const RawData &params, const SharedSenderLink &link) {
					if (0 == params.size()) {
						link->post(OK_ANSWER);
						return StatusCode::OK;
					}
					else {
						link->post(NOK_ANSWER);
						return StatusCode::ERROR;
					}
				} },
			{ 0, NULL},
	};


	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	ensureRegistryStarted(PORT3);

	registry1.addReference("localhost", PORT2);
	registry1.addReference("localhost", PORT3);
	registry2.addReference("localhost", PORT3);

	const Actor actor2(ACTOR_NAME2, CommandExecutor(commandsActor2));
	registry3.registerActor(actor2.getActorLinkRef());

	auto linkActor2 = registry3.getActor(ACTOR_NAME2);
	static const commandMap commandsActor1[] = {
			{DUMMY_COMMAND, [linkActor2](Context &, const RawData &params, const SharedSenderLink &link) {
				if (0 == params.size()) {
					linkActor2->post(INTERMEDIATE_COMMAND, link);
					return StatusCode::OK;
				}
				else {
					link->post(NOK_ANSWER);
					return StatusCode::ERROR;
				}
			} },
			{ 0, NULL},
	};

	const Actor actor1(ACTOR_NAME1, CommandExecutor(commandsActor1));
	registry2.registerActor(actor1.getActorLinkRef());
	registry1.registerActor(link);
	const auto actor = registry1.getActor(ACTOR_NAME1);
	assert_false (nullptr == actor.get());

	actor->post(DUMMY_COMMAND, link);
	assert_eq (OK_ANSWER, link->get().code);
}

static void findActorFromOtherRegistryAndSendCommandWithParamsTest() {
	static const Command DUMMY_COMMAND = 0x33 | CommandValue::COMMAND_FLAG;
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
	auto link = Link::create("link name needed because the actor is remote");
	static const commandMap commands[] = {
		{DUMMY_COMMAND, [](Context &, const RawData &params, const SharedSenderLink &link) {
			if (0 == PARAM_VALUE.compare(params.toString()))
				link->post(OK_ANSWER);
			else
				link->post(NOK_ANSWER);
			return StatusCode::OK;
		} },
		{ 0, NULL},
	};

	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	const std::string name = registry1.addReference("localhost", PORT2);
	assert_eq (NAME2, name);
		
	const Actor a(ACTOR_NAME, CommandExecutor(commands));
	registry2.registerActor(a.getActorLinkRef());
	registry1.registerActor(link);
	const auto actor = registry1.getActor(ACTOR_NAME);
	assert_false(nullptr == actor.get());

	actor->post(DUMMY_COMMAND, PARAM_VALUE, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void findUnknownActorInMultipleRegistryTest() {
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
	assert_eq(NAME2, name);
	
	const Actor a(ACTOR_NAME, CommandExecutor());
	registry2.registerActor(a.getActorLinkRef());
	const auto actor = registry1.getActor("unknown actor");
	assert_eq(nullptr, actor.get());
}

static void initSupervisionTest() {
	Actor supervisor("supervisor", CommandExecutor());
	Actor supervised("supervised", CommandExecutor());
	supervisor.registerActor(supervised);
	supervisor.unregisterActor(supervised);
}

static void unregisterToSupervisorWhenActorDestroyedTest() {
	Actor supervisor("supervisor", CommandExecutor());
	auto supervised = std::make_unique<Actor>("supervised", CommandExecutor());
	supervisor.registerActor(*supervised.get());

	supervised->post(CommandValue::SHUTDOWN);
	supervised.reset();
}

static void supervisorRestartsActorAfterExceptionTest() {
	static const Command RESTART_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
	static const Command OTHER_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
	static const int OK_ANSWER = 0x99;
	static bool exceptionThrown = false;
	static bool restarted = false;
	const auto link = Link::create("queue for reply");
	Actor supervisor("supervisor", CommandExecutor());
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			 [](const Context &) { restarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
		{RESTART_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
		{OTHER_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { link->post(OK_ANSWER); return StatusCode::OK; }},
		{ 0, NULL},
	};
	Actor supervised("supervised", CommandExecutor(commands), hooks);
	supervisor.registerActor(supervised);
	supervised.post(RESTART_COMMAND, link);
	for(int i = 0; i < 5 && !restarted; i++) sleep(1);
	supervised.post(OTHER_COMMAND, link);
	assert_eq(OK_ANSWER, link->get().code);
		
	assert_true(exceptionThrown);
	assert_true(restarted);
}

static void supervisorRestartsActorAfterReturningErrorTest() {
	static const Command RESTART_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
	static const Command OTHER_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
	static const int OK_ANSWER = 0x99;
	static bool restarted = false;
	const auto link = Link::create("queue for reply");
	Actor supervisor("supervisor", CommandExecutor());
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			 [](const Context &) { restarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
		{RESTART_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { return StatusCode::ERROR; }},
		{OTHER_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { link->post(OK_ANSWER); return StatusCode::OK; }},
		{ 0, NULL},
	};
	Actor supervised("supervised", CommandExecutor(commands), hooks);
	supervisor.registerActor(supervised);
	supervised.post(RESTART_COMMAND, link);
	for(int i = 0; i < 5 && !restarted; i++) sleep(1);
	supervised.post(OTHER_COMMAND, link);
	assert_eq(OK_ANSWER, link->get().code);
	assert_true(restarted);
}

static void supervisorStopActorTest() {
	static const Command STOP_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
	static bool exceptionThrown = false;
	static bool actorStopped = false;
	const auto link = Link::create("queue for reply");
	Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::stopActor(); });
	static const ActorHooks hooks(DEFAULT_START_HOOK, [](const Context&) { actorStopped = true; },
			 						DEFAULT_RESTART_HOOK);
	static const commandMap commands[] = {
		{STOP_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
		{ 0, NULL},
	};

	Actor supervised("supervised", CommandExecutor(commands), hooks);
	supervisor.registerActor(supervised);
	supervised.post(STOP_COMMAND, link);

	for (int i = 0; i < 5 && !exceptionThrown; i++) sleep(1);
	for (int i = 0; i < 5 && !actorStopped; i++) sleep(1);
	assert_true(exceptionThrown);
	assert_true(actorStopped);
}

static void supervisorStopAllActorsTest() {
	static const Command STOP_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
	static bool exceptionThrown = false;
	static int actorStopped = 0;
	const auto link = Link::create("queue for reply");
	Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::stopAllActor(); });
	static const ActorHooks hooks(DEFAULT_START_HOOK, [](const Context&) { actorStopped++; },
			 						DEFAULT_RESTART_HOOK);
	static const commandMap commands[] = {
		{STOP_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
		{ 0, NULL},
	};

	Actor firstSupervised("firstSupervised", CommandExecutor(commands), hooks);
	Actor secondSupervised("secondSupervised", CommandExecutor(), hooks);
	supervisor.registerActor(firstSupervised);
	supervisor.registerActor(secondSupervised);
	firstSupervised.post(STOP_COMMAND, link);

	for (int i = 0; i < 5 && !exceptionThrown; i++) sleep(1);
	for (int i = 0; i < 5 && actorStopped < 2; i++) sleep(1);
	assert_true(exceptionThrown);
	assert_true(actorStopped);
}


static void supervisorForwardErrorRestartTest() {
	static const Command STOP_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
	static bool exceptionThrown = false;
	static bool actorRestarted1 = false;
	static bool actorRestarted2 = false;
	static bool actorRestarted3 = false;
	const auto link = Link::create("queue for reply");
	static const ActorHooks rootHook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[](const Context &c) { actorRestarted1 = true; c.restartActors(); return StatusCode::OK; });
	Actor rootSupervisor("supervisor", CommandExecutor(), rootHook, [](ErrorCode) { return ErrorReactionFactory::restartActor(); });
	static const ActorHooks supervisorHook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[](const Context &c) { actorRestarted2 = true; c.restartActors(); return StatusCode::OK; });
	Actor supervisor("supervisor", CommandExecutor(), supervisorHook, [](ErrorCode) { return ErrorReactionFactory::escalateError(); });
	static const ActorHooks supervisedHooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
	 [](const Context &){ actorRestarted3 = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{STOP_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
			{ 0, NULL},
		};

	Actor supervised("supervised", CommandExecutor(commands), supervisedHooks);
	rootSupervisor.registerActor(supervisor);
	supervisor.registerActor(supervised);
	supervised.post(STOP_COMMAND, link);

	for (int i = 0; i < 5 && !exceptionThrown; i++) sleep(1);
	for (int i = 0; i < 5 && (!actorRestarted2 || !actorRestarted3); i++) sleep(1);
	assert_true(exceptionThrown);
	assert_false(actorRestarted1);
	assert_true(actorRestarted2);
	assert_true(actorRestarted3);
}

static void supervisorForwardErrorStopTest() {
	static const Command STOP_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
	static bool exceptionThrown = false;
	static bool stopped = false;
	const auto link = Link::create("queue for reply");
	Actor rootSupervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::stopActor(); });
	Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::escalateError(); });
	static const ActorHooks hooks(DEFAULT_START_HOOK, [](const Context &) { stopped = true; }, DEFAULT_RESTART_HOOK);
	static const commandMap commands[] = {
			{STOP_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
			{ 0, NULL},
		};
	Actor supervised("supervised", CommandExecutor(commands), hooks);
	rootSupervisor.registerActor(supervisor);
	supervisor.registerActor(supervised);
	supervised.post(STOP_COMMAND, link);

	for (int i = 0; i < 5 && !exceptionThrown && !stopped; i++) sleep(1);
	assert_true(exceptionThrown);
}

static void actorNotifiesErrorToSupervisorTest() {
	static const Command SOME_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
	bool supervisorRestarted = false;
	int supervised1Restarted = 0;
	bool supervised2Restarted = false;
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervisorRestarted](const Context &) { supervisorRestarted = true; return StatusCode::OK; });
	Actor supervisor("supervisor", CommandExecutor(), hooks);
	static const ActorHooks supervised1Hook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			 [&supervised1Restarted](const Context &) { supervised1Restarted++; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { return (Actor::notifyError(0x69), StatusCode::OK); }},
			{ 0, NULL},
		};

	Actor supervised1("supervised1", CommandExecutor(commands), supervised1Hook);
	static const ActorHooks supervised2Hook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervised2Restarted](const Context &) { supervised2Restarted = true; return StatusCode::OK; });
	Actor supervised2("supervised2", CommandExecutor(), supervised2Hook);

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	supervised1.post(SOME_COMMAND);
	supervised1.post(SOME_COMMAND);

	for (int i = 0; i < 5 && 2 > supervised1Restarted; i++)
		sleep(1);
	assert_false(supervisorRestarted || ! (2 == supervised1Restarted) || supervised2Restarted);
}

static void noEffectIfErrorNotifiedAndNoSupervisorTest() {
	static int commandExecuted = 0;
	static bool supervisedRestarted = false;
	static bool supervisedStopped = false;
	static const Command SOME_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
	static const ActorHooks hooks(DEFAULT_START_HOOK, 
			[](const Context &){ supervisedStopped = true;  },
			[](const Context &){ supervisedRestarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { commandExecuted++; return (Actor::notifyError(0x69), StatusCode::OK); }},
			{ 0, NULL},
		};

	Actor supervised("supervised", CommandExecutor(commands), hooks);

	supervised.post(SOME_COMMAND);

	for (int i = 0; i < 5 && 1 > commandExecuted; i++)
		sleep(1);

	assert_eq(1, commandExecuted);
	assert_false(supervisedRestarted);
	assert_true(supervisedStopped);
}

static void noEffectIfActorCommandReturnsErrorAndNoSupervisorTest() {
	static int commandExecuted = 0;
	static bool supervisedRestarted = false;
	static bool supervisedStopped = false;
	static const Command SOME_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
	static const ActorHooks hooks(DEFAULT_START_HOOK, 
			[](const Context &){ supervisedStopped = true;  },
			[](const Context &){ supervisedRestarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { commandExecuted++; return StatusCode::ERROR; }},
			{ 0, NULL},
		};

	Actor supervised("supervised", CommandExecutor(commands), hooks);

	supervised.post(SOME_COMMAND);

	for (int i = 0; i < 5 && 1 > commandExecuted; i++)
		sleep(1);

	assert_eq(1, commandExecuted);
	assert_false(supervisedRestarted);
	assert_true(supervisedStopped);
}

static void actorDoesNothingIfNoSupervisorAndExceptionThrownTest() {
	static const Command SOME_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
	static bool supervisorRestarted = false;
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[](const Context &){ supervisorRestarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) -> StatusCode{ throw std::runtime_error("some error"); }},
			{ 0, NULL},
		};

	auto supervised = std::make_shared<Actor>("supervised", CommandExecutor(commands), hooks);
	supervised->post(SOME_COMMAND);
	supervised->post(SOME_COMMAND);

	supervised.reset();
	assert_false(supervisorRestarted);
}

static void actorDoesNothingIfSupervisorUsesDoNothingErrorStrategyTest() {
	static const Command SOME_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
	static bool supervisedRestarted = false;
	static int commandExecuted = 0;
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[](const Context &){ supervisedRestarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) -> StatusCode
							{ commandExecuted++; throw std::runtime_error("some error"); }},
			{ 0, NULL},
		};

	Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::doNothing(); });		
	Actor supervised("supervised", CommandExecutor(commands), hooks);
	supervisor.registerActor(supervised);
	supervised.post(SOME_COMMAND);
	supervised.post(SOME_COMMAND);
	for (int i = 0; i < 5 && 2 > commandExecuted; i++)
		sleep(1);

	assert_eq(2, commandExecuted);
	assert_false(supervisedRestarted);
}

static void restartAllActorBySupervisorTest() {
	static const Command SOME_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
	bool supervisorRestarted = false;
	bool supervised1Restarted = false;
	bool supervised2Restarted = false;
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervisorRestarted](const Context &) { supervisorRestarted = true; return StatusCode::OK; });
	Actor supervisor("supervisor", CommandExecutor(), hooks, [](ErrorCode) { return ErrorReactionFactory::restartAllActor(); });
	static const ActorHooks supervised1Hook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervised1Restarted](const Context &) { supervised1Restarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { return (Actor::notifyError(0x69), StatusCode::OK); }},
			{ 0, NULL},
		};
	Actor supervised1("supervised1", CommandExecutor(commands), supervised1Hook);
	static const ActorHooks supervised2Hook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervised2Restarted](const Context &) { supervised2Restarted = true; return StatusCode::OK; });
	Actor supervised2("supervised2", CommandExecutor(), supervised2Hook);

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	supervised1.post(SOME_COMMAND);
	supervised1.post(SOME_COMMAND);
	supervised2.post(SOME_COMMAND);

	for (int i = 0; i < 5 && !supervised1Restarted && !supervised2Restarted; i++)
		sleep(1);

	assert_false(supervisorRestarted || !supervised1Restarted || !supervised2Restarted);
}

static void stoppingSupervisorStopssupervisedTest() {
	static const Command STOP_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
	static bool supervisorStopped = false;
	static bool supervisedStopped = false;
	static const ActorHooks supersivorHooks(DEFAULT_START_HOOK,
			[](const Context&c) { c.stopActors(); supervisorStopped = true; }, DEFAULT_RESTART_HOOK);
	static const commandMap commands[] = {
			{STOP_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { return StatusCode::SHUTDOWN; }},
			{ 0, NULL},
		};
	Actor supervisor("supervisor", CommandExecutor(commands), supersivorHooks);
	static const ActorHooks supervisedHooks(DEFAULT_START_HOOK, [](const Context&) { supervisedStopped = true; },
			 DEFAULT_RESTART_HOOK);
	Actor supervised("supervised", CommandExecutor(), supervisedHooks);
	supervisor.registerActor(supervised);
	supervisor.post(STOP_COMMAND);

	for (int i = 0; i < 5 && !(supervisedStopped && supervisorStopped); i++) sleep(1);
	assert_true(supervisorStopped);
	assert_true(supervisedStopped);
}

static void supervisorHasDifferentStrategyDependingOnErrorTest() {
	static const Command first_error = 0x11 | CommandValue::COMMAND_FLAG;
	static const Command second_error = 0x22 | CommandValue::COMMAND_FLAG;
	static const auto strategy = [](ErrorCode error) {
		if (first_error == error)
			return ErrorReactionFactory::restartActor();
		else
			return ErrorReactionFactory::stopActor();
	};
	bool supervisorRestarted = false;
	bool supervisedRestarted = false;
	bool supervisedStopped = false;
	static const ActorHooks supervisorHooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervisorRestarted](const Context &) { supervisorRestarted = true; return StatusCode::OK; });
	Actor supervisor("supervisor", CommandExecutor(), supervisorHooks, strategy);
	static const ActorHooks supervisedHooks(DEFAULT_START_HOOK,
			[&supervisedStopped](const Context &) { supervisedStopped = true; },
			[&supervisedRestarted](const Context &) { supervisedRestarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{first_error, [](Context &, const RawData &, const SharedSenderLink &) { return (Actor::notifyError(first_error), StatusCode::OK); }},
			{second_error, [](Context &, const RawData &, const SharedSenderLink &) { return (Actor::notifyError(second_error), StatusCode::OK); }},
			{ 0, NULL},
	};
	Actor supervised("supervised", CommandExecutor(commands), supervisedHooks);

	supervisor.registerActor(supervised);

	supervised.post(first_error);
	for (int i = 0; i < 5 && !supervisedRestarted; i++)
		sleep(1);
	supervised.post(second_error);
	for (int i = 0; i < 5 && !supervisedStopped; i++)
		sleep(1);

	assert_false(supervisorRestarted ||  !supervisedRestarted || !supervisedStopped);
}

static void executorTest() {
	auto link = Link::create();
	Executor executor([](MessageType, int, const RawData &, const SharedSenderLink &) { return StatusCode::SHUTDOWN; }, *link);
	link->post(MessageType::COMMAND_MESSAGE, CommandValue::SHUTDOWN);
}

static void serializationTest() {
	static const uint32_t EXPECTED_VALUE = 0x11223344;

	const RawData s(EXPECTED_VALUE);
	assert_eq(EXPECTED_VALUE, toId(s));
}

static void startActorFailureTest() {
	bool exceptionThrown = false;
	static const ActorHooks hooks([](const Context& ) { return StatusCode::ERROR; },
			DEFAULT_STOP_HOOK, DEFAULT_RESTART_HOOK);
	try {
		Actor actor("supervisor", CommandExecutor(), hooks);
	} catch (ActorStartFailure &) {
		exceptionThrown = true;
	}
	assert_true(exceptionThrown);
}

static void restartActorFailureTest() {
	static const Command COMMAND = 0x33 | CommandValue::COMMAND_FLAG;
	static int actorRestarted = 0;
	static bool actorStoppedProperly = false;
	static const ActorHooks hooks(DEFAULT_START_HOOK,
			[](const Context& ) { actorStoppedProperly = true; },
			[](const Context& ) { actorRestarted++; return StatusCode::ERROR; });
	static const commandMap commands[] = {
			{ COMMAND, [](Context &, const RawData &, const SharedSenderLink &) -> StatusCode { throw std::runtime_error("error"); }},
			{ 0, NULL},
	};

	{
		Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::restartActor(); });
		Actor supervised("supervised", CommandExecutor(commands), hooks);
		supervisor.registerActor(supervised);
		supervised.post(COMMAND);
		for (int i = 0; i < 5 && 0 == actorRestarted; i++) sleep(1);

	}
	assert_eq(1, actorRestarted);
	assert_true(actorStoppedProperly);
}

static void preAndPostActionCalledTest() {
	static const Command COMMAND = 0x33 | CommandValue::COMMAND_FLAG;
	static bool preCalled = false;
	static bool postCalled = false;
	static const commandMap commands[] = {
			{ COMMAND, [](Context &, const RawData &, const SharedSenderLink &) { return StatusCode::OK; }},
			{ 0, NULL},
	};


	auto preCommand = [](Context &, Command, const RawData &, const SharedSenderLink &) { preCalled = true; return StatusCode::OK; };
	auto postCommand = [](Context &, Command, const RawData &, const SharedSenderLink &) { postCalled = true; };
	Actor actor("actor", CommandExecutor(preCommand, postCommand, commands));
	actor.post(COMMAND);
	for (int i = 0; i < 5 && ! (preCalled && postCalled); i++) sleep(1);
	assert_true(preCalled);
	assert_true(postCalled);
}

static void preActionFailsTest() {
	static const Command COMMAND = 0x33 | CommandValue::COMMAND_FLAG;
	static bool commandExecuted = false;
	static bool preCalled = false;
	static bool postCalled = false;
	static const commandMap commands[] = {
			{ COMMAND, [](Context &, const RawData &, const SharedSenderLink &) { commandExecuted = true; return StatusCode::OK; }},
			{ 0, NULL},
	};

	auto preCommand = [](Context &, Command, const RawData &, const SharedSenderLink &) { preCalled = true; return StatusCode::ERROR; };
	auto postCommand = [](Context &, Command, const RawData &, const SharedSenderLink &) { postCalled = true; };
	Actor actor("actor", CommandExecutor(preCommand, postCommand, commands));
	actor.post(COMMAND);
	for (int i = 0; i < 5 && ! preCalled; i++) sleep(1);
	assert_true(preCalled);
	assert_false(postCalled); 
	assert_false(commandExecuted);
}

static void commandFailsAndPostActionCalledTest() {
	static const Command COMMAND = 0x33 | CommandValue::COMMAND_FLAG;
	static bool preCalled = false;
	static bool postCalled = false;
	static const commandMap commands[] = {
			{ COMMAND, [](Context &, const RawData &, const SharedSenderLink &) { return StatusCode::ERROR; }},
			{ 0, NULL},
	};


	auto preCommand = [](Context &, Command, const RawData &, const SharedSenderLink &) { preCalled = true; return StatusCode::OK; };
	auto postCommand = [](Context &, Command, const RawData &, const SharedSenderLink &) { postCalled = true; };
	Actor actor("actor", CommandExecutor(preCommand, postCommand, commands));
	actor.post(COMMAND);
	for (int i = 0; i < 5 && ! (preCalled && postCalled); i++) sleep(1);
	assert_true(preCalled);
	assert_true(postCalled);
}

static void commandFailsWithExceptionAndPostActionCalledTest() {
	static const Command COMMAND = 0x33 | CommandValue::COMMAND_FLAG;
	static bool preCalled = false;
	static bool postCalled = false;
	static const commandMap commands[] = {
			{ COMMAND, [](Context &, const RawData &, const SharedSenderLink &) {
								throw std::runtime_error("some problem"); return StatusCode::OK; }},
			{ 0, NULL},
	};


	auto preCommand = [](Context &, Command, const RawData &, const SharedSenderLink &) { preCalled = true; return StatusCode::OK; };
	auto postCommand = [](Context &, Command, const RawData &, const SharedSenderLink &) { postCalled = true; };
	Actor actor("actor", CommandExecutor(preCommand, postCommand, commands));
	actor.post(COMMAND);
	for (int i = 0; i < 5 && ! (preCalled && postCalled); i++) sleep(1);
	assert_true(preCalled);
	assert_true(postCalled);
}

class DummyState : public State {
public:
	DummyState() : initCalled(0) {}
	~DummyState() = default;
	void init(const std::string &name) { initCalled++; }
	int isInitCalled() { return initCalled; }
private:
	int initCalled;
};

static void initStateDoneAtStart() {
	DummyState *state = new DummyState();
	Actor actor("actor", CommandExecutor(), std::unique_ptr<State>(state));
	for (int i = 0; i < 5 && (0 == state->isInitCalled()); i++) sleep(1);
	assert_eq(1, state->isInitCalled());
}

static void initStateDoneAtRestart() {
	static const Command RESTART_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
	static const commandMap commands[] = {
		{RESTART_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) -> StatusCode { throw std::runtime_error("some error"); }},
		{ 0, NULL},
	};
	DummyState *state = new DummyState();
	Actor supervisor("supervisor", CommandExecutor());
	Actor supervised("supervisor", CommandExecutor(commands), std::unique_ptr<State>(state));

	supervisor.registerActor(supervised);
	supervised.post(RESTART_COMMAND);

	for (int i = 0; i < 5 && (2 != state->isInitCalled()); i++) sleep(1);
	assert_eq(2, state->isInitCalled());
}

int main() {
	static const _test suite[] = {
			TEST(actorCommandHasReservedCodeTest),
			TEST(basicActorTest),
			TEST(basicActorWithParamsTest),
			TEST(proxyTest),
			TEST(actorSendMessageAndReceiveAnAnswerTest),
			TEST(actorSendMessageAndDoNptReceiveAnAnswerTest),
			TEST(actorSendUnknownCommandAndReceiveErrorCodeTest),
			TEST(actorSendUnknownCommandCodeTest),
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
			TEST(supervisorRestartsActorAfterExceptionTest),
			TEST(supervisorRestartsActorAfterReturningErrorTest),
			TEST(supervisorStopActorTest),
			TEST(supervisorStopAllActorsTest),
			TEST(supervisorForwardErrorRestartTest),
			TEST(supervisorForwardErrorStopTest),
		 	TEST(actorNotifiesErrorToSupervisorTest),
			TEST(noEffectIfErrorNotifiedAndNoSupervisorTest),
			TEST(noEffectIfActorCommandReturnsErrorAndNoSupervisorTest),
			TEST(actorDoesNothingIfNoSupervisorAndExceptionThrownTest),
			TEST(actorDoesNothingIfSupervisorUsesDoNothingErrorStrategyTest),
			TEST(restartAllActorBySupervisorTest),
			TEST(stoppingSupervisorStopssupervisedTest),
			TEST(supervisorHasDifferentStrategyDependingOnErrorTest),
			TEST(executorTest),
			TEST(serializationTest),
			TEST(startActorFailureTest),
			TEST(restartActorFailureTest),
			TEST(preAndPostActionCalledTest),
			TEST(preActionFailsTest),
			TEST(commandFailsAndPostActionCalledTest),
			TEST(commandFailsWithExceptionAndPostActionCalledTest),
			TEST(initStateDoneAtStart),
			TEST(initStateDoneAtRestart),
	};

	const auto nbFailure = runTest(suite);
	if (nbFailure > 0)
		return (std::cout << nbFailure << " tests failed." << std::endl, EXIT_FAILURE);
	else
		return (std::cout << "Success!" << std::endl, EXIT_SUCCESS);
}
