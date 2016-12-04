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

Actor::Actor(std::string name, ExecutorBody body)  : name(std::move(name)), body(body),
executor(new Executor([this, body](int command, const std::vector<unsigned char> &params)
		{ return this->actorExecutor(body, command, params); }, &executorQueue)) { }

Actor::~Actor() = default;

returnCode Actor::postSync(int i, std::vector<unsigned char> params) { return executorQueue.putMessage(i, params).get(); }

void Actor::post(int i, std::vector<unsigned char> params) { executorQueue.putMessage(i, params); }

void Actor::restart(void) {
	executor.reset(); //stop the current thread. Ensure that the new thread does not receive the shutdown
	executor.reset(new Executor([this](int command, const std::vector<unsigned char> &params)
			{ return this->actorExecutor(this->body, command, params); }, &executorQueue));
}

std::string Actor::getName() const { return name; }

ActorRef Actor::createActorRef(std::string name, ExecutorBody body) { return std::make_shared<Actor>(name, body); }

void Actor::registerActor(ActorRef monitor, ActorRef monitored) {
	monitor->monitored.addActor(monitored);
	monitored->supervisor = std::weak_ptr<Actor>(monitor);
}

void Actor::unregisterActor(ActorRef monitor, ActorRef monitored) {
	monitor->monitored.removeActor(monitored->getName());
	monitored->supervisor.reset();
}

returnCode Actor::actorExecutor(ExecutorBody body, int command, const std::vector<unsigned char> &params) {
	if (command == 0x69) {
		monitored.restartOne(std::string(params.begin(), params.end()));
		return returnCode::ok;
	}
	try {
		return body(command, params);
	} catch (std::exception e) {
		ActorRef supervisorRef(supervisor);
		supervisorRef->post(0x69, std::vector<unsigned char>(name.begin(), name.end()));
		return returnCode::error;
	}
}
