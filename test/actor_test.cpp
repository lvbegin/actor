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
#include <private/commandValue.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>

static const std::string PARAM_VALUE("Hello World");
static const int OK_ANSWER = 0x22;
static const int NOK_ANSWER = 0x73;

static const Command OK_COMMAND = 0xaa | CommandValue::COMMAND_FLAG;
static const Command OK_COMMAND_NO_ANSWER = 0x01 | CommandValue::COMMAND_FLAG;
static const Command OK_COMMAND_CHECK_DATA = 0x33 | CommandValue::COMMAND_FLAG;
static const Command OK_COMMAND_CHECK_NO_DATA = 0xF3 | CommandValue::COMMAND_FLAG;
static const Command STOP_COMMAND = 0x99 | CommandValue::COMMAND_FLAG;
static const Command EXCEPTION_THROWN_COMMAND = 0x13 | CommandValue::COMMAND_FLAG;
static const Command ERROR_NOTIFIED_COMMAND = 0xB3 | CommandValue::COMMAND_FLAG;
static const Command ERROR_NOTIFIED_2_COMMAND = 0xE5 | CommandValue::COMMAND_FLAG;
static const Command ERROR_RETURNED_COMMAND = 0xca | CommandValue::COMMAND_FLAG;

static const std::string REGISTRY_NAME1("registry name1");
static const std::string REGISTRY_NAME2("registry name2");
static const std::string REGISTRY_NAME3("registry name3");

static const std::string ACTOR_NAME("my actor");

class testCommands {
	public:
		testCommands(StatusCode rc = StatusCode::OK) : exceptionThrown(false), commandExecuted(0), preCalled(false), postCalled(false), rc(rc) {
			commandMap c[] = {
	{OK_COMMAND, [this](Context &, const RawData &, const SharedSenderLink &link) {
		this->commandExecuted++; 
		if (nullptr != link.get())
			link->post(OK_ANSWER);
		return StatusCode::OK;
	}},
	{OK_COMMAND_NO_ANSWER, [](Context &, const RawData &, const SharedSenderLink &link) { 
		return StatusCode::OK;
	}},
	{OK_COMMAND_CHECK_DATA, [](Context &, const RawData &params, const SharedSenderLink &link) {
		if (0 == PARAM_VALUE.compare(params.toString()))
			link->post(OK_ANSWER);
		else
			link->post(NOK_ANSWER);
		return StatusCode::OK;
	}},
	{OK_COMMAND_CHECK_NO_DATA, [](Context &, const RawData &params, const SharedSenderLink &link) {
		if (0 == params.size())
			link->post(OK_ANSWER);
		else
			link->post(NOK_ANSWER);
		return StatusCode::OK;
	}},

	{ERROR_RETURNED_COMMAND, [this](Context &, const RawData &, const SharedSenderLink &link) { this->commandExecuted++; return StatusCode::ERROR; }},
	{ EXCEPTION_THROWN_COMMAND, [this](Context &, const RawData &, const SharedSenderLink &) {
					this->exceptionThrown++; 
					throw std::runtime_error("some problem"); 
					return StatusCode::OK; 
	}},
	{STOP_COMMAND, [](Context &, const RawData &, const SharedSenderLink &link) { return StatusCode::SHUTDOWN; }},
	{ERROR_NOTIFIED_COMMAND, [this](Context &, const RawData &, const SharedSenderLink &) { this->commandExecuted++; return (Actor::notifyError(ERROR_NOTIFIED_COMMAND), StatusCode::OK); }},
	{ERROR_NOTIFIED_2_COMMAND, [](Context &, const RawData &, const SharedSenderLink &) { return (Actor::notifyError(ERROR_NOTIFIED_2_COMMAND), StatusCode::OK); }},

				{ 0, NULL},
			};
			for (int i = 0; i < sizeof(c) / sizeof(commandMap); i++)
				commands[i] = c[i];
		executor = CommandExecutor(
		[this](Context &, Command, const RawData &, const SharedSenderLink &) { this->preCalled = true; return this->rc; },
		[this](Context &, Command, const RawData &, const SharedSenderLink &) { this->postCalled = true; },
		commands);
		}
		commandMap commands[10];
		int exceptionThrown;
		int commandExecuted;
		bool preCalled;
		bool postCalled; 
		CommandExecutor executor;
	private:
		StatusCode rc;
};

static void basicActorTest(void) {
	const auto link = Link::create();
	const Actor a(ACTOR_NAME, testCommands().commands);
	a.post(OK_COMMAND, link);
	assert_eq(OK_ANSWER,  link->get().code);
}

static void basicActorWithParamsTest(void) {
	const auto link = Link::create();
	const Actor a(ACTOR_NAME, testCommands().commands);
	a.post(OK_COMMAND_CHECK_DATA, PARAM_VALUE, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void actorSendMessageAndReceiveAnAnswerTest(void) {
	const Actor a(ACTOR_NAME, testCommands().commands);
	auto queue = Link::create();
	a.post(OK_COMMAND_CHECK_DATA, PARAM_VALUE, queue);
	assert_eq(OK_ANSWER, queue->get().code);
}

static void actorSendMessageAndDoNptReceiveAnAnswerTest(void) {
	const Actor a(ACTOR_NAME, testCommands().commands);
	auto queue = Link::create();
	a.post(OK_COMMAND_NO_ANSWER, PARAM_VALUE, queue);
	auto answer = queue->get(2000);
	assert_false(answer.isValid());
}

static void actorSendUnknownCommandAndReceiveErrorCodeTest(void) {
	const Actor a(ACTOR_NAME);
	auto queue = Link::create();
	a.post(OK_COMMAND, PARAM_VALUE, queue);
	assert_eq(UNKNOWN_COMMAND, queue->get().code);
}

static void actorSendUnknownCommandCodeTest(void) {
	const Actor a(ACTOR_NAME);
	a.post(OK_COMMAND, PARAM_VALUE);
}

static void registryAddActorTest(void) {
	static const uint16_t PORT = 4001;
	const Actor a(ACTOR_NAME, testCommands().commands);
	ActorRegistry registry(REGISTRY_NAME1, PORT);

	registry.registerActor(a);
}

static void registryAddActorAndRemoveTest(void) {
	static const uint16_t PORT = 4001;
	const Actor a(ACTOR_NAME);
	ActorRegistry registry(REGISTRY_NAME1, PORT);

	registry.registerActor(a);
	registry.unregisterActor(ACTOR_NAME);
	assert_exception(std::out_of_range, registry.unregisterActor(ACTOR_NAME));

	const Actor anotherA(ACTOR_NAME);
	registry.registerActor(anotherA.getActorLinkRef());
}

static void registryAddReferenceTest(void) {
	static const uint16_t PORT1 = 4001;
	static const uint16_t port2 = 4002;
	ActorRegistry registry1(REGISTRY_NAME1, PORT1);
	ActorRegistry registry2(REGISTRY_NAME2, port2);

	const std::string name = registry1.addReference("localhost", port2);
	assert_eq(REGISTRY_NAME2, name);
}

static void registryAddReferenceOverrideExistingOneTest(void) {
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	static const uint16_t PORT3 = 6003;
	ActorRegistry registry1(REGISTRY_NAME1, PORT1);
	ActorRegistry registry2(REGISTRY_NAME2, PORT2);
	ActorRegistry registry3(REGISTRY_NAME1, PORT3);

	registry1.addReference("localhost", PORT2);
	const std::string name = registry3.addReference("localhost", PORT2);
    assert_eq(REGISTRY_NAME2, name);
}

static void registeryAddActorFindItBackAndSendMessageTest() {
	static const uint16_t PORT = 4001;
	const auto link = Link::create();
	const Actor a(ACTOR_NAME, testCommands().commands);
	const auto actorRefLink = a.getActorLinkRef();
	ActorRegistry registry(REGISTRY_NAME1, PORT);
	registry.registerActor(actorRefLink);

	const auto b = registry.getActor(ACTOR_NAME);

	assert_eq(actorRefLink.get(), b.get());
	b->post(OK_COMMAND, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void registeryFindUnknownActorTest() {
	static const uint16_t PORT = 4001;
	const Actor a(ACTOR_NAME);
	ActorRegistry registry(REGISTRY_NAME1, PORT);
	registry.registerActor(a);

	const SharedSenderLink b = registry.getActor("wrong name");
	assert_eq(nullptr, b.get());
}

static void findActorFromOtherRegistryAndSendMessageTest() {
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	ActorRegistry registry1(REGISTRY_NAME1, PORT1);
	ActorRegistry registry2(REGISTRY_NAME2, PORT2);
	auto link = Link::create("link name needed because the actor is remote");
	const std::string name = registry1.addReference("localhost", PORT2);
	assert_eq (REGISTRY_NAME2, name);
	const Actor a(ACTOR_NAME, CommandExecutor(testCommands().commands) );

	registry1.registerActor(link);
	registry2.registerActor(a);
	const auto actor = registry1.getActor(ACTOR_NAME);
	assert_true (nullptr != actor.get());

	sleep(10); // ensure that the proxy server does not stop after a timeout when no command is sent.
	actor->post(OK_COMMAND_CHECK_NO_DATA, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void findActorFromOtherRegistryAndSendWithSenderForwardToAnotherActorMessageTest() {
	static const std::string ACTOR_NAME1(ACTOR_NAME);
	static const std::string ACTOR_NAME2("my actor 2");
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	static const uint16_t PORT3 = 4003;
	static const Command INTERMEDIATE_COMMAND = 0x44 | CommandValue::COMMAND_FLAG;
	ActorRegistry registry1(REGISTRY_NAME1, PORT1);
	ActorRegistry registry2(REGISTRY_NAME2, PORT2);
	ActorRegistry registry3(REGISTRY_NAME3, PORT3);
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


	registry1.addReference("localhost", PORT2);
	registry1.addReference("localhost", PORT3);
	registry2.addReference("localhost", PORT3);

	const Actor actor2(ACTOR_NAME2, commandsActor2);
	registry3.registerActor(actor2);

	auto linkActor2 = registry3.getActor(ACTOR_NAME2);
	static const commandMap commandsActor1[] = {
			{OK_COMMAND, [linkActor2](Context &, const RawData &params, const SharedSenderLink &link) {
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

	const Actor actor1(ACTOR_NAME1, commandsActor1);
	registry2.registerActor(actor1);
	registry1.registerActor(link);
	const auto actor = registry1.getActor(ACTOR_NAME1);
	assert_false (nullptr == actor.get());

	actor->post(OK_COMMAND, link);
	assert_eq (OK_ANSWER, link->get().code);
}

static void findActorFromOtherRegistryAndSendCommandWithParamsTest() {
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	ActorRegistry registry1(REGISTRY_NAME1, PORT1);
	ActorRegistry registry2(REGISTRY_NAME2, PORT2);
	auto link = Link::create("link name needed because the actor is remote");

	const std::string name = registry1.addReference("localhost", PORT2);
	assert_eq (REGISTRY_NAME2, name);
		
	const Actor a(ACTOR_NAME, testCommands().commands);
	registry2.registerActor(a);
	registry1.registerActor(link);
	const auto actor = registry1.getActor(ACTOR_NAME);
	assert_false(nullptr == actor.get());

	actor->post(OK_COMMAND_CHECK_DATA, PARAM_VALUE, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void findUnknownActorInMultipleRegistryTest() {
	static const uint16_t PORT1 = 4001;
	static const uint16_t PORT2 = 4002;
	ActorRegistry registry1(REGISTRY_NAME1, PORT1);
	ActorRegistry registry2(REGISTRY_NAME2, PORT2);

	const std::string name = registry1.addReference("localhost", PORT2);
	assert_eq(REGISTRY_NAME2, name);
	
	const Actor a(ACTOR_NAME);
	registry2.registerActor(a);
	const auto actor = registry1.getActor("unknown actor");
	assert_eq(nullptr, actor.get());
}

static void initSupervisionTest() {
	Actor supervisor("supervisor");
	Actor supervised("supervised");
	supervisor.registerActor(supervised);
	supervisor.unregisterActor(supervised);
}

static void unregisterToSupervisorWhenActorDestroyedTest() {
	Actor supervisor("supervisor");
	auto supervised = std::make_unique<Actor>("supervised");
	supervisor.registerActor(*supervised.get());

	supervised->post(CommandValue::SHUTDOWN);
	supervised.reset();
}
class TestHooks {
	public:
		TestHooks(StatusCode rcStart = StatusCode::OK, StatusCode rcRestart = StatusCode::OK) : 
			actorStopped(false), actorRestarted(0), rcStart(rcStart), rcRestart(rcRestart), hooks(
			[this](const Context&) { return this->rcStart; },
			[this](const Context& c) { c.stopActors(); this->actorStopped = true; },
			[this](const Context& c) { c.restartActors(); this->actorRestarted++; return this->rcRestart; }	
		) { }
		bool actorStopped;
		int actorRestarted;
		StatusCode rcStart;
		StatusCode rcRestart;
		ActorHooks hooks;
};

static void supervisorRestartsActorAfterExceptionTest() {
	const auto link = Link::create("queue for reply");
	Actor supervisor("supervisor");
	TestHooks hooks;
	testCommands commands;
	Actor supervised("supervised", commands.commands, hooks.hooks);
	supervisor.registerActor(supervised);
	supervised.post(EXCEPTION_THROWN_COMMAND, link);
	waitCondition([&hooks]() { return hooks.actorRestarted; });
	assert_true(commands.exceptionThrown);

	supervised.post(OK_COMMAND, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void supervisorRestartsActorAfterReturningErrorTest() {
	const auto link = Link::create("queue for reply");
	Actor supervisor("supervisor");
	TestHooks hooks;
	Actor supervised("supervised", testCommands().commands, hooks.hooks);
	supervisor.registerActor(supervised);
	supervised.post(ERROR_RETURNED_COMMAND, link);
	waitCondition([&hooks]() { return hooks.actorRestarted; });

	supervised.post(OK_COMMAND, link);
	assert_eq(OK_ANSWER, link->get().code);
}

static void supervisorStopActorTest() {
	const auto link = Link::create("queue for reply");
	Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::stopActor(); });
	TestHooks hooks;
	testCommands commands;
	Actor supervised("supervised", commands.commands, hooks.hooks);
	supervisor.registerActor(supervised);
	supervised.post(EXCEPTION_THROWN_COMMAND, link);

	waitCondition([&commands, &hooks]() { return commands.exceptionThrown && hooks.actorStopped; });
}

static void supervisorStopAllActorsTest() {
	const auto link = Link::create("queue for reply");
	Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::stopAllActor(); });
	TestHooks supervised1Hooks;
	TestHooks supervised2Hooks;
	testCommands commands;
	Actor firstSupervised("firstSupervised", commands.commands, supervised1Hooks.hooks);
	Actor secondSupervised("secondSupervised", CommandExecutor(), supervised2Hooks.hooks);
	supervisor.registerActor(firstSupervised);
	supervisor.registerActor(secondSupervised);
	firstSupervised.post(EXCEPTION_THROWN_COMMAND, link);

	waitCondition([&commands, &supervised1Hooks, &supervised2Hooks]() 
					{ return commands.exceptionThrown && supervised1Hooks.actorStopped && supervised2Hooks.actorStopped; });
}

static void supervisorForwardErrorRestartTest() {
	const auto link = Link::create("queue for reply");
	TestHooks rootHooks;
	TestHooks supervisorHooks;
	TestHooks supervisedHooks;
	Actor rootSupervisor("supervisor", CommandExecutor(), rootHooks.hooks, [](ErrorCode) { return ErrorReactionFactory::restartActor(); });
	Actor supervisor("supervisor", CommandExecutor(), supervisorHooks.hooks, [](ErrorCode) { return ErrorReactionFactory::escalateError(); });
	testCommands commands;
	Actor supervised("supervised", commands.commands, supervisedHooks.hooks);
	rootSupervisor.registerActor(supervisor);
	supervisor.registerActor(supervised);
	supervised.post(EXCEPTION_THROWN_COMMAND, link);

	waitCondition([&commands, &supervisedHooks, &supervisorHooks]() 
			{ return commands.exceptionThrown && supervisedHooks.actorRestarted && supervisorHooks.actorRestarted; });
	assert_false(rootHooks.actorRestarted);
}

static void supervisorForwardErrorStopTest() {
	const auto link = Link::create("queue for reply");
	Actor rootSupervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::stopActor(); });
	Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::escalateError(); });
	TestHooks hooks;
	testCommands commands;
	Actor supervised("supervised", commands.commands, hooks.hooks);
	rootSupervisor.registerActor(supervisor);
	supervisor.registerActor(supervised);
	supervised.post(EXCEPTION_THROWN_COMMAND, link);

	waitCondition([&commands, &hooks]() { return commands.exceptionThrown && hooks.actorStopped; });
}

static void actorNotifiesErrorToSupervisorTest() {
	TestHooks supervisorHooks;
	TestHooks supervised1Hooks;
	TestHooks supervised2Hooks;
	Actor supervisor("supervisor", CommandExecutor(), supervisorHooks.hooks);
	Actor supervised1("supervised1", testCommands().commands, supervised1Hooks.hooks);
	Actor supervised2("supervised2", CommandExecutor(), supervised2Hooks.hooks);

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);
	supervised1.post(ERROR_NOTIFIED_COMMAND);
	supervised1.post(ERROR_NOTIFIED_COMMAND);

	waitCondition([&supervised1Hooks]() { return 2 == supervised1Hooks.actorRestarted; });
	assert_false(supervisorHooks.actorRestarted || supervised2Hooks.actorRestarted);
}

static void noEffectIfErrorNotifiedAndNoSupervisorTest() {
	TestHooks hooks;
	testCommands commands;

	Actor supervised("supervised", commands.commands, hooks.hooks);

	supervised.post(ERROR_NOTIFIED_COMMAND);

	waitCondition([&commands]() { return 1 == commands.commandExecuted; });
	assert_false(hooks.actorRestarted);
	assert_true(hooks.actorStopped);
}

static void noEffectIfActorCommandReturnsErrorAndNoSupervisorTest() {
	TestHooks hooks;
	testCommands commands;
	Actor supervised("supervised", commands.commands, hooks.hooks);
	supervised.post(ERROR_RETURNED_COMMAND);
	waitCondition([&commands]() { return 1 == commands.commandExecuted; });
	assert_false(hooks.actorRestarted);
	assert_true(hooks.actorStopped);
}

static void actorDoesNothingIfNoSupervisorAndExceptionThrownTest() {
	TestHooks hooks;
	auto supervised = std::make_shared<Actor>("supervised", testCommands().commands, hooks.hooks);
	supervised->post(EXCEPTION_THROWN_COMMAND);
	supervised->post(EXCEPTION_THROWN_COMMAND);

	supervised.reset();
	assert_false(hooks.actorRestarted);
}

static void actorDoesNothingIfSupervisorUsesDoNothingErrorStrategyTest() {
	TestHooks hooks;
	testCommands commands;
	Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::doNothing(); });		
	Actor supervised("supervised", commands.commands, hooks.hooks);
	supervisor.registerActor(supervised);
	supervised.post(EXCEPTION_THROWN_COMMAND);
	supervised.post(EXCEPTION_THROWN_COMMAND);
	waitCondition([&commands]() { return 2 == commands.exceptionThrown;});
	assert_false(hooks.actorRestarted);
}

static void restartAllActorBySupervisorTest() {
	TestHooks hooks;
	Actor supervisor("supervisor", CommandExecutor(), hooks.hooks, [](ErrorCode) { return ErrorReactionFactory::restartAllActor(); });
	TestHooks supervised1Hook;
	Actor supervised1("supervised1", testCommands().commands, supervised1Hook.hooks);
	TestHooks supervised2Hook;
	Actor supervised2("supervised2", CommandExecutor(), supervised2Hook.hooks);

	supervisor.registerActor(supervised1);
	supervisor.registerActor(supervised2);

	supervised1.post(ERROR_NOTIFIED_COMMAND);
	supervised1.post(ERROR_NOTIFIED_COMMAND);
	supervised2.post(ERROR_NOTIFIED_COMMAND);

	waitCondition([&supervised1Hook, &supervised2Hook]() { return supervised1Hook.actorRestarted || supervised2Hook.actorRestarted; });
	assert_false(hooks.actorRestarted);
}

static void stoppingSupervisorStopssupervisedTest() {
	TestHooks supersivorHooks;
	Actor supervisor("supervisor", testCommands().commands, supersivorHooks.hooks);
	TestHooks supervisedHooks;
	Actor supervised("supervised", CommandExecutor(), supervisedHooks.hooks);
	supervisor.registerActor(supervised);
	supervisor.post(STOP_COMMAND);
	waitCondition([&supersivorHooks, &supervisedHooks] () { return (supervisedHooks.actorStopped && supersivorHooks.actorStopped); } );
}

static void supervisorHasDifferentStrategyDependingOnErrorTest() {
	static const auto strategy = [](ErrorCode error) {
		if (ERROR_NOTIFIED_COMMAND == error)
			return ErrorReactionFactory::restartActor();
		else
			return ErrorReactionFactory::stopActor();
	};
	TestHooks supervisorHooks;
	Actor supervisor("supervisor", CommandExecutor(), supervisorHooks.hooks, strategy);
	TestHooks supervisedHooks;
	Actor supervised("supervised", testCommands().commands, supervisedHooks.hooks);

	supervisor.registerActor(supervised);

	supervised.post(ERROR_NOTIFIED_COMMAND);
	waitCondition([&supervisedHooks]() { return supervisedHooks.actorRestarted; });
	supervised.post(ERROR_NOTIFIED_2_COMMAND);
	waitCondition([&supervisedHooks]() { return supervisedHooks.actorStopped; });
	assert_false(supervisorHooks.actorRestarted);
}

static void startActorFailureTest() {
	TestHooks hooks(StatusCode::ERROR, StatusCode::OK);
	try {
		Actor actor("supervisor", CommandExecutor(), hooks.hooks);
		assert_true(false);
	} catch (ActorStartFailure &) { }
}

static void restartActorFailureTest() {
	TestHooks hooks(StatusCode::OK, StatusCode::ERROR);
	{
		Actor supervisor("supervisor", CommandExecutor(), [](ErrorCode) { return ErrorReactionFactory::restartActor(); });
		Actor supervised("supervised", testCommands().commands, hooks.hooks);
		supervisor.registerActor(supervised);
		supervised.post(EXCEPTION_THROWN_COMMAND);
		waitCondition([&hooks](){ return 1 == hooks.actorRestarted; });
	}
	assert_true(hooks.actorStopped);
}

static void preAndPostActionCalledTest() {
	testCommands commands;
	Actor actor(ACTOR_NAME, std::move(commands.executor));
	actor.post(OK_COMMAND);
	waitCondition([&commands]() { return (commands.preCalled && commands.postCalled); });
}

static void preActionFailsTest() {
	testCommands commands(StatusCode::ERROR);
	Actor actor(ACTOR_NAME, std::move(commands.executor));
	actor.post(OK_COMMAND);
	waitCondition([&commands]() { return commands.preCalled; });
	assert_false(commands.postCalled); 
	assert_eq(0, commands.commandExecuted);
}

static void commandFailsAndPostActionCalledTest() {
	testCommands commands;
	Actor actor(ACTOR_NAME, std::move(commands.executor));
	actor.post(ERROR_RETURNED_COMMAND);
	waitCondition([&commands](){ return (commands.preCalled && commands.postCalled); });
}

static void commandFailsWithExceptionAndPostActionCalledTest() {
	testCommands commands;
	Actor actor(ACTOR_NAME, std::move(commands.executor));
	actor.post(EXCEPTION_THROWN_COMMAND);
	waitCondition([&commands](){ return (commands.preCalled && commands.postCalled); });
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
	Actor actor(ACTOR_NAME, CommandExecutor(), std::unique_ptr<State>(state));
	waitCondition([state]() { return 1 == state->isInitCalled(); });
}

static void initStateDoneAtRestart() {
	DummyState *state = new DummyState();
	Actor supervisor("supervisor");
	Actor supervised("supervised", testCommands().commands, std::unique_ptr<State>(state));

	supervisor.registerActor(supervised);
	supervised.post(EXCEPTION_THROWN_COMMAND);

	waitCondition([state](){ return 2 == state->isInitCalled(); });
}

int main() {
	static const _test suite[] = {
			TEST(basicActorTest),
			TEST(basicActorWithParamsTest),
			TEST(actorSendMessageAndReceiveAnAnswerTest),
			TEST(actorSendMessageAndDoNptReceiveAnAnswerTest),
			TEST(actorSendUnknownCommandAndReceiveErrorCodeTest),
			TEST(actorSendUnknownCommandCodeTest),
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
