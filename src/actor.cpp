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
#include <command.h>

std::function<void(void)> Actor::doNothing = [](void) {};

Actor::Actor(std::string name, ActorBody body, RestartStrategy restartStrategy)  : Actor(name, body, Actor::doNothing, restartStrategy) {}

Actor::Actor(std::string name, ActorBody body, std::function<void(void)> atRestart, RestartStrategy restartStrategy) :
						name(std::move(name)), executorQueue(new MessageQueue()), restartStrategy(restartStrategy), atRestart(atRestart), body(body),
						executor(new Executor([this](MessageType type, int command, const std::vector<unsigned char> &params)
								{ return this->actorExecutor(this->body, type, command, params); }, executorQueue.get())) { }

Actor::~Actor() { /* we must also unregister the actor to the suppervisor */
	stateMachine.moveTo(ActorStateMachine::ActorState::STOPPED);
	executorQueue->post(MessageType::COMMAND_MESSAGE, Command::COMMAND_SHUTDOWN);
};

StatusCode Actor::postSync(int i, std::vector<unsigned char> params) {
	return executorQueue->postSync(MessageType::COMMAND_MESSAGE, i, params);
}

void Actor::post(int i, std::vector<unsigned char> params) {
	executorQueue->post(MessageType::COMMAND_MESSAGE, i, params);
}

void Actor::restart(void) { executorQueue->post(MessageType::COMMAND_MESSAGE, Command::COMMAND_RESTART); }

StatusCode Actor::restartSateMachine(void) {
	stateMachine.moveTo(ActorStateMachine::ActorState::RESTARTING);
	const auto rc = doRestart();
	stateMachine.moveTo(ActorStateMachine::ActorState::RUNNING);
	return rc;
}


StatusCode Actor::doRestart(void) {
	auto status = std::promise<StatusCode>();
	auto e = std::promise<std::unique_ptr<Executor> &>();
	std::unique_ptr<Executor> newExecutor = std::make_unique<Executor>(
			[this](MessageType type, int command, const std::vector<unsigned char> &params) {
				return this->actorExecutor(this->body, type, command, params);
			}, executorQueue.get(),
			[this, &status, & e]() mutable {
				std::unique_ptr<Executor> ref(std::move(e.get_future().get()));
				std::swap(this->executor, ref);
				this->atRestart();
				status.set_value(StatusCode::ok);
			});
	e.set_value(newExecutor);
	return status.get_future().get();
}

std::string Actor::getName(void) const { return name; }

ActorRef Actor::createActorRef(std::string name, ActorBody body, RestartStrategy restartStragy) {
	return createActorRefWithRestart(name, body, Actor::doNothing, restartStragy);
}

ActorRef Actor::createActorRefWithRestart(std::string name, ActorBody body, std::function<void(void)> atRestart,
											RestartStrategy restartStragy) {
	return std::make_unique<Actor>(name, body, atRestart, restartStragy);
}

LinkApi *Actor::getActorLink() { return new ActorQueue(executorQueue); }

std::shared_ptr<LinkApi> Actor::getActorLinkRef() { return std::make_shared<ActorQueue>(executorQueue); }


void Actor::registerActor(ActorRef &monitor, ActorRef &monitored) {
	doRegistrationOperation(monitor, monitored, [&monitor, &monitored](void) { monitor->monitored.add(monitored->getName(), monitored->executorQueue); } );
}

void Actor::unregisterActor(ActorRef &monitor, ActorRef &monitored) {
	doRegistrationOperation(monitor, monitored, [&monitor, &monitored](void) { monitor->monitored.remove(monitored->getName()); } );
}

void Actor::doRegistrationOperation(ActorRef &monitor, ActorRef &monitored, std::function<void(void)> op) {
	std::lock(monitor->monitorMutex, monitored->monitorMutex);
    std::lock_guard<std::mutex> l1(monitor->monitorMutex, std::adopt_lock);
    std::lock_guard<std::mutex> l2(monitored->monitorMutex, std::adopt_lock);

    auto tmp = std::move(monitored->supervisor);
    monitored->supervisor = std::weak_ptr<MessageQueue>(monitor->executorQueue);
    try {
    	op();
    } catch (std::exception &e) {
    	monitored->supervisor = std::move(tmp);
    	throw e;
    }
}

void Actor::notifyError(int e) { throw ActorException(e, "error in actor"); }

StatusCode Actor::actorExecutor(ActorBody body, MessageType type, int code, const std::vector<unsigned char> &params) {
	if (stateMachine.isIn(ActorStateMachine::ActorState::STOPPED))
		return StatusCode::ok;

	if (MessageType::ERROR_MESSAGE == type)
		return doSupervisorOperation(code, params);
	if (Command::COMMAND_RESTART == code) {
		return (StatusCode::ok == restartSateMachine()) ? StatusCode::shutdown : StatusCode::error;
	}
	return executeActorBody(body, code, params);
}

StatusCode Actor::executeActorBody(ActorBody body, int code, const std::vector<unsigned char> &params) {
	try {
		return body(code, params);
	} catch (ActorException &e) {
		const auto supervisorRef = supervisor.lock();
		if (nullptr != supervisorRef.get())
			supervisorRef->post(MessageType::ERROR_MESSAGE,e.getErrorCode(), std::vector<unsigned char>(name.begin(), name.end()));
		return StatusCode::error;
	} catch (std::exception &e) {
		const auto supervisorRef = supervisor.lock();
		if (nullptr != supervisorRef.get())
			supervisorRef->post(MessageType::ERROR_MESSAGE, EXCEPTION_THROWN_ERROR, std::vector<unsigned char>(name.begin(), name.end()));
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
