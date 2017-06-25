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
#include <rawData.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>


static int basicActorTest(void) {
	static const int ANSWER = 0x22;
	static const uint32_t command = 0xaa;
	static const commandMap commands[] = {
			{command, [](const RawData &, const ActorLink &link) { return (link->post(ANSWER), StatusCode::OK);}},
	};
	const auto link = std::make_shared<MessageQueue>();
	const Actor a("actor name", ActorCommand(commands, 1));
	a.post(command, link);
	return (ANSWER == link->get().code) ? 0 : 1;
}

static int basicActorWithParamsTest(void) {
	static const std::string paramValue("Hello World");
	static const uint32_t command = 1;
	static const int OK_ANSWER = 0x22;
	static const int NOK_ANSWER = 0x88;
	static const commandMap commands[] = {
			{command, [](const RawData &params, const ActorLink &link) {
				if (0 == paramValue.compare(params.toString()))
					link->post(OK_ANSWER);
				else
					link->post(NOK_ANSWER);
				return StatusCode::OK;
				}},
	};

	const auto link = std::make_shared<MessageQueue>();

	const Actor a("actor name", ActorCommand(commands, 1));
	a.post(command, paramValue, link);
	return (OK_ANSWER == link->get().code) ? 0 : 1;
}

static int actorSendMessageAndReceiveAnAnswerTest(void) {
	static const std::string paramValue("Hello World");
	static const uint32_t OK_ANSWER = 0x00;
	static const uint32_t NOK_ANSWER = 0x01;
	static const uint32_t command = 1;
	static const commandMap commands[] = {
			{command, [](const RawData &params, const ActorLink &link) {
				if (0 == paramValue.compare(params.toString()))
					link->post(OK_ANSWER);
				else
					link->post(NOK_ANSWER);
				return StatusCode::OK;
				}},
	};

	const Actor a("actor name", ActorCommand(commands, 1));
	auto queue = std::make_shared<MessageQueue>();
	a.post(1, paramValue, queue);
	return (OK_ANSWER == queue->get().code) ? 0 : 1;
}

static int actorSendUnknownCommandAndReceiveErrorCodeTest(void) {
	static const std::string paramValue("Hello World");

	const Actor a("actor name", ActorCommand());
	auto queue = std::make_shared<MessageQueue>();
	a.post(1, paramValue, queue);
	return (CommandValue::UNKNOWN_COMMAND == queue->get().code) ? 0 : 1;
}

static int actorSendUnknownCommandCodeTest(void) {
	static const std::string paramValue("Hello World");

	const Actor a("actor name", ActorCommand());
	a.post(1, paramValue);
	return 0;
}

static void executeSeverProxy(uint16_t port, int *nbMessages) {
	static const uint32_t CODE = 0x33;
	static const commandMap commands[] = {
			{CODE, [nbMessages](const RawData &, const ActorLink &) { return ((*nbMessages)++, StatusCode::OK); }},
	};

	const Actor actor("actor name", ActorCommand(commands, 1));
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
	static const uint32_t command = 1;
	static const commandMap commands[] = {
			{command, [](const RawData &, const ActorLink &) { return StatusCode::OK;}},
	};

	const Actor a(ACTOR_NAME, ActorCommand(commands, 1));
	ActorRegistry registry(std::string("name"), PORT);

	registry.registerActor(a.getActorLinkRef());

	return 0;
}

static int registryAddActorAndRemoveTest(void) {
	static const uint16_t PORT = 4001;
	static const std::string ACTOR_NAME("my actor");
	const Actor a(ACTOR_NAME, ActorCommand());
	ActorRegistry registry(std::string("name"), PORT);

	registry.registerActor(a.getActorLinkRef());
	registry.unregisterActor(ACTOR_NAME);
	try {
	    registry.unregisterActor(ACTOR_NAME);
	    return 1;
	} catch (std::out_of_range &e) { }

	const Actor anotherA(ACTOR_NAME, ActorCommand());
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
	static const std::string ACTOR_NAME("my actor");
	static const uint16_t PORT = 4001;
	static const commandMap commands[] = {
			{COMMAND, [](const RawData &, const ActorLink &link) { return (link->post(OK_ANSWER), StatusCode::OK); } },
	};

	const auto link = std::make_shared<MessageQueue>();
	const Actor a(ACTOR_NAME, ActorCommand(commands, 1));
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
	const Actor a(ACTOR_NAME, ActorCommand());
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
	bool rc = false;
	static const commandMap commands[] = {
			{DUMMY_COMMAND, [&rc](const RawData &params, const ActorLink &link) {
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
	};

	ActorRegistry registry1(NAME1, PORT1);
	ActorRegistry registry2(NAME2, PORT2);
	auto link = std::make_shared<MessageQueue>("link name needed because the actor is remote");
	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	const std::string name = registry1.addReference("localhost", PORT2);
	if (NAME2.compare(name))
		return 1;
	const Actor a(ACTOR_NAME, ActorCommand(commands, 1) );

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
	static const commandMap commandsActor2[] = {
			{INTERMEDIATE_COMMAND, [](const RawData &params, const ActorLink &link) {
					if (0 == params.size()) {
						link->post(OK_ANSWER);
						return StatusCode::OK;
					}
					else {
						link->post(NOK_ANSWER);
						return StatusCode::ERROR;
					}
				} },
	};


	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	ensureRegistryStarted(PORT3);

	registry1.addReference("localhost", PORT2);
	registry1.addReference("localhost", PORT3);
	registry2.addReference("localhost", PORT3);

	const Actor actor2(ACTOR_NAME2, ActorCommand(commandsActor2, 1));
	registry3.registerActor(actor2.getActorLinkRef());

	auto linkActor2 = registry3.getActor(ACTOR_NAME2);
	static const commandMap commandsActor1[] = {
			{DUMMY_COMMAND, [linkActor2](const RawData &params, const ActorLink &link) {
				if (0 == params.size()) {
					linkActor2->post(INTERMEDIATE_COMMAND, link);
					return StatusCode::OK;
				}
				else {
					link->post(NOK_ANSWER);
					return StatusCode::ERROR;
				}
			} },
	};

	const Actor actor1(ACTOR_NAME1, ActorCommand(commandsActor1, 1));
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
	static const commandMap commands[] = { {DUMMY_COMMAND, [](const RawData &params, const ActorLink &link) {
		if (0 == PARAM_VALUE.compare(params.toString()))
			link->post(OK_ANSWER);
		else
			link->post(NOK_ANSWER);
		return StatusCode::OK;
	} }
	};

	ensureRegistryStarted(PORT1);
	ensureRegistryStarted(PORT2);
	const std::string name = registry1.addReference("localhost", PORT2);
	if (NAME2.compare(name))
		return 1;
	const Actor a(ACTOR_NAME, ActorCommand(commands, 1) );
	registry2.registerActor(a.getActorLinkRef());
	registry1.registerActor(link);
	const auto actor = registry1.getActor(ACTOR_NAME);
	if (nullptr == actor.get())
		return 1;
	actor->post(DUMMY_COMMAND, PARAM_VALUE, link);
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
	const Actor a(ACTOR_NAME, ActorCommand());
	registry2.registerActor(a.getActorLinkRef());
	const auto actor = registry1.getActor("unknown actor");
	return nullptr == actor.get() ? 0 : 1;
}

static int initSupervisionTest() {
	Actor supervisor("supervisor", ActorCommand());
	Actor supervised("supervised", ActorCommand());
	supervisor.registerActor(supervised);
	supervisor.unregisterActor(supervised);
	return 0;
}


static int unregisterToSupervisorWhenActorDestroyedTest() {
	Actor supervisor("supervisor", ActorCommand());
	auto supervised = std::make_unique<Actor>("supervised", ActorCommand());
	supervisor.registerActor(*supervised.get());

	supervised->post(CommandValue::SHUTDOWN);
	supervised.reset();

	return 0;
}

static int supervisorRestartsActorAfterExceptionTest() {
	static const int RESTART_COMMAND = 0x99;
	static const int OTHER_COMMAND = 0xaa;
	static const int OK_ANSWER = 0x99;
	static bool exceptionThrown = false;
	static bool restarted = false;
	const auto link = std::make_shared<MessageQueue>("queue for reply");
	Actor supervisor("supervisor", ActorCommand());
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			 [](const ActorContext &) { restarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
		{RESTART_COMMAND, [](const RawData &, const ActorLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
		{OTHER_COMMAND, [](const RawData &, const ActorLink &link) { link->post(OK_ANSWER); return StatusCode::OK; }}
	};
	Actor supervised("supervised", ActorCommand(commands, 2), hooks);
	supervisor.registerActor(supervised);
	supervised.post(RESTART_COMMAND, link);
	for(int i = 0; i < 5 && !restarted; i++) sleep(1);
	supervised.post(OTHER_COMMAND, link);
	if (OK_ANSWER != link->get().code)
		return 1;
	return (exceptionThrown && restarted) ? 0 : 1;
}

//LOLO
static int supervisorRestartsActorAfterReturningErrorTest() {
	static const int RESTART_COMMAND = 0x99;
	static const int OTHER_COMMAND = 0xaa;
	static const int OK_ANSWER = 0x99;
	static bool restarted = false;
	const auto link = std::make_shared<MessageQueue>("queue for reply");
	Actor supervisor("supervisor", ActorCommand());
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			 [](const ActorContext &) { restarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
		{RESTART_COMMAND, [](const RawData &, const ActorLink &link) { return StatusCode::ERROR; }},
		{OTHER_COMMAND, [](const RawData &, const ActorLink &link) { link->post(OK_ANSWER); return StatusCode::OK; }}
	};
	Actor supervised("supervised", ActorCommand(commands, 2), hooks);
	supervisor.registerActor(supervised);
	supervised.post(RESTART_COMMAND, link);
	for(int i = 0; i < 5 && !restarted; i++) sleep(1);
	supervised.post(OTHER_COMMAND, link);
	if (OK_ANSWER != link->get().code)
		return 1;
	return (restarted) ? 0 : 1;
}

static int supervisorStopActorTest() {
	static const int STOP_COMMAND = 0x99;
	static bool exceptionThrown = false;
	static bool actorStopped = false;
	const auto link = std::make_shared<MessageQueue>("queue for reply");
	Actor supervisor("supervisor", ActorCommand(), [](ErrorCode) { return StopActor::create(); });
	static const ActorHooks hooks(DEFAULT_START_HOOK, [](const ActorContext&) { actorStopped = true; },
			 						DEFAULT_RESTART_HOOK);
	static const commandMap commands[] = {
		{STOP_COMMAND, [](const RawData &, const ActorLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
	};

	Actor supervised("supervised", ActorCommand(commands, 2), hooks);
	supervisor.registerActor(supervised);
	supervised.post(STOP_COMMAND, link);

	for (int i = 0; i < 5 && !exceptionThrown; i++) sleep(1);
	for (int i = 0; i < 5 && !actorStopped; i++) sleep(1);
	return (exceptionThrown && actorStopped) ? 0 : 1;
}

static int supervisorForwardErrorRestartTest() {
	static const int STOP_COMMAND = 0x99;
	static bool exceptionThrown = false;
	static bool actorRestarted1 = false;
	static bool actorRestarted2 = false;
	static bool actorRestarted3 = false;
	const auto link = std::make_shared<MessageQueue>("queue for reply");
	static const ActorHooks rootHook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[](const ActorContext &c) { actorRestarted1 = true; c.restartActors(); return StatusCode::OK; });
	Actor rootSupervisor("supervisor", ActorCommand(), rootHook, [](ErrorCode) { return RestartActor::create(); });
	static const ActorHooks supervisorHook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[](const ActorContext &c) { actorRestarted2 = true; c.restartActors(); return StatusCode::OK; });
	Actor supervisor("supervisor", ActorCommand(), supervisorHook, [](ErrorCode) { return EscalateError::create(); });
	static const ActorHooks supervisedHooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
	 [](const ActorContext &){ actorRestarted3 = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{STOP_COMMAND, [](const RawData &, const ActorLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
		};

	Actor supervised("supervised", ActorCommand(commands, 1),
	 supervisedHooks);
	rootSupervisor.registerActor(supervisor);
	supervisor.registerActor(supervised);
	supervised.post(STOP_COMMAND, link);

	for (int i = 0; i < 5 && !exceptionThrown; i++) sleep(1);
	for (int i = 0; i < 5 && (!actorRestarted2 || !actorRestarted3); i++) sleep(1);
	return (exceptionThrown && !actorRestarted1 && actorRestarted2 && actorRestarted3) ? 0 : 1;
}

static int supervisorForwardErrorStopTest() {
	static const int STOP_COMMAND = 0x99;
	static bool exceptionThrown = false;
	static bool stopped = false;
	const auto link = std::make_shared<MessageQueue>("queue for reply");
	Actor rootSupervisor("supervisor", ActorCommand(), [](ErrorCode) { return StopActor::create(); });
	Actor supervisor("supervisor", ActorCommand(), [](ErrorCode) { return EscalateError::create(); });
	static const ActorHooks hooks(DEFAULT_START_HOOK, [](const ActorContext &) { stopped = true; }, DEFAULT_RESTART_HOOK);
	static const commandMap commands[] = {
			{STOP_COMMAND, [](const RawData &, const ActorLink &link) -> StatusCode { exceptionThrown = true; throw std::runtime_error("some error"); }},
		};
	Actor supervised("supervised", ActorCommand(commands, 1), hooks);
	rootSupervisor.registerActor(supervisor);
	supervisor.registerActor(supervised);
	supervised.post(STOP_COMMAND, link);

	for (int i = 0; i < 5 && !exceptionThrown && !stopped; i++) sleep(1);
	return exceptionThrown ? 0 : 1;
}

static int actorNotifiesErrorToSupervisorTest() {
	static const int SOME_COMMAND = 0xaa;
	bool supervisorRestarted = false;
	int supervised1Restarted = 0;
	bool supervised2Restarted = false;
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervisorRestarted](const ActorContext &c) { supervisorRestarted = true; return StatusCode::OK; });
	Actor supervisor("supervisor", ActorCommand(), hooks);
	static const ActorHooks supervised1Hook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			 [&supervised1Restarted](const ActorContext &) { supervised1Restarted++; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](const RawData &, const ActorLink &link) { return (Actor::notifyError(0x69), StatusCode::OK); }},
		};

	Actor supervised1("supervised1", ActorCommand(commands, 1), supervised1Hook);
	static const ActorHooks supervised2Hook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervised2Restarted](const ActorContext &) { supervised2Restarted = true; return StatusCode::OK; });
	Actor supervised2("supervised2", ActorCommand(), supervised2Hook);

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	supervised1.post(SOME_COMMAND);
	supervised1.post(SOME_COMMAND);

	for (int i = 0; i < 5 && 2 > supervised1Restarted; i++)
		sleep(1);
	return (supervisorRestarted || ! (2 == supervised1Restarted) || supervised2Restarted) ? 1 : 0;
}

static int actorDoesNothingIfNoSupervisorTest() {
	bool supervisorRestarted = false;
	static const int SOME_COMMAND = 0xaa;
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervisorRestarted](const ActorContext &){ supervisorRestarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](const RawData &, const ActorLink &link) { return (Actor::notifyError(0x69), StatusCode::OK); }},
		};

	auto supervised = std::make_shared<Actor>("supervised", ActorCommand(commands, 1), hooks);

	supervised->post(SOME_COMMAND);
	supervised->post(SOME_COMMAND);

	supervised.reset();
	return !supervisorRestarted ? 0 : 1;
}

static int actorDoesNothingIfNoSupervisorAndExceptionThrownTest() {
	static const int SOME_COMMAND = 0xaa;
	static bool supervisorRestarted = false;
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[](const ActorContext &){ supervisorRestarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](const RawData &, const ActorLink &link) -> StatusCode{ throw std::runtime_error("some error"); }},
		};

	auto supervised = std::make_shared<Actor>("supervised", ActorCommand(commands, 1), hooks);
	supervised->post(SOME_COMMAND);
	supervised->post(SOME_COMMAND);

	supervised.reset();
	return !supervisorRestarted ? 0 : 1;
}

static int restartAllActorBySupervisorTest() {
	static const int SOME_COMMAND = 0xaa;
	bool supervisorRestarted = false;
	bool supervised1Restarted = false;
	bool supervised2Restarted = false;
	static const ActorHooks hooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervisorRestarted](const ActorContext &) { supervisorRestarted = true; return StatusCode::OK; });
	Actor supervisor("supervisor", ActorCommand(), hooks, [](ErrorCode) { return RestartAllActor::create(); });
	static const ActorHooks supervised1Hook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervised1Restarted](const ActorContext &) { supervised1Restarted = true; return StatusCode::OK; });
	static const commandMap commands[] = {
			{SOME_COMMAND, [](const RawData &, const ActorLink &link) { return (Actor::notifyError(0x69), StatusCode::OK); }},
		};
	Actor supervised1("supervised1", ActorCommand(commands, 1), supervised1Hook);
	static const ActorHooks supervised2Hook(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervised2Restarted](const ActorContext &) { supervised2Restarted = true; return StatusCode::OK; });
	Actor supervised2("supervised2", ActorCommand(), supervised2Hook);

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	supervised1.post(SOME_COMMAND);
	supervised1.post(SOME_COMMAND);
	supervised2.post(SOME_COMMAND);

	for (int i = 0; i < 5 && !supervised1Restarted && !supervised2Restarted; i++)
		sleep(1);

	return (supervisorRestarted || !supervised1Restarted || !supervised2Restarted) ? 1 : 0;
}

static int stoppingSupervisorStopsSupervisedTest() {
	static const int STOP_COMMAND = 0x99;
	static bool supervisorStopped = false;
	static bool supervisedStopped = false;
	static const ActorHooks supersivorHooks(DEFAULT_START_HOOK,
			[](const ActorContext&c) { c.stopActors(); supervisorStopped = true; }, DEFAULT_RESTART_HOOK);
	static const commandMap commands[] = {
			{STOP_COMMAND, [](const RawData &, const ActorLink &link) { return StatusCode::SHUTDOWN; }},
		};
	Actor supervisor("supervisor", ActorCommand(commands, 1), supersivorHooks);
	static const ActorHooks supervisedHooks(DEFAULT_START_HOOK, [](const ActorContext&) { supervisedStopped = true; },
			 DEFAULT_RESTART_HOOK);
	Actor supervised("supervised", ActorCommand(), supervisedHooks);
	supervisor.registerActor(supervised);
	supervisor.post(STOP_COMMAND);

	for (int i = 0; i < 5 && !(supervisedStopped && supervisorStopped); i++) sleep(1);
	return (supervisorStopped && supervisedStopped) ? 0 : 1;
}

static int supervisorHasDifferentStrategyDependingOnErrorTest() {
	static const int first_error = 0x11;
	static const int second_error = 0x22;
	static const auto strategy = [](ErrorCode error) {
		if (first_error == error)
			return RestartActor::create();
		else
			return StopActor::create();
	};
	bool supervisorRestarted = false;
	bool supervisedRestarted = false;
	bool supervisedStopped = false;
	static const ActorHooks supervisorHooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK,
			[&supervisorRestarted](const ActorContext &c) { supervisorRestarted = true; return StatusCode::OK; });
	Actor supervisor("supervisor", ActorCommand(), supervisorHooks, strategy);
	static const ActorHooks supervisedHooks(DEFAULT_START_HOOK,
			[&supervisedStopped](const ActorContext &c) { supervisedStopped = true; },
			[&supervisedRestarted](const ActorContext &) { supervisedRestarted++; return StatusCode::OK; });
	static const commandMap commands[] = {
			{first_error, [](const RawData &, const ActorLink &) { return (Actor::notifyError(first_error), StatusCode::OK); }},
			{second_error, [](const RawData &, const ActorLink &) { return (Actor::notifyError(second_error), StatusCode::OK); }},
	};
	Actor supervised("supervised", ActorCommand(commands, 2), supervisedHooks);

	supervisor.registerActor(supervised);

	supervised.post(first_error);
	for (int i = 0; i < 5 && !supervisedRestarted; i++)
		sleep(1);
	supervised.post(second_error);
	for (int i = 0; i < 5 && !supervisedStopped; i++)
		sleep(1);

	return (supervisorRestarted ||  !supervisedRestarted || !supervisedStopped) ? 1 : 0;
}

static int executorTest() {
	MessageQueue messageQueue;
	Executor executor([](MessageType, int, const RawData &, const ActorLink &) { return StatusCode::SHUTDOWN; }, messageQueue);
	messageQueue.post(MessageType::COMMAND_MESSAGE, CommandValue::SHUTDOWN);
	return 0;
}

static int serializationTest() {
	static const uint32_t EXPECTED_VALUE = 0x11223344;

	const RawData s(EXPECTED_VALUE);
	return (EXPECTED_VALUE == s.toId()) ? 0 : 1;
}

static int startActorFailureTest() {
	bool exceptionThrown = false;
	static const ActorHooks hooks([](const ActorContext& ) { return StatusCode::ERROR; },
			DEFAULT_STOP_HOOK, DEFAULT_RESTART_HOOK);
	try {
		Actor actor("supervisor", ActorCommand(), hooks);
	} catch (ActorStartFailure &) {
		exceptionThrown = true;
	}
	return exceptionThrown ? 0 : 1;
}

static int restartActorFailureTest() {
	static const Command COMMAND = 0x33;
	static int actorRestarted = 0;
	static bool actorStoppedProperly = false;
	static const ActorHooks hooks(DEFAULT_START_HOOK,
			[](const ActorContext& ) { actorStoppedProperly = true; },
			[](const ActorContext& ) { actorRestarted++; return StatusCode::ERROR; });
	static const commandMap commands[] = {
			{ COMMAND, [](const RawData &, const ActorLink &) -> StatusCode { throw std::runtime_error("error"); }},
	};

	{
		Actor supervisor("supervisor", ActorCommand(), [](ErrorCode) { return RestartActor::create(); });
		Actor supervised("supervised", ActorCommand(commands, 1), hooks);
		supervisor.registerActor(supervised);
		supervised.post(COMMAND);
		for (int i = 0; i < 5 && 0 == actorRestarted; i++) sleep(1);

	}
	return (1 == actorRestarted && actorStoppedProperly) ? 0 : 1;

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
			TEST(supervisorForwardErrorRestartTest),
			TEST(supervisorForwardErrorStopTest),
		 	TEST(actorNotifiesErrorToSupervisorTest),
			TEST(actorDoesNothingIfNoSupervisorTest),
			TEST(actorDoesNothingIfNoSupervisorAndExceptionThrownTest),
			TEST(restartAllActorBySupervisorTest),
			TEST(stoppingSupervisorStopsSupervisedTest),
			TEST(supervisorHasDifferentStrategyDependingOnErrorTest),
			TEST(executorTest),
			TEST(serializationTest),
			TEST(startActorFailureTest),
			TEST(restartActorFailureTest),
	};

	const auto nbFailure = runTest(suite);
	if (nbFailure > 0)
		return (std::cout << nbFailure << " tests failed." << std::endl, EXIT_FAILURE);
	else
		return (std::cout << "Success!" << std::endl, EXIT_SUCCESS);
}
