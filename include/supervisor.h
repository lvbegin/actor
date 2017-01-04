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

#ifndef SUPERVISOR_H__
#define SUPERVISOR_H__

#include <actorController.h>
#include <messageQueue.h>
#include <command.h>

class Supervisor {
public:
	Supervisor(std::string name, RestartStrategy strategy) : name(std::move(name)), restartStrategy(std::move(strategy)) { }
	~Supervisor() = default;

	void notifySupervisor(uint32_t code) {
		std::unique_lock<std::mutex> l(monitorMutex);

		auto ref = supervisorRef.lock();
		if (nullptr != ref.get())
			ref->post(MessageType::COMMAND_MESSAGE, Command::COMMAND_UNREGISTER_ACTOR, std::vector<unsigned char>(name.begin(), name.end()));
	}

	void sendErrorToSupervisor(uint32_t code) {
		std::unique_lock<std::mutex> l(monitorMutex);

		auto ref = supervisorRef.lock();
		if (nullptr != ref.get())
			ref->post(MessageType::ERROR_MESSAGE, code, std::vector<unsigned char>(name.begin(), name.end()));
	}

	void removeSupervised(const std::string &name) {
		std::unique_lock<std::mutex> l(monitorMutex);

		supervisedRefs.remove(name);
	}

	void doSupervisorOperation(int code, const std::vector<unsigned char> &params) {
		std::unique_lock<std::mutex> l(monitorMutex);

		switch (restartStrategy())
		{
			case RestartType::RESTART_ONE:
				supervisedRefs.restartOne(std::string(params.begin(), params.end()));
				break;
			case RestartType::RESTART_ALL:
				supervisedRefs.restartAll();
				break;
			default:
				/* should escalate to supervisor */
				throw std::runtime_error("unexpected case.");

		}
	}

	static void registerMonitored(const std::shared_ptr<MessageQueue> &monitorQueue, Supervisor &monitor, const std::shared_ptr<MessageQueue> &monitoredQueue, Supervisor &monitored) {
		doRegistrationOperation(monitor, monitorQueue, monitored, [&monitor, &monitored, &monitoredQueue](void) { monitor.supervisedRefs.add(monitored.name, monitoredQueue); } );
	}

	static void unregisterMonitored(Supervisor &monitor, Supervisor &monitored) {
		static const std::shared_ptr<MessageQueue> noQueue {};
		doRegistrationOperation(monitor, noQueue, monitored, [&monitor, &monitored](void) { monitor.supervisedRefs.remove(monitored.name); } );
	}

	static void doRegistrationOperation(const Supervisor &monitor, const std::shared_ptr<MessageQueue> &monitorQueue, Supervisor &monitored, std::function<void(void)> op) {
		std::lock(monitor.monitorMutex, monitored.monitorMutex);
	    std::lock_guard<std::mutex> l1(monitor.monitorMutex, std::adopt_lock);
	    std::lock_guard<std::mutex> l2(monitored.monitorMutex, std::adopt_lock);

	    auto tmp = std::move(monitored.supervisorRef);
	    monitored.supervisorRef = std::weak_ptr<MessageQueue>(monitorQueue);
	    try {
	    	op();
	    } catch (std::exception &e) {
	    	monitored.supervisorRef = std::move(tmp);
	    	throw e;
	    }
	}

private:
	mutable std::mutex monitorMutex;
	const std::string name;
	const RestartStrategy restartStrategy;
	ActorController supervisedRefs;
	std::weak_ptr<MessageQueue> supervisorRef;
};

#endif
