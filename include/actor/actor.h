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

#include <actor/errorActionDispatcher.h>
#include <actor/context.h>
#include <actor/commandExecutor.h>

#include <functional>

using AtStopHook = std::function<void(const Context &)>;
using AtStartHook = std::function<StatusCode(const Context &)>;
using AtRestartHook = std::function<StatusCode(const Context &)>;

extern const AtStartHook DEFAULT_START_HOOK;
extern const AtStopHook DEFAULT_STOP_HOOK;
extern const AtRestartHook DEFAULT_RESTART_HOOK;
extern const ErrorActionDispatcher DEFAULT_ERROR_DISPATCHER;


class ActorStartFailure : public std::runtime_error {
public:
	ActorStartFailure() : std::runtime_error("actor failed to start.") { }
	~ActorStartFailure() = default;
};

struct ActorHooks {
	AtStartHook atStart;
	AtStopHook atStop;
	AtRestartHook atRestart;
	ActorHooks(AtStartHook atStart, AtStopHook atStop, AtRestartHook atRestart) :
									atStart(atStart), atStop(atStop), atRestart(atRestart) { }
};

class Executor;

class Actor {
public:
	Actor(std::string name, CommandExecutor commandExecutor, ErrorActionDispatcher errorDispatcher = DEFAULT_ERROR_DISPATCHER);
	Actor(std::string name, CommandExecutor commandExecutor, ActorHooks hooks,
			ErrorActionDispatcher errorDispatcher = DEFAULT_ERROR_DISPATCHER);

	Actor(std::string name, CommandExecutor commandExecutor, std::unique_ptr<State> state,
			ErrorActionDispatcher errorDispatcher = DEFAULT_ERROR_DISPATCHER);
	Actor(std::string name, CommandExecutor commandExecutor, ActorHooks hooks, std::unique_ptr<State> state,
			ErrorActionDispatcher errorDispatcher = DEFAULT_ERROR_DISPATCHER);

	~Actor();

	Actor(const Actor &a) = delete;
	Actor &operator=(const Actor &a) = delete;

	void post(Command command, SharedSenderLink sender = SharedSenderLink()) const;
	void post(Command command, const RawData &params, SharedSenderLink sender = SharedSenderLink()) const;

	SharedSenderLink getActorLinkRef() const;

	void registerActor(Actor &monitored);
	void unregisterActor(Actor &monitored);

	static void notifyError(int e);

private:
	class ActorImpl;
	ActorImpl *pImpl;
	static const int ACTOR_BODY_FAILED = 0x00;

};

#endif
