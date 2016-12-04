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

#include <AbstractActor.h>
#include <Controller.h>
#include <executor.h>

#include <functional>
#include <memory>
#include <mutex>

class Actor;
using ActorRef = std::shared_ptr<Actor>;

class Actor : public AbstractActor {
public:
	Actor(std::string name, ExecutorBody body);
	~Actor();

	Actor(const Actor &a) = delete;
	Actor &operator=(const Actor &a) = delete;
	returnCode postSync(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	void post(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	void restart(void);
	std::string getName() const;

	static ActorRef createActorRef(std::string name, ExecutorBody body);
	static void registerActor(ActorRef monitor, ActorRef monitored);
	static void unregisterActor(ActorRef monitor, ActorRef monitored);

private:
	const std::string name;
	Controller<ActorRef> monitored;
	std::weak_ptr<Actor> supervisor;
	ExecutorBody body;
	MessageQueue executorQueue;
	std::unique_ptr<Executor> executor;
	returnCode actorExecutor(ExecutorBody body, int command, const std::vector<unsigned char> &params);
};

#endif
