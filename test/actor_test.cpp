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
#include <executor.h> // to remove

#include <cstdlib>
#include <iostream>
#include <unistd.h>


static int basicActorTest(void) {
	std::cout << "basicActorTest" << std::endl;
	Actor a("actor name", [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });
	sleep(2);
	auto val = a.postSync(1);
	if (returnCode::ok != val) {
		std::cout << "post failure" << std::endl;
		return 1;
	}
	return 0;
}

static int basicActorWithParamsTest(void) {
	std::cout << "basicActorWithParamsTest" << std::endl;
	static const std::string paramValue("Hello World");
	std::vector<unsigned char> params(paramValue.begin(), paramValue.end());

	Actor a("actor name", [](int i, const std::vector<unsigned char> &params) {
				if (0 == paramValue.compare(std::string(params.begin(), params.end())))
					return returnCode::ok;
				else
					return returnCode::error;});
	sleep(2);
	auto val = a.postSync(1, params);
	if (returnCode::ok != val) {
		std::cout << "post failure" << std::endl;
		return 1;
	}
	return 0;
}

void executeSeverProxy(uint16_t port) {
	auto actor = std::make_shared<Actor>("actor name", [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });
	auto doNothing = []() { };
	proxyServer server(actor, ServerSocket::getConnection(port), doNothing);
}


static int proxyTest(void) {
	std::cout << "proxyTest" << std::endl;
	static const uint16_t port = 4011;
	std::thread t(executeSeverProxy, port);
	sleep(1);
	proxyClient client(ClientSocket::openHostConnection("localhost", port));
	client.postSync(AbstractActor::COMMAND_SHUTDOWN);
	t.join();
	return 0;
}

static int proxyRestartTest(void) {
	std::cout << "proxyRestartTest" << std::endl;
	static const uint16_t port = 4003;
	static const int command = 0x33;
	std::thread t(executeSeverProxy, port);
	sleep(1);
	proxyClient client(ClientSocket::openHostConnection("localhost", port));
	int NbError = (returnCode::ok == client.postSync(command)) ? 0 : 1;
	client.restart();
	NbError += (returnCode::ok == client.postSync(command)) ? 0 : 1;
	NbError +=  (returnCode::shutdown == client.postSync(AbstractActor::COMMAND_SHUTDOWN)) ? 0 : 1;
	t.join();
	return NbError;
}

static int registryConnectTest(void) {
	std::cout << "registryConnectTest" << std::endl;
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name"), port);
	sleep(1);
	Connection c = ClientSocket::openHostConnection("localhost", port);
	return 0;
}

static int registryAddActorTest(void) {
	std::cout << "registryAddAtorTest" << std::endl;
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name"), port);
	ActorRef a = Actor::createActorRef("my actor", [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });

	registry.registerActor(a);
	return 0;
}

static int registryAddActorAndRemoveTest(void) {
	std::cout << "registryAddActorAndRemoveTest" << std::endl;
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name"), port);
	ActorRef a = Actor::createActorRef("my actor", [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });

	registry.registerActor(a);
	registry.unregisterActor("my actor");
	try {
	    registry.unregisterActor("my actor");
	    return 1;
	} catch (std::runtime_error e) { }
	a = Actor::createActorRef("my actor", [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });
	registry.registerActor(/*"my actor",*/ a);
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

	ActorRef a = Actor::createActorRef(actorName, [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });
	registry.registerActor(a);

	GenericActorPtr b = registry.getActor(actorName);
	return (a.get() == b.get()) ? 0 : 1;
}

static int registeryFindUnknownActorTest() {
	std::cout << "registeryFindUnknownActorTest" << std::endl;

	static const std::string actorName("my actor");
	static const uint16_t port = 6001;
	ActorRegistry registry(std::string("name1"), port);

	ActorRef a = Actor::createActorRef(actorName, [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });
	registry.registerActor(a);

	GenericActorPtr b = registry.getActor(std::string("wrong name"));
	return (nullptr == b.get()) ? 0 : 1;
}

static int findActorFromOtherRegistryTest() {
	std::cout << "findActorFromOtherRegistryTest" << std::endl;
	static const uint32_t dummyCommand = 0x33;
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string actorName("my actor");
	static const uint16_t port1 = 6001;
	static const uint16_t port2 = 6002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);
	sleep(1);
	std::string name = registry1.addReference("localhost", port2);
	ActorRef a = Actor::createActorRef(actorName, [](int i, const std::vector<unsigned char> &params) {
		if (i == dummyCommand && 0 == params.size())
			return returnCode::ok;
		else
			return returnCode::error;} );

	registry2.registerActor(a);
	auto actor = registry1.getActor(actorName);
	if (returnCode::ok != actor->postSync(dummyCommand))
		return 1;
	actor->postSync(AbstractActor::COMMAND_SHUTDOWN);
	return nullptr != actor.get() ? 0 : 1;
}

static int findActorFromOtherRegistryAndSendCommandWithParamsTest() {
	std::cout << "findActorFromOtherRegistryAndSendCommandWithParamsTest" << std::endl;
	static const uint32_t dummyCommand = 0x33;
	static const std::string paramValue("Hello World");
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string actorName("my actor");
	static const uint16_t port1 = 6001;
	static const uint16_t port2 = 6002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);
	sleep(1);
	std::string name = registry1.addReference("localhost", port2);
	ActorRef a = Actor::createActorRef(actorName, [](int i, const std::vector<unsigned char> &params) {
		if (i == dummyCommand && 0 == paramValue.compare(std::string(params.begin(), params.end())))
							return returnCode::ok;
						else
							return returnCode::error;} );
	registry2.registerActor(a);
	auto actor = registry1.getActor(actorName);
	if (returnCode::ok != actor->postSync(dummyCommand, std::vector<unsigned char>(paramValue.begin(), paramValue.end())))
		return 1;
	actor->postSync(AbstractActor::COMMAND_SHUTDOWN);
	return nullptr != actor.get() ? 0 : 1;
}


static int findUnknownActorInMultipleRegistryTest() {
	std::cout << "findUnknownActorInMultipleRegistryTest" << std::endl;
	static const std::string name1("name1");
	static const std::string name2("name2");
	static const std::string actorName("my actor");
	static const uint16_t port1 = 6001;
	static const uint16_t port2 = 6002;
	ActorRegistry registry1(name1, port1);
	ActorRegistry registry2(name2, port2);
	sleep(1);
	std::string name = registry1.addReference("localhost", port2);
	ActorRef a = Actor::createActorRef(actorName, [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });
	registry2.registerActor(a);
	auto actor = registry1.getActor("unknown actor");
	return nullptr == actor.get() ? 0 : 1;
}

static int initSupervisionTest() {
	std::cout << "initSupervisionTest" << std::endl;

	auto supervisor = Actor::createActorRef("supervisor", [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });
	auto supervised = Actor::createActorRef("supervised", [](int i, const std::vector<unsigned char> &params) { /* do something */ return returnCode::ok; });
	Actor::registerActor(supervisor, supervised);
	Actor::unregisterActor(supervisor, supervised);
	return 0;
}

int main() {

	int nbFailure = basicActorTest();
	nbFailure += basicActorWithParamsTest();
	nbFailure += proxyTest();
	nbFailure += proxyRestartTest();
	nbFailure += registryConnectTest();
	nbFailure += registryAddActorTest();
	nbFailure += registryAddActorAndRemoveTest();
	nbFailure += registryAddReferenceTest();
	nbFailure += registeryAddActorAndFindItBackTest();
	nbFailure += registeryFindUnknownActorTest();
	nbFailure += findActorFromOtherRegistryTest();
	nbFailure += findActorFromOtherRegistryAndSendCommandWithParamsTest();
	nbFailure += findUnknownActorInMultipleRegistryTest();
	nbFailure += initSupervisionTest();
	std::cout << ((nbFailure) ? "Failure" : "Success") << std::endl;
	return (nbFailure) ? EXIT_FAILURE : EXIT_SUCCESS;
}
