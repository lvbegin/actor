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

#ifndef ACTOR_CONTROLLER_H__
#define ACTOR_CONTROLLER_H__

#include <actor/link.h>
#include <private/controllerApi.h>
#include <private/sharedVector.h>
#include <private/commandValue.h>

using LinkRefOperation = std::function<void(const SharedLink &)>;

class ActorController : public ControllerApi {
public:
	ActorController();
	~ActorController();

	void add(SharedLink actorLink);
	void remove(const std::string &name);
	void stopOne(const std::string &name) const override;
	void stopAll(void) const override;
	void restartOne(const std::string &name) const override;
	void restartAll(void) const override;
private:
	SharedVector<SharedLink> actors;

	void doOperationOneActor(const std::string &name, LinkRefOperation op) const;
	void doOperationAllActors(LinkRefOperation op) const;
	static void restart(const SharedLink &link);
	static void stop(const SharedLink &link);
	static void sendMessage(const SharedLink &link, Command command);
};

#endif
