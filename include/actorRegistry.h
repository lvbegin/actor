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

#ifndef ACTOR_REGISTRY_H__
#define ACTOR_REGISTRY_H__

#include <actor.h>
#include <proxyContainer.h>
#include <serverSocket.h>
#include <sharedMap.h>
#include <sharedVector.h>

#include <cstdint>
#include <thread>

class ActorRegistry {
public:
	ActorRegistry(std::string name, uint16_t port);
	~ActorRegistry();
	std::string addReference(const std::string &host, uint16_t port);
	void removeReference(const std::string &registryName);
	void registerActor(ActorLink actor);
	void unregisterActor(const std::string &name);
	ActorLink  getActor(const std::string &name) const;
private:
	enum class RegistryCommand : uint32_t { REGISTER_REGISTRY = 0, SEARCH_ACTOR = 1, };
	enum class ActorSearchResult : uint32_t { ACTOR_NOT_FOUND = 0, ACTOR_FOUND = 1, };
	const std::string name;
	const uint16_t port;
	const FindActor findActorCallback;
	bool terminated;
	SharedMap<const std::string, const struct NetAddr> registryAddresses;
	SharedVector<ActorLink> actors;
	ProxyContainer proxies;
	std::thread t;

	void registryBody(const ServerSocket &s);
	ActorLink getLocalActor(const std::string &name) const;
	ActorLink getRemoteActor(const std::string &name) const;
};

#endif
