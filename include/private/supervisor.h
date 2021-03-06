/* Copyright 2017 Laurent Van Begin
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

#ifndef SUPERVISOR_H__
#define SUPERVISOR_H__

#include <actor/link.h>
#include <actor/errorActionDispatcher.h>
#include <private/actorController.h>

class Supervisor {
public:
	Supervisor(ErrorActionDispatcher strategy, SharedLink self);
	~Supervisor();

	bool notifySupervisor(Command command) const;
	bool sendErrorToSupervisor(Command command) const;
	void manageErrorFromSupervised(ErrorCode error, const RawData &params) const;

	void restartActor(std::string name) const;
	void stopActor(std::string name) const;
	void restartActors() const;
	void stopActors() const;

	void removeActor(const std::string &name);
	void registerMonitored(Supervisor &monitored);
	void unregisterMonitored(Supervisor &monitored);
private:
	mutable std::mutex monitorMutex;
	const ErrorActionDispatcher actionDispatcher;
	const SharedLink self;
	ActorController supervisedRefs;
	std::weak_ptr<Link> supervisorRef;

	bool postSupervisor(MessageType type, Command command, const RawData &data) const;
	bool sendToSupervisor(MessageType type, uint32_t code) const;
	void doOperation(std::function<void(void)> op) const;
	bool doOperationWithReturn(std::function<bool(void)> op) const;
	void doRegistrationOperation(Supervisor &monitored, std::function<void(void)> op) const;
};

#endif
