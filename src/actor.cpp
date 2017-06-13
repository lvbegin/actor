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
#include <commandValue.h>
#include <exception.h>

#include <iostream>

const AtStartHook DEFAULT_START_HOOK = [](const ActorContext&) { return StatusCode::OK; };
const AtStopHook DEFAULT_STOP_HOOK = [](const ActorContext& c) { c.stopActors(); };
const AtRestartHook DEFAULT_RESTART_HOOK = [](const ActorContext& c) { c.restartActors(); return StatusCode::OK; };

static const ActorHooks DEFAULT_HOOKS = ActorHooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK, DEFAULT_RESTART_HOOK);

Actor::ActorException::ActorException(ErrorCode error, const std::string& what_arg) : std::runtime_error(what_arg), error(error) { }
Actor::ActorException::~ActorException() = default;

int Actor::ActorException::getErrorCode() const { return error; }

Actor::Actor(std::string name, ActorBody body, ActionStrategy restartStrategy) :
		Actor(std::move(name), std::move(body), DEFAULT_HOOKS, restartStrategy) { }
Actor::Actor(std::string name, ActorBody body, ActorHooks hooks, ActionStrategy restartStrategy) :
								executorQueue(new MessageQueue(std::move(name))), hooks(hooks),
								body(body), supervisor(restartStrategy, executorQueue),
								executor(createAtStartExecutor())
								{ checkActorInitialization(); }

Actor::~Actor() {
	stateMachine.moveTo(ActorStateMachine::State::STOPPED);
	supervisor.notifySupervisor(CommandValue::UNREGISTER_ACTOR);
	executorQueue->post(CommandValue::SHUTDOWN);
}

void Actor::checkActorInitialization(void) const {
	if (ActorStateMachine::State::STOPPED == this->stateMachine.waitStarted())
		throw ActorStartFailure();
}

void Actor::post(Command command, ActorLink sender) const {
	static const RawData EMPTY_DATA;
	executorQueue->post(command, EMPTY_DATA, std::move(sender));
}

void Actor::post(Command command, const RawData &params, ActorLink sender) const { executorQueue->post(command, params, std::move(sender)); }

StatusCode Actor::restartSateMachine(void) {
	stateMachine.moveTo(ActorStateMachine::State::RESTARTING);

	if (StatusCode::OK == restartExecutor())
		stateMachine.moveTo(ActorStateMachine::State::RUNNING);
	else
		stateMachine.moveTo(ActorStateMachine::State::ERROR);
	return StatusCode::OK;
}

StatusCode Actor::restartExecutor(void) {
	std::promise<StatusCode> status;
	std::promise<std::unique_ptr<Executor> &> e;
	auto newExecutor = createExecutor([this, &status, & e]() { return executorRestartCb(status, e); });
	e.set_value(newExecutor);
	return status.get_future().get();
}

ActorLink Actor::getActorLinkRef() const { return executorQueue; }

void Actor::registerActor(Actor &monitored) { supervisor.registerMonitored(monitored.supervisor); }

void Actor::unregisterActor(Actor &monitored) { supervisor.unregisterMonitored(monitored.supervisor); }

void Actor::notifyError(int e) { throw ActorException(e, "error in actor"); }

StatusCode Actor::actorExecutor(ActorBody body, MessageType type, Command command, const RawData &params, const ActorLink &sender) {
	if (stateMachine.isIn(ActorStateMachine::State::STOPPED))
		return StatusCode::SHUTDOWN;
	switch (type) {
		case MessageType::ERROR_MESSAGE:
			return (supervisor.manageErrorFromSupervised(command, params), StatusCode::OK);
		case MessageType::MANAGEMENT_MESSAGE:
			return executeActorManagement(command, params);
		case MessageType::COMMAND_MESSAGE: {
			const auto status = executeActorBody(body, command, params, sender);
			if (StatusCode::SHUTDOWN == status)
				stateMachine.moveTo(ActorStateMachine::State::STOPPED);
			return status;
		}
		default:
			std::cerr << "Unknown Message type in " << __PRETTY_FUNCTION__ << std::endl;
			return StatusCode::ERROR;
	}
}

StatusCode Actor::executeActorManagement(Command command, const RawData &params) {
	switch (command) {
		case CommandValue::RESTART:
			return (StatusCode::OK == restartSateMachine()) ? StatusCode::SHUTDOWN : StatusCode::ERROR;
		case CommandValue::UNREGISTER_ACTOR:
			supervisor.removeActor(params.toString());
			return StatusCode::OK;
		case CommandValue::SHUTDOWN:
			stateMachine.moveTo(ActorStateMachine::State::STOPPED);
			return StatusCode::SHUTDOWN;
		default:
			std::cerr << "Unknown Command in " << __PRETTY_FUNCTION__ << std::endl;
			return StatusCode::ERROR;
	}
}

StatusCode Actor::executeActorBody(ActorBody body, Command command, const RawData &params, const ActorLink &sender) {
	if ( CommandValue::SHUTDOWN == command )
		return StatusCode::SHUTDOWN;
	try {
		const auto rc = body(command, params, sender);
		if (StatusCode::ERROR == rc)
			supervisor.sendErrorToSupervisor(ACTOR_BODY_FAILED);
		return rc;
	} catch (const ActorException &e) {
		supervisor.sendErrorToSupervisor(e.getErrorCode());
		return StatusCode::ERROR;
	} catch (const std::exception &e) {
		supervisor.sendErrorToSupervisor(ACTOR_BODY_FAILED);
		return StatusCode::ERROR;
	}
}

std::unique_ptr<Executor> Actor::createAtStartExecutor() {
	return createExecutor([this]() { return executorStartCb(hooks.atStart); });
}

std::unique_ptr<Executor> Actor::createExecutor(ExecutorAtStart atStartCb) {
return std::make_unique<Executor>( [this](auto type, auto command, auto &params, auto &sender)
		{ return this->actorExecutor(body, type, command, params, sender); }, *executorQueue,
			atStartCb, [this]() { executorStopCb(); } );
}

StatusCode Actor::executorStartCb(AtStartHook atStart) {
	const auto rc = atStart(this->supervisor);
	if (StatusCode::ERROR == rc)
		this->stateMachine.moveTo(ActorStateMachine::State::STOPPED);
	else
		this->stateMachine.moveTo(ActorStateMachine::State::RUNNING);
	return rc;
}

void Actor::executorStopCb(void) {
	if (stateMachine.isIn(ActorStateMachine::State::STOPPED) ||
			stateMachine.isIn(ActorStateMachine::State::ERROR))
		hooks.atStop(supervisor);
}

StatusCode Actor::executorRestartCb(std::promise<StatusCode> &status, std::promise<std::unique_ptr<Executor> &> &e) {
	std::unique_ptr<Executor> ref(std::move(e.get_future().get()));
	std::swap(executor, ref);
	const auto rc = hooks.atRestart(supervisor);
	status.set_value(rc);
	return  rc;
}
