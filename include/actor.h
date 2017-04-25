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

#ifndef ACTOR_H__
#define ACTOR_H__

#include <actorController.h>
#include <executor.h>
#include <actorStateMachine.h>
#include <supervisor.h>
#include <supervisorStragegy.h>

#include <functional>
#include <memory>
#include <mutex>
#include <future>

using ActorBody = std::function<StatusCode(Command, const RawData &, const ActorLink &)>;

using AtStopHook = std::function<void(const ActorContext &)>;
using AtStartHook = std::function<StatusCode(const ActorContext &)>;
using AtRestartHook = std::function<StatusCode(const ActorContext &)>;


class ActorStartFailure : public std::runtime_error {
public:
	ActorStartFailure() : std::runtime_error("actor failed to start.") { }
	~ActorStartFailure() = default;
};

class Actor {
public:
	Actor(std::string name, ActorBody body, SupervisorStrategy restartStrategy = DEFAULT_SUPERVISOR_STRATEGY);
	Actor(std::string name, ActorBody body, AtStartHook atStart, AtStopHook atStop, AtRestartHook atRestart, SupervisorStrategy restartStrategy = DEFAULT_SUPERVISOR_STRATEGY);
	~Actor();

	Actor(const Actor &a) = delete;
	Actor &operator=(const Actor &a) = delete;

	void post(Command command, ActorLink sender = ActorLink()) const;
	void post(Command command, const RawData &params, ActorLink sender = ActorLink()) const;

	ActorLink getActorLinkRef() const;

	void registerActor(Actor &monitored);
	void unregisterActor(Actor &monitored);

	static void notifyError(int e);

private:
	static const int ACTOR_BODY_FAILED = 0x00;
	const LinkRef executorQueue;
	const AtStopHook atStop;
	const AtRestartHook atRestart;
	const ActorBody body;
	Supervisor supervisor;
	ActorStateMachine stateMachine;
	std::unique_ptr<Executor> executor;

	StatusCode actorExecutor(ActorBody body, MessageType type, Command command, const RawData &params, const ActorLink &sender);
	StatusCode executeActorBody(ActorBody body, Command command, const RawData &params, const ActorLink &sender);
	StatusCode executeActorManagement(Command command, const RawData &params);
	StatusCode doRestart(void);
	StatusCode restartSateMachine(void);
	void checkActorInitialization(void) const;
	StatusCode executorStartCb(AtStartHook atStart);
	void executorStopCb(void);
	StatusCode executorRestartCb(std::promise<StatusCode> &status, std::promise<std::unique_ptr<Executor> &> &e);

	class ActorException : public std::runtime_error {
	public:
		ActorException(ErrorCode error, const std::string& what_arg);
		~ActorException();

		int getErrorCode() const;
	private:
		const ErrorCode error;
	};

};

#endif
