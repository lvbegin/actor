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

Actor::Actor(ActorBody body, RestartStrategy restartStrategy)  : Actor(body, Actor::doNothing, restartStrategy) {}

Actor::Actor(ActorBody body, std::function<void(void)> atRestart, RestartStrategy restartStrategy) :
						executorQueue(new MessageQueue()), supervisor(std::move(restartStrategy)), atRestart(atRestart), body(body),
						executor(new Executor([this](MessageType type, int command, const std::vector<uint8_t> &params)
								{ return this->actorExecutor(this->body, type, command, params); }, executorQueue.get())) { }

Actor::~Actor() {
	stateMachine.moveTo(ActorStateMachine::ActorState::STOPPED);
	supervisor.notifySupervisor(Command::COMMAND_UNREGISTER_ACTOR);
	executorQueue->post(MessageType::COMMAND_MESSAGE, Command::COMMAND_SHUTDOWN);
};

StatusCode Actor::postSync(int i, std::vector<uint8_t> params) {
	return executorQueue->postSync(MessageType::COMMAND_MESSAGE, i, params);
}

void Actor::post(int i, std::vector<uint8_t> params) {
	executorQueue->post(MessageType::COMMAND_MESSAGE, i, params);
}

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
			[this](MessageType type, int command, const std::vector<uint8_t> &params) {
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

ActorRef Actor::createActorRef(ActorBody body, RestartStrategy restartStragy) {
	return createActorRefWithRestart(body, Actor::doNothing, restartStragy);
}

ActorRef Actor::createActorRefWithRestart(ActorBody body, std::function<void(void)> atRestart,
											RestartStrategy restartStragy) {
	return std::make_unique<Actor>(body, atRestart, restartStragy);
}

LinkApi *Actor::getActorLink() const { return new ActorQueue(executorQueue); }

std::shared_ptr<LinkApi> Actor::getActorLinkRef() const { return std::make_shared<ActorQueue>(executorQueue); }

void Actor::registerActor(ActorRef &monitor, ActorRef &monitored) {
	Supervisor::registerMonitored(monitor->executorQueue, monitor->supervisor, monitored->executorQueue, monitored->supervisor);
}

void Actor::unregisterActor(ActorRef &monitor, ActorRef &monitored) {
	Supervisor::unregisterMonitored(monitor->supervisor, monitored->supervisor);
}

void Actor::notifyError(int e) { throw ActorException(e, "error in actor"); }

StatusCode Actor::actorExecutor(ActorBody body, MessageType type, int code, const std::vector<uint8_t> &params) {
	if (stateMachine.isIn(ActorStateMachine::ActorState::STOPPED))
		return StatusCode::ok;

	if (MessageType::ERROR_MESSAGE == type)
		return (supervisor.doSupervisorOperation(code, params), StatusCode::ok);
	if (Command::COMMAND_RESTART == code) {
		return (StatusCode::ok == restartSateMachine()) ? StatusCode::shutdown : StatusCode::error;
	}
	if (Command::COMMAND_UNREGISTER_ACTOR == code) {
		// check size.
		const uint32_t toRemove { *(uint32_t *)params.data() }; //do better ?
		supervisor.removeSupervised(toRemove);
		return StatusCode::ok;
	}
	return executeActorBody(body, code, params);
}

StatusCode Actor::executeActorBody(ActorBody body, int code, const std::vector<uint8_t> &params) {
	try {
		return body(code, params);
	} catch (ActorException &e) {
		supervisor.sendErrorToSupervisor(e.getErrorCode());
		return StatusCode::error;
	} catch (std::exception &e) {
		supervisor.sendErrorToSupervisor(EXCEPTION_THROWN_ERROR);
		return StatusCode::error;
	}
}
