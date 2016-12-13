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

Actor::Actor(std::string name, ActorBody body)  : name(std::move(name)), body(body), executorQueue(),
executor(new Executor([this, body](MessageQueue::type type, int command, const std::vector<unsigned char> &params)
		{ return this->actorExecutor(body, command, params); }, &executorQueue)) { }

Actor::~Actor() = default;

returnCode Actor::postSync(int i, std::vector<unsigned char> params) {
	return executorQueue.putMessage(MessageQueue::type::COMMAND_MESSAGE, i, params).get();
}

void Actor::post(int i, std::vector<unsigned char> params) {
	executorQueue.putMessage(MessageQueue::type::COMMAND_MESSAGE, i, params);
}

void Actor::restart(void) {
	executor.reset(); //stop the current thread. Ensure that the new thread does not receive the shutdown
	executor.reset(new Executor([this](MessageQueue::type type, int command, const std::vector<unsigned char> &params)
			{ return this->actorExecutor(this->body, command, params); }, &executorQueue));
}

std::string Actor::getName(void) const { return name; }

ActorRef Actor::createActorRef(std::string name, ActorBody body) { return std::make_shared<Actor>(name, body); }

void Actor::registerActor(ActorRef monitor, ActorRef monitored) {
	std::lock(monitor->monitorMutex, monitored->monitorMutex);
    std::lock_guard<std::mutex> l1(monitor->monitorMutex, std::adopt_lock);
    std::lock_guard<std::mutex> l2(monitored->monitorMutex, std::adopt_lock);

    auto tmp = monitored->supervisor;
    monitored->supervisor = std::weak_ptr<Actor>(monitor);
    try {
    	monitor->monitored.addActor(monitored);
    } catch (std::exception e) {
    	monitored->supervisor = tmp;
    	throw e;
    }
}

void Actor::unregisterActor(ActorRef monitor, ActorRef monitored) {
	std::lock(monitor->monitorMutex, monitored->monitorMutex);
    std::lock_guard<std::mutex> l1(monitor->monitorMutex, std::adopt_lock);
    std::lock_guard<std::mutex> l2(monitored->monitorMutex, std::adopt_lock);

    auto tmp = monitored->supervisor;
	monitored->supervisor.reset();

	try {
		monitor->monitored.removeActor(monitored->getName());
	} catch (std::exception e) {
    	monitored->supervisor = tmp;
    	throw e;
	}
}

void Actor::notifyError(int e) { throw std::runtime_error("error"); }

void Actor::postError(int i, const std::string &actorName) {
	executorQueue.putMessage(MessageQueue::type::ERROR_MESSAGE, i, std::vector<unsigned char>(actorName.begin(), actorName.end()));
}
returnCode Actor::actorExecutor(ActorBody body, int command, const std::vector<unsigned char> &params) {
	if (command == 0x69) {
		std::lock_guard<std::mutex> l(monitorMutex);

		monitored.restartOne(std::string(params.begin(), params.end()));
		return returnCode::ok;
	}
	try {
		return body(command, params);
	} catch (std::exception e) {
		ActorRef supervisorRef = supervisor.lock();
		supervisorRef->postError(0x69, name);
		return returnCode::error;
	}
}
