/* Copyright 2017 Laurent Van Begin
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

#include <actorController.h>
#include <exception.h>
#include <command.h>

#include <algorithm>

ActorController::ActorController() = default;
ActorController::~ActorController() = default;

void ActorController::add(Id id, std::shared_ptr<MessageQueue> actorLink) {
	std::unique_lock<std::mutex> l(mutex);

	actors.insert(std::pair<Id, std::shared_ptr<MessageQueue>>(id, std::move(actorLink)));
}
void ActorController::remove(Id id) {
	std::unique_lock<std::mutex> l(mutex);

	actors.erase(id);
}
void ActorController::restartOne(Id id) const {
	std::unique_lock<std::mutex> l(mutex);

	auto it = actors.find(id);
	if (actors.end() != it)
		restart(it->second);
}

void ActorController::restartAll(void) const {
	std::unique_lock<std::mutex> l(mutex);

	std::for_each(actors.begin(), actors.end(),
			[](const std::pair<const Id, const std::shared_ptr<MessageQueue>> &e) { restart(e.second);} );
}

void ActorController::restart(const std::shared_ptr<MessageQueue> &link) { link->post(Command::COMMAND_RESTART); }
