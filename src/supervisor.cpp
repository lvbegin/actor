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

#include <supervisor.h>
#include <uniqueId.h>

Supervisor::Supervisor(RestartStrategy strategy, std::shared_ptr<MessageQueue> self) : id(UniqueId::newId()),
						restartStrategy(std::move(strategy)), self(std::move(self)) { }
Supervisor::~Supervisor() = default;

void Supervisor::notifySupervisor(uint32_t code) const{ sendToSupervisor(MessageType::COMMAND_MESSAGE, code); }

void Supervisor::sendErrorToSupervisor(uint32_t code) const { sendToSupervisor(MessageType::ERROR_MESSAGE, code); }

void Supervisor::sendToSupervisor(MessageType type, uint32_t code) const {
	std::unique_lock<std::mutex> l(monitorMutex);

	auto ref = supervisorRef.lock();
	if (nullptr != ref.get())
		ref->post(type, code, UniqueId::serialize(id));
}

void Supervisor::removeSupervised(Id id) {
	std::unique_lock<std::mutex> l(monitorMutex);

	supervisedRefs.remove(id);
}

void Supervisor::doSupervisorOperation(int code, const RawData &params) {
	std::unique_lock<std::mutex> l(monitorMutex);

	switch (restartStrategy())
	{
		case RestartType::RESTART_ONE:
			supervisedRefs.restartOne(UniqueId::unserialize(params));
			break;
		case RestartType::RESTART_ALL:
			supervisedRefs.restartAll();
			break;
		default:
			/* should escalate to supervisor */
			throw std::runtime_error("unexpected case.");
	}
}

void Supervisor::registerMonitored(Supervisor &monitored) {
	doRegistrationOperation(monitored, [this, &monitored](void) { this->supervisedRefs.add(monitored.id, monitored.self); } );
}

void Supervisor::unregisterMonitored(Supervisor &monitored) {
	doRegistrationOperation(monitored, [this, &monitored](void) { this->supervisedRefs.remove(monitored.id); } );
}

void Supervisor::doRegistrationOperation(Supervisor &monitored, std::function<void(void)> op) const {
	std::lock(this->monitorMutex, monitored.monitorMutex);
	std::lock_guard<std::mutex> l1(this->monitorMutex, std::adopt_lock);
	std::lock_guard<std::mutex> l2(monitored.monitorMutex, std::adopt_lock);

	auto tmp = std::move(monitored.supervisorRef);
	monitored.supervisorRef = std::weak_ptr<MessageQueue>(this->self);
	try {
		op();
	} catch (std::exception &e) {
		monitored.supervisorRef = std::move(tmp);
		throw e;
	}
}
