/* Copyright 2018 Laurent Van Begin
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
#include <private/internalCommands.h>
#include <private/serverSocket.h>
#include <private/executor.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>

static const std::string PARAM_VALUE("Hello World");
static const int OK_ANSWER = 0x22;

static const Command OK_COMMAND = 0xaa | COMMAND_FLAG;

static const std::string REGISTRY_NAME1("registry name1");

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

static void executeServerProxy(uint16_t port, testCommands *commands) {
	const Actor actor(ACTOR_NAME, commands->commands);
	static const auto DO_NOTHING = []() { };
	const auto DummyGetConnection = [] (std::string) { return SharedSenderLink(); };
	const ProxyServer server(actor.getActorLinkRef(), ServerSocket::getConnection(port), DO_NOTHING, DummyGetConnection);
}

static Connection openOneConnection(uint16_t port) {
	while (true) {
		try {
			return ClientSocket::openHostConnection("localhost", port);
		} catch (std::exception &e) { }
	}
}

static void actorCommandHasReservedCodeTest(void) {
	static const uint32_t wrongCommand = 0xaa;
	static const commandMap commands[] = {
			{wrongCommand, [](Context &, const RawData &, const SharedSenderLink &link) { return StatusCode::OK;}},
			{ 0, NULL},
	};
	assert_exception(std::runtime_error, ActorCommand c(commands));
}

static void proxyTest(void) {
	static const uint16_t PORT = 4011;
	int nbMessages { 0 };
	testCommands commands;
	std::thread t(executeServerProxy, PORT, &commands);
	ProxyClient client("client name", openOneConnection(PORT));
	client.post(OK_COMMAND);
	waitCondition([&commands]() { return 1 == commands.commandExecuted; } );
	client.post(InternalCommands::SHUTDOWN);
	t.join();
}

static void registryConnectTest(void) {
	static const uint16_t PORT = 4001;
	const ActorRegistry registry(REGISTRY_NAME1, PORT);
	const Connection c = openOneConnection(PORT);
}


static void executorTest() {
	auto link = Link::create();
	Executor executor([](MessageType, int, const RawData &, const SharedSenderLink &) { return StatusCode::SHUTDOWN; }, *link);
	link->post(MessageType::COMMAND_MESSAGE, InternalCommands::SHUTDOWN);
}

static void serializationTest() {
	static const uint32_t EXPECTED_VALUE = 0x11223344;

	const RawData s(EXPECTED_VALUE);
	assert_eq(EXPECTED_VALUE, toId(s));
}


int main() {
	static const _test suite[] = {
		TEST(actorCommandHasReservedCodeTest),
		TEST(proxyTest),
		TEST(registryConnectTest),
		TEST(executorTest),
		TEST(serializationTest),
	};

	const auto nbFailure = runTest(suite);
	if (nbFailure > 0)
		return (std::cout << nbFailure << " tests failed." << std::endl, EXIT_FAILURE);
	else
		return (std::cout << "Success!" << std::endl, EXIT_SUCCESS);
}
