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

#include <actor/actor.h>

#include <private/commandValue.h>
#include <private/exception.h>
#include <private/actorController.h>
#include <private/executor.h>
#include <private/actorStateMachine.h>
#include <private/actorContext.h>

#include <mutex>
#include <future>
#include <memory>
#include <iostream>
#include <exception>


class ActorException : public std::runtime_error {
	public:
		ActorException(ErrorCode error, const std::string& what_arg) : std::runtime_error(what_arg),
																						error(error) { }
		~ActorException() = default;

		int getErrorCode() const { return error; }
	private:
		const ErrorCode error;
};


class Actor::ActorImpl {
public:
	ActorImpl(std::string name, CommandExecutor commandExecutor, ActorHooks hooks, std::unique_ptr<State> state,
			ErrorActionDispatcher errorDispatcher) :
				executorQueue(std::make_shared<Link>(std::move(name))),
				hooks(hooks), commandExecutor(std::move(commandExecutor)),
				context(errorDispatcher, executorQueue,	std::move(state)),
				executor(createAtStartExecutor())
				{ checkActorInitialization(); }
	~ActorImpl() {
		stateMachine.moveTo(ActorStateMachine::State::STOPPED);
		context.getConstSupervisor().notifySupervisor(CommandValue::UNREGISTER_ACTOR);
		executorQueue->post(CommandValue::SHUTDOWN);
	}

	std::unique_ptr<Executor> createAtStartExecutor() {
		return createExecutor([this]() { return executorStartCb(hooks.atStart); });
	}


	StatusCode executorStartCb(AtStartHook atStart) {
		initContextState();
		const auto rc = atStart(this->context);
		const auto nextState = (StatusCode::ERROR == rc) ? ActorStateMachine::State::STOPPED :
															ActorStateMachine::State::RUNNING;
		return (this->stateMachine.moveTo(nextState), rc);
	}

	StatusCode executorRestartCb(std::promise<StatusCode> &status, std::promise<std::unique_ptr<Executor> &> &e) {
		std::unique_ptr<Executor> ref(std::move(e.get_future().get()));
		std::swap(executor, ref);
		initContextState();
		const auto rc = hooks.atRestart(context);
		status.set_value(rc);
		return  rc;
	}

	void executorStopCb(void) {
		static const ActorStateMachine::State stateValues [] = { ActorStateMachine::State::STOPPED,
																ActorStateMachine::State::ERROR };
		static const std::vector<ActorStateMachine::State> states(stateValues, stateValues + sizeof(stateValues) / sizeof(stateValues[0]));
		if (stateMachine.isIn(states))
			hooks.atStop(context);
	}

	void initContextState() { context.getState().init(executorQueue->getName()); }


	StatusCode actorExecutor(MessageType type, Command command, const RawData &params, const SenderLink &sender) {
		static const ActorStateMachine::State stoppedValue[] = { ActorStateMachine::State::STOPPED };
		static const std::vector<ActorStateMachine::State> stopped(stoppedValue, stoppedValue +
																			sizeof(stoppedValue) / sizeof(stoppedValue[0]));
		if (stateMachine.isIn(stopped))
			return StatusCode::SHUTDOWN;
		switch (type) {
			case MessageType::ERROR_MESSAGE:
				return (context.getConstSupervisor().manageErrorFromSupervised(command, params), StatusCode::OK);
			case MessageType::MANAGEMENT_MESSAGE:
				return executeActorManagement(command, params);
			case MessageType::COMMAND_MESSAGE: {
				const auto status = executeActorBody(command, params, sender);
				if (StatusCode::SHUTDOWN == status)
					stateMachine.moveTo(ActorStateMachine::State::STOPPED);
				return status;
			}
			default:
				std::cerr << "Unknown Message type in " << __PRETTY_FUNCTION__ << std::endl;
				return StatusCode::ERROR;
		}
	}


	StatusCode executeActorManagement(Command command, const RawData &params) {
		switch (command) {
			case CommandValue::RESTART:
				return (StatusCode::OK == restartSateMachine()) ? StatusCode::SHUTDOWN : StatusCode::ERROR;
			case CommandValue::UNREGISTER_ACTOR:
				context.getSupervisor().removeActor(params.toString());
				return StatusCode::OK;
			case CommandValue::SHUTDOWN:
				stateMachine.moveTo(ActorStateMachine::State::STOPPED);
				return StatusCode::SHUTDOWN;
			default:
				std::cerr << "Unknown Command in " << __PRETTY_FUNCTION__ << std::endl;
				return StatusCode::ERROR;
		}
	}

	StatusCode restartSateMachine(void) {
		stateMachine.moveTo(ActorStateMachine::State::RESTARTING);
	    const auto nextState = (StatusCode::OK == restartExecutor()) ? ActorStateMachine::State::RUNNING :
	    															ActorStateMachine::State::ERROR;
		return (stateMachine.moveTo(nextState), StatusCode::OK);
	}

	StatusCode restartExecutor(void) {
		std::promise<StatusCode> status;
		std::promise<std::unique_ptr<Executor> &> e;
		auto newExecutor = createExecutor([this, &status, & e]() { return executorRestartCb(status, e); });
		e.set_value(newExecutor);
		return status.get_future().get();
	}

	std::unique_ptr<Executor> createExecutor(ExecutorAtStart atStartCb) {
	return std::make_unique<Executor>( [this](auto type, auto command, auto &params, auto &sender)
			{ return this->actorExecutor(type, command, params, sender); }, *executorQueue,
				atStartCb, [this]() { executorStopCb(); } );
	}

	StatusCode executeActorBody(Command command, const RawData &params, const SenderLink &sender) {
		try {
			const auto rc = commandExecutor.execute(context, command, params, sender);
			if (StatusCode::ERROR != rc)
				return rc;
			throw std::runtime_error("Actor command terminated on error.");
		} catch (const ActorException &e) {
			return (context.getConstSupervisor().sendErrorToSupervisor(e.getErrorCode())) ?
					StatusCode::ERROR : StatusCode::SHUTDOWN;
		} catch (const std::exception &e) {
			return (context.getConstSupervisor().sendErrorToSupervisor(ACTOR_BODY_FAILED)) ?
					StatusCode::ERROR : StatusCode::SHUTDOWN;;
		}
	}

	void checkActorInitialization(void) const {
		if (ActorStateMachine::State::STOPPED == stateMachine.waitStarted())
			throw ActorStartFailure();
	}

	const LinkRef executorQueue;
	const ActorHooks hooks;
	const CommandExecutor commandExecutor;
	ActorContext context;
	ActorStateMachine stateMachine;
	std::unique_ptr<Executor> executor;
};

const AtStartHook DEFAULT_START_HOOK = [](const Context&) { return StatusCode::OK; };
const AtStopHook DEFAULT_STOP_HOOK = [](const Context& c) { c.stopActors(); };
const AtRestartHook DEFAULT_RESTART_HOOK = [](const Context& c) {
	c.restartActors();
	return StatusCode::OK;
};
const ErrorActionDispatcher DEFAULT_ERROR_DISPATCHER = [](ErrorCode) { return RestartActor::create(); };
static const ActorHooks DEFAULT_HOOKS = ActorHooks(DEFAULT_START_HOOK, DEFAULT_STOP_HOOK, DEFAULT_RESTART_HOOK);


Actor::Actor(std::string name, CommandExecutor commandExecutor, ErrorActionDispatcher errorDispatcher) :
				Actor(std::move(name), std::move(commandExecutor), DEFAULT_HOOKS, std::make_unique<NoState>(),errorDispatcher) { }

Actor::Actor(std::string name, CommandExecutor commandExecutor, ActorHooks hooks, ErrorActionDispatcher errorDispatcher) :
		Actor(std::move(name), std::move(commandExecutor), hooks, std::make_unique<NoState>(),errorDispatcher) { }

Actor::Actor(std::string name, CommandExecutor commandExecutor, std::unique_ptr<State> state,
				ErrorActionDispatcher errorDispatcher) :
						Actor(std::move(name), std::move(commandExecutor), DEFAULT_HOOKS, std::move(state), errorDispatcher) { }

Actor::Actor(std::string name, CommandExecutor commandExecutor, ActorHooks hooks, std::unique_ptr<State> state,
				ErrorActionDispatcher errorDispatcher) :
					pImpl(new Actor::ActorImpl(name, std::move(commandExecutor), hooks, std::move(state), errorDispatcher))
										{ }

Actor::~Actor() { delete pImpl; }


void Actor::post(Command command, SenderLink sender) const {
	static const RawData EMPTY_DATA;
	pImpl->executorQueue->post(command, EMPTY_DATA, std::move(sender));
}

void Actor::post(Command command, const RawData &params, SenderLink sender) const {
	pImpl->executorQueue->post(command, params, std::move(sender));
}

SenderLink Actor::getActorLinkRef() const { return pImpl->executorQueue; }

void Actor::registerActor(Actor &monitored) {
	pImpl->context.getSupervisor().registerMonitored(monitored.pImpl->context.getSupervisor());
}

void Actor::unregisterActor(Actor &monitored) {
	pImpl->context.getSupervisor().unregisterMonitored(monitored.pImpl->context.getSupervisor());
}

void Actor::notifyError(int e) { throw ActorException(e, "error in actor"); }


