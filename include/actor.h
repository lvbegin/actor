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
#include <restartStragegy.h>
#include <actorStateMachine.h>
#include <supervisor.h>

#include <functional>
#include <memory>
#include <mutex>

using ActorBody = std::function<StatusCode(int, const RawData &)>;

class Actor {
public:
	Actor(ActorBody body, RestartStrategy restartStrategy = defaultRestartStrategy);
	Actor(ActorBody body, std::function<void(void)> atRestart, RestartStrategy restartStrategy = defaultRestartStrategy);
	~Actor();

	Actor(const Actor &a) = delete;
	Actor &operator=(const Actor &a) = delete;

	StatusCode postSync(int i, RawData params = RawData()) const;
	void post(int i, RawData params = RawData()) const;
	std::shared_ptr<LinkApi> getActorLinkRef() const;

	void registerActor(Actor &monitored);
	void unregisterActor(Actor &monitored);

	static void notifyError(int e);
private:
	static const int EXCEPTION_THROWN_ERROR = 0x00;
	static std::function<void(void)> doNothing;

	std::shared_ptr<MessageQueue> executorQueue;
	Supervisor supervisor;
	const std::function<void(void)> atRestart;
	const ActorBody body;
	ActorStateMachine stateMachine;
	std::unique_ptr<Executor> executor;

	StatusCode actorExecutor(ActorBody body, MessageType type, int code, const RawData &params);
	StatusCode executeActorBody(ActorBody body, int code, const RawData &params);
	StatusCode doRestart(void);
	StatusCode restartSateMachine(void);
};

#endif
