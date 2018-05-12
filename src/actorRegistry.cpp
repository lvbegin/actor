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

#include <actor/actorRegistry.h>
#include <private/proxyContainer.h>
#include <private/serverSocket.h>
#include <private/sharedMap.h>
#include <private/sharedVector.h>
#include <private/clientSocket.h>
#include <private/proxyClient.h>

#include <iostream>
#include <netinet/in.h>


static void threadBody(uint16_t port, std::function<void(const ServerSocket &s)> body);

class ActorRegistry::ActorRegistryImpl {
	public:
		ActorRegistryImpl(std::string name, uint16_t port) : name(std::move(name)), port(port),
		findActorCallback([this](auto &name) { return this->getRemoteActor(name); }), terminated(false), started(false),
		t([this]() {  threadBody(this->port, 
			[this](const ServerSocket &s) { notifyStarted(); registryBody(s); }); }) 
			{ 
				std::unique_lock<std::mutex> l(m);
				cond_started.wait(l, [this]() { return this->started; });
			}

		~ActorRegistryImpl() {
			terminated = true;
			t.join();
		}
		
		std::string addReference(const std::string &host, uint16_t port)  {
			const auto connection = ClientSocket::openHostConnection(host, port);
			connection.writeInt(RegistryCommand::REGISTER_REGISTRY).writeInt<uint32_t>(this->port).writeString(name);
			const auto otherName = connection.readString();
			registryAddresses.insert(otherName, ClientSocket::toNetAddr(host, port));
			return otherName;
		}

		void removeReference(const std::string &registryName) { registryAddresses.erase(registryName); }

		void registerActor(SharedSenderLink actor) { actors.push_back(std::move(actor)); }

		void unregisterActor(const std::string &name) { actors.erase(SenderApi::nameComparator(name)); }
		
		
		SharedSenderLink  getActor(const std::string &name) const {
			try {
				return getLocalActor(name);
			} catch (const std::out_of_range &e) {
				return getRemoteActor(name);
			}
		}

	private:
		enum class RegistryCommand : uint32_t { REGISTER_REGISTRY = 0, SEARCH_ACTOR = 1, };
		enum class ActorSearchResult : uint32_t { ACTOR_NOT_FOUND = 0, ACTOR_FOUND = 1, };
		const std::string name;
		const uint16_t port;
		const FindActor findActorCallback;
		bool terminated;
		SharedMap<const std::string, const struct NetAddr> registryAddresses;
		SharedVector<SharedSenderLink> actors;
		ProxyContainer proxies;
		std::mutex m;
		bool started;
		std::condition_variable cond_started;
		std::thread t;

	void registryBody(const ServerSocket &s) {
		while (!terminated) {
			try {
				struct NetAddr client_addr;
				Connection connection = s.acceptOneConnection(2, &client_addr);
				switch (connection.readInt<RegistryCommand>()) {
					case RegistryCommand::REGISTER_REGISTRY:
						reinterpret_cast<sockaddr_in *>(&client_addr.ai_addr)->sin_port =
																			htons(connection.readInt<uint32_t>());
						registryAddresses.insert(connection.readString(), client_addr);
						connection.writeString(this->name);
						break;
					case RegistryCommand::SEARCH_ACTOR:
						try {
							auto actor = getLocalActor(connection.readString());
							connection.writeInt(ActorSearchResult::ACTOR_FOUND);
							proxies.createNewProxy(std::move(actor), std::move(connection), findActorCallback);
						} catch (const std::out_of_range &e) {
							connection.writeInt(ActorSearchResult::ACTOR_NOT_FOUND);
						}
						break;
					default:
						std::cerr << "Unknown Registry command in " << __PRETTY_FUNCTION__ << std::endl;
						break;
				}
			}
			catch (const std::exception &e) { }
		}
	}

	void notifyStarted() {
			std::unique_lock<std::mutex> l(m);
			this->started = true;
			cond_started.notify_one();
	}

	SharedSenderLink getLocalActor(const std::string &name) const {
		return actors.find_if(SenderApi::nameComparator(name));
	}

	SharedSenderLink getRemoteActor(const std::string &name) const {
		SharedSenderLink actor;
		registryAddresses.for_each([&actor, &name](const std::pair<const std::string, const struct NetAddr> &c) {
			auto connection = ClientSocket::openHostConnection(c.second);
			connection.writeInt(RegistryCommand::SEARCH_ACTOR).writeString(name);
			if (ActorSearchResult::ACTOR_FOUND == connection.readInt<ActorSearchResult>())
				actor.reset(new ProxyClient(name, std::move(connection)));
		});
		return actor;
	}

};

ActorRegistry::ActorRegistry(std::string name, uint16_t port) : pImpl(new ActorRegistryImpl(std::move(name), port)) { }

ActorRegistry::~ActorRegistry() { delete pImpl; }


std::string ActorRegistry::addReference(const std::string &host, uint16_t port) { return pImpl->addReference(host, port); }
void ActorRegistry::removeReference(const std::string &registryName) { pImpl->removeReference(registryName); }
void ActorRegistry::registerActor(SharedSenderLink actor) { pImpl->registerActor(std::move(actor)); }
void ActorRegistry::registerActor(const SharableSenderApi &actor) { registerActor(actor.getActorLinkRef()); } 
void ActorRegistry::unregisterActor(const std::string &name) { pImpl->unregisterActor(name); }
SharedSenderLink  ActorRegistry::getActor(const std::string &name) const { return pImpl->getActor(name); } 

static void threadBody(uint16_t port, std::function<void(const ServerSocket &s)> body) {
	body(*std::make_unique<ServerSocket>(port));
}
