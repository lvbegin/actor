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

#include <actor/controllerApi.h>
#include <actor/senderApi.h>
#include <actor/types.h>


using NotifySupervisor = std::function<void(const std::string &actorName, ErrorCode error)>; 

class ErrorReaction {
public:
		virtual void executeAction(const ControllerApi &supervisedRefs,
				NotifySupervisor notifySupervisor, ErrorCode error, const RawData &params,
				const SenderLink &actor) const = 0;
protected:
	ErrorReaction() = default;
	virtual ~ErrorReaction() = default;
};

class RestartActor : public ErrorReaction {
public:
	static const ErrorReaction *create();
	virtual ~RestartActor();
	void executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,			
		ErrorCode error, const RawData &params, const SenderLink &actor)  const override;
private:
	RestartActor();
	static const RestartActor singletonElement;

};

class StopActor : public ErrorReaction {
public:
	static const ErrorReaction *create();
	virtual ~StopActor();
	void executeAction(const ControllerApi &supervisedRefs, 
			NotifySupervisor notifySupervisor,
			ErrorCode error, const RawData &params, const SenderLink &actor)  const override;
private:
	StopActor();
	static const StopActor singletonElement;
};

class StopAllActor : public ErrorReaction {
public:
	static const ErrorReaction *create();
	virtual ~StopAllActor();
	void executeAction(const ControllerApi &supervisedRefs, 
			NotifySupervisor notifySupervisor,
			ErrorCode error, const RawData &params, const SenderLink &actor)  const override;
private:
	StopAllActor();
	static const StopAllActor singletonElement;
};

class RestartAllActor : public ErrorReaction {
public:
	static const ErrorReaction *create();
	virtual ~RestartAllActor();
	void executeAction(const ControllerApi &supervisedRefs, 
			NotifySupervisor notifySupervisor,
			ErrorCode error, const RawData &params, const SenderLink &actor)  const override;
private:
	RestartAllActor();
	static const RestartAllActor singletonElement;
};

class EscalateError : public ErrorReaction {
public:
	static const ErrorReaction *create();
	virtual~EscalateError();
	void executeAction(const ControllerApi &supervisedRefs, 
			NotifySupervisor notifySupervisor,
			ErrorCode error, const RawData &params, const SenderLink &actor)  const override;
private:
	EscalateError();
	static const EscalateError singletonElement;
};

class DoNothingError : public ErrorReaction {
public:
	static const ErrorReaction *create();
	virtual~DoNothingError();
	void executeAction(const ControllerApi &supervisedRefs, 
			NotifySupervisor notifySupervisor,
			ErrorCode error, const RawData &params, const SenderLink &actor)  const override;
private:
	DoNothingError();
	static const DoNothingError singletonElement;
};

#endif
