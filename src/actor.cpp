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
#include <exception.h>

std::function<void(void)> Actor::doNothing = [](void) {};

Actor::Actor(ActorBody body, RestartStrategy restartStrategy)  : Actor(body, Actor::doNothing, restartStrategy) {}

Actor::Actor(ActorBody body, std::function<void(void)> atRestart, RestartStrategy restartStrategy) :
						executorQueue(new MessageQueue()), supervisor(std::move(restartStrategy), executorQueue), atRestart(atRestart), body(body),
						executor(new Executor([this](MessageType type, int command, const RawData &params, const std::shared_ptr<LinkApi> &sender)
								{ return this->actorExecutor(this->body, type, command, params, sender); }, *executorQueue)) { }

Actor::~Actor() {
	stateMachine.moveTo(ActorStateMachine::ActorState::STOPPED);
	supervisor.notifySupervisor(Command::COMMAND_UNREGISTER_ACTOR);
	executorQueue->post(Command::COMMAND_SHUTDOWN);
}

//StatusCode Actor::postSync(int i, ActorLink sender) const { return executorQueue->postSync(i, RawData(), std::move(sender)); }

void Actor::post(int i, ActorLink sender) const { executorQueue->post(i, RawData(), std::move(sender)); }

//StatusCode Actor::postSync(int i, RawData params, ActorLink sender) const { return executorQueue->postSync(i, params, std::move(sender)); }

void Actor::post(int i, RawData params, ActorLink sender) const { executorQueue->post(i, params, std::move(sender)); }

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
			[this](MessageType type, int command, const RawData &params, const std::shared_ptr<LinkApi> &sender) {
				return this->actorExecutor(this->body, type, command, params, sender);
			}, *executorQueue,
			[this, &status, & e]() mutable {
				std::unique_ptr<Executor> ref(std::move(e.get_future().get()));
				std::swap(this->executor, ref);
				this->atRestart();
				status.set_value(StatusCode::ok);
			});
	e.set_value(newExecutor);
	return status.get_future().get();
}

ActorLink Actor::getActorLinkRef() const { return executorQueue; }

void Actor::registerActor(Actor &monitored) { supervisor.registerMonitored(monitored.supervisor); }

void Actor::unregisterActor(Actor &monitored) { supervisor.unregisterMonitored(monitored.supervisor); }

void Actor::notifyError(int e) { throw ActorException(e, "error in actor"); }

StatusCode Actor::actorExecutor(ActorBody body, MessageType type, int code, const RawData &params, const ActorLink &sender) {
	if (stateMachine.isIn(ActorStateMachine::ActorState::STOPPED)) //FIX: race condition when the actor is stopped just after.
		return StatusCode::ok;

	switch (type) {
		case MessageType::ERROR_MESSAGE:
			return (supervisor.doSupervisorOperation(code, params), StatusCode::ok);
		case MessageType::MANAGEMENT_MESSAGE:
			return executeActorManagement(code, params);
		case MessageType::COMMAND_MESSAGE:
			return executeActorBody(body, code, params, sender);
		default:
			THROW(std::runtime_error, "unsupported message type.");
	}
}

StatusCode Actor::executeActorManagement(int code, const RawData &params) {
	switch (code) {
		case Command::COMMAND_RESTART:
			return (StatusCode::ok == restartSateMachine()) ? StatusCode::shutdown : StatusCode::error;
		case Command::COMMAND_UNREGISTER_ACTOR:
			supervisor.removeSupervised(UniqueId::unserialize(params));
			return StatusCode::ok;
		default:
			THROW(std::runtime_error, "unsupported management message code.");
	}
}

StatusCode Actor::executeActorBody(ActorBody body, int code, const RawData &params, const ActorLink &sender) {
	try {
		return body(code, params, sender);
	} catch (ActorException &e) {
		supervisor.sendErrorToSupervisor(e.getErrorCode());
		return StatusCode::error;
	} catch (std::exception &e) {
		supervisor.sendErrorToSupervisor(EXCEPTION_THROWN_ERROR);
		return StatusCode::error;
	}
}
