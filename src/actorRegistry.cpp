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

#include <actorRegistry.h>
#include <clientSocket.h>
#include <proxyClient.h>

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body);

ActorRegistry::ActorRegistry(std::string name, uint16_t port) : name(name), terminated(false),
					t([this, port]() {  threadBody(port, [this](ServerSocket &s) { registryBody(s); }); }) { }

static void threadBody(uint16_t port, std::function<void(ServerSocket &s)> body) {
	auto s = std::make_unique<ServerSocket>(port);
	body(*s);
}

ActorRegistry::~ActorRegistry() {
	terminated = true;
	t.join();
}

void ActorRegistry::registryBody(const ServerSocket &s) {
	while (!terminated) {
		try {
			struct sockaddr_in client_addr {};
			auto connection = s.acceptOneConnection(2, &client_addr);
			switch (connection.readInt<RegistryCommand>()) {
				case RegistryCommand::REGISTER_REGISTRY:
					registryAddresses.insert(connection.readString(), client_addr);
					connection.writeString(this->name);
					break;
				case RegistryCommand::SEARCH_ACTOR:
					try {
						auto actor = actors.find(connection.readString());
						connection.writeInt(ACTOR_FOUND);
						proxies.createNewProxy(actor, std::move(connection));
					} catch (std::out_of_range e) {
						connection.writeInt(ACTOR_NOT_FOUND);
					}
					break;
				default:
					THROW(std::runtime_error, "unknown case.");
			}
		}
		catch (std::exception e) { }
	}
}

std::string ActorRegistry::addReference(std::string host, uint16_t port) {
	auto connection = ClientSocket::openHostConnection(host, port);
	connection.writeInt(RegistryCommand::REGISTER_REGISTRY).writeString(name);
	//should read status...and react in consequence: what if failure returned?
	const std::string otherName = connection.readString();
	registryAddresses.insert(otherName, ClientSocket::toSockAddr(host, port));
	return otherName;
}

void ActorRegistry::removeReference(std::string registryName) { registryAddresses.erase(registryName); }

void ActorRegistry::registerActor(ActorRef actor) { actors.insert(actor->getName(), actor); }

void ActorRegistry::unregisterActor(std::string name) { actors.erase(name); }

GenericActorPtr  ActorRegistry::getActor(std::string name) const {
	try {
		return getLocalActor(name);
	} catch (std::out_of_range e) {
		return getRemoteActor(name);
	}
}

GenericActorPtr ActorRegistry::getLocalActor(const std::string &name) const { return actors.find(name); }

GenericActorPtr ActorRegistry::getRemoteActor(const std::string &name) const {
	GenericActorPtr actor;
	registryAddresses.for_each([&actor, &name](const std::pair<const std::string, struct sockaddr_in> &c) {
		auto connection = ClientSocket::openHostConnection(c.second);
		connection.writeInt(RegistryCommand::SEARCH_ACTOR).writeString(name);
		if (ACTOR_FOUND == connection.readInt<uint32_t>())
			actor.reset(new proxyClient(std::move(connection)));
	});
	return actor;
}
