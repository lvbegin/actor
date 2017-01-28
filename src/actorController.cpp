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
#include <commandValue.h>

#include <algorithm>

ActorController::ActorController() = default;
ActorController::~ActorController() = default;

void ActorController::add(std::shared_ptr<MessageQueue> actorLink) { actors.push_back(std::move(actorLink)); }

void ActorController::remove(const std::string &name) { actors.erase([&name](const auto &e){ return (0 == name.compare(e->getName())); }); }

void ActorController::restartOne(const std::string &name) const {
	try {
		auto actor = actors.find_if([&name](const auto &e){ return (0 == name.compare(e->getName())); });
		restart(actor);
	} catch (std::out_of_range &) { }
}

void ActorController::restartAll(void) const { actors.for_each([](const auto &e) { restart(e);} ); }

void ActorController::restart(const std::shared_ptr<MessageQueue> &link) {
	link->post(MessageType::MANAGEMENT_MESSAGE, CommandValue::RESTART);
}
