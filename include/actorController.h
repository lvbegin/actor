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

#include <exception.h>
#include <command.h>
#include <messageQueue.h>

#include <algorithm>
#include <mutex>
#include <map>
#include <functional>

class ActorController {
public:
	ActorController() = default;
	~ActorController() = default;

	void add(std::string name, std::shared_ptr<MessageQueue> actorLink) {
		std::unique_lock<std::mutex> l(mutex);

		actors.insert(std::pair<std::string, std::shared_ptr<MessageQueue>>(std::move(name), std::move(actorLink)));
	}
	void remove(const std::string &name) {
		std::unique_lock<std::mutex> l(mutex);

		actors.erase(name);
	}
	void restartOne(const std::string &name) const {
		std::unique_lock<std::mutex> l(mutex);

		auto it = actors.find(name);
		if (actors.end() != it)
			restart(it->second);
	}
	void restartAll() const {
		std::unique_lock<std::mutex> l(mutex);

		std::for_each(actors.begin(), actors.end(), [](const std::pair<std::string, std::shared_ptr<MessageQueue>> &e) { restart(e.second);} );
	}
private:
	mutable std::mutex mutex;
	std::map<std::string, std::shared_ptr<MessageQueue>> actors;
	static void restart(const std::shared_ptr<MessageQueue> &link) { link->post(MessageType::COMMAND_MESSAGE, Command::COMMAND_RESTART); }
};

#endif
