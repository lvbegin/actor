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

#ifndef ERROR_STRATEGY_H__
#define ERROR_STRATEGY_H__

#include <actorController.h>
#include <messageQueue.h>
#include <types.h>

typedef uint32_t ErrorCode;

class ErrorStrategy {
public:
		virtual void executeAction(const ActorController &supervisedRefs, const std::weak_ptr<MessageQueue> &supervisorRef,
									ErrorCode error, const RawData &params, const LinkRef &actor) const = 0;
protected:
		ErrorStrategy() = default;
		~ErrorStrategy() = default;
};

class RestartActor : public ErrorStrategy {
public:
	static const ErrorStrategy *create() { return &singletonElement; }
	virtual ~RestartActor() = default;
	void executeAction(const ActorController &supervisedRefs, const std::weak_ptr<MessageQueue> &supervisorRef,
			ErrorCode error, const RawData &params, const LinkRef &actor)  const { supervisedRefs.restartOne(params.toString()); }
private:
	RestartActor() = default;
	static const RestartActor singletonElement;

};

class StopActor : public ErrorStrategy {
public:
	static const ErrorStrategy *create() { return &singletonElement; }
	virtual ~StopActor() = default;
	void executeAction(const ActorController &supervisedRefs, const std::weak_ptr<MessageQueue> &supervisorRef,
			ErrorCode error, const RawData &params, const LinkRef &actor)  const { supervisedRefs.stopOne(params.toString()); }
private:
	StopActor() = default;
	static const StopActor singletonElement;
};


class RestartAllActor : public ErrorStrategy {
public:
	static const ErrorStrategy *create() { return &singletonElement; }
	virtual ~RestartAllActor() = default;
	void executeAction(const ActorController &supervisedRefs, const std::weak_ptr<MessageQueue> &supervisorRef,
			ErrorCode error, const RawData &params, const LinkRef &actor)  const { supervisedRefs.restartAll(); }
private:
	RestartAllActor() = default;
	static const RestartAllActor singletonElement;

};

class EscalateError : public ErrorStrategy {
public:
	static const ErrorStrategy *create() { return &singletonElement; }
	virtual ~EscalateError() = default;
	void executeAction(const ActorController &supervisedRefs, const std::weak_ptr<MessageQueue> &supervisorRef,
			ErrorCode error, const RawData &params, const LinkRef &actor)  const {
		const auto ref = supervisorRef.lock();
		if (nullptr != ref.get())
			ref->post(MessageType::ERROR_MESSAGE, error, actor->getName());
	}
private:
	EscalateError() = default;
	static const EscalateError singletonElement;
};

#endif
