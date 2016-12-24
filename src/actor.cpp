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
#include <actorException.h>

std::function<void(void)> Actor::doNothing = [](void) {};

Actor::Actor(std::string name, ActorBody body, RestartStrategy restartStrategy)  : Actor(name, body, Actor::doNothing, restartStrategy) {}

Actor::Actor(std::string name, ActorBody body, std::function<void(void)> atRestart, RestartStrategy restartStrategy) :
						name(std::move(name)), restartStrategy(restartStrategy), atRestart(atRestart), body(body), executorQueue(),
						executor(new Executor([this](MessageQueue::type type, int command, const std::vector<unsigned char> &params)
								{ return this->actorExecutor(this->body, type, command, params); }, &executorQueue)) { }

Actor::~Actor() {
	stateMachine.moveTo(ActorStateMachine::ActorState::STOPPED);
	executorQueue.put(MessageQueue::type::COMMAND_MESSAGE, Executor::COMMAND_SHUTDOWN);
};

StatusCode Actor::postSync(int i, std::vector<unsigned char> params) {
	return executorQueue.putSync(MessageQueue::type::COMMAND_MESSAGE, i, params);
}

void Actor::post(int i, std::vector<unsigned char> params) {
	executorQueue.put(MessageQueue::type::COMMAND_MESSAGE, i, params);
}

void Actor::restart(void) { executorQueue.put(MessageQueue::type::COMMAND_MESSAGE, COMMAND_RESTART); }

StatusCode Actor::restartMessage(void) {
	stateMachine.moveTo(ActorStateMachine::ActorState::RESTARTING); //move outside

	auto status = std::promise<StatusCode>();
	auto e = std::promise<std::unique_ptr<Executor> &>();
	std::unique_ptr<Executor> newExecutor = std::make_unique<Executor>(
			[this](MessageQueue::type type, int command, const std::vector<unsigned char> &params) {
				return this->actorExecutor(this->body, type, command, params);
			}, &executorQueue,
			[this, &status, & e]() mutable {
				std::unique_ptr<Executor> &ref = e.get_future().get();
				std::unique_ptr<Executor> ref2 = std::move(ref);
				std::swap(this->executor, ref2);
				this->atRestart();
				status.set_value(StatusCode::ok); //condition variable ?
			});
	e.set_value(newExecutor);
	auto r = status.get_future().get();

	stateMachine.moveTo(ActorStateMachine::ActorState::RUNNING); //move outside
 	return r;
}

std::string Actor::getName(void) const { return name; }

ActorRef Actor::createActorRef(std::string name, ActorBody body, RestartStrategy restartStragy) {
	return createActorRefWithRestart(name, body, Actor::doNothing, restartStragy);
}

ActorRef Actor::createActorRefWithRestart(std::string name, ActorBody body, std::function<void(void)> atRestart,
											RestartStrategy restartStragy) {
	return std::make_shared<Actor>(name, body, atRestart, restartStragy);
}


void Actor::registerActor(ActorRef monitor, ActorRef monitored) {
	doRegistrationOperation(monitor, monitored, [&monitor, &monitored](void) { monitor->monitored.addActor(monitored); } );
}

void Actor::unregisterActor(ActorRef monitor, ActorRef monitored) {
	doRegistrationOperation(monitor, monitored, [&monitor, &monitored](void) { monitor->monitored.removeActor(monitored->getName()); } );
}

void Actor::doRegistrationOperation(ActorRef &monitor, ActorRef &monitored, std::function<void(void)> op) {
	std::lock(monitor->monitorMutex, monitored->monitorMutex);
    std::lock_guard<std::mutex> l1(monitor->monitorMutex, std::adopt_lock);
    std::lock_guard<std::mutex> l2(monitored->monitorMutex, std::adopt_lock);

    auto tmp = std::move(monitored->supervisor);
    monitored->supervisor = std::weak_ptr<Actor>(monitor);
    try {
    	op();
    } catch (std::exception e) {
    	monitored->supervisor = std::move(tmp);
    	throw e;
    }
}

void Actor::notifyError(int e) { throw ActorException(e, "error in actor"); }

void Actor::postError(int i, const std::string &actorName) {
	executorQueue.put(MessageQueue::type::ERROR_MESSAGE, i, std::vector<unsigned char>(actorName.begin(), actorName.end()));
}

StatusCode Actor::actorExecutor(ActorBody body, MessageQueue::type type, int code, const std::vector<unsigned char> &params) {
	/* check that we are indeed in running state */
	if (MessageQueue::type::ERROR_MESSAGE == type)
		return doSupervisorOperation(code, params);
	if (COMMAND_RESTART == code) {
		return (StatusCode::ok == restartMessage()) ? StatusCode::shutdown : StatusCode::error;
	}
	return executeActorBody(body, code, params);
}

StatusCode Actor::executeActorBody(ActorBody body, int code, const std::vector<unsigned char> &params) {
	try {
		return body(code, params);
	} catch (ActorException e) {
		const auto supervisorRef = supervisor.lock();
		if (nullptr != supervisorRef.get())
			supervisorRef->postError(e.getErrorCode(), name);
		return StatusCode::error;
	} catch (std::exception e) {
		const auto supervisorRef = supervisor.lock();
		if (nullptr != supervisorRef.get())
			supervisorRef->postError(EXCEPTION_THROWN_ERROR, name);
		return StatusCode::error;
	}
}

StatusCode Actor::doSupervisorOperation(int code, const std::vector<unsigned char> &params) {
	std::lock_guard<std::mutex> l(monitorMutex);

	switch (restartStrategy())
	{
		case RestartType::RESTART_ONE:
			monitored.restartOne(std::string(params.begin(), params.end()));
			break;
		case RestartType::RESTART_ALL:
			monitored.restartAll();
			break;
		default:
			/* should escalate to supervisor */
			throw std::runtime_error("unexpected case.");

	}
	return StatusCode::ok;
}
