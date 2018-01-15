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

#include <private/errorReaction.h>

RestartActor::RestartActor() = default;
RestartActor::~RestartActor() = default;
void RestartActor::executeAction(const ControllerApi &supervisedRefs, 
	NotifySupervisor notifySupervisor,			
	ErrorCode error, const RawData &params, const SharedSenderLink &actor)  const {
    supervisedRefs.restartOne(params.toString());
}
const ErrorReaction *RestartActor::create() { return &singletonElement; }

const RestartActor RestartActor::singletonElement;

StopActor::StopActor() = default;
StopActor::~StopActor() = default;
void StopActor::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const SharedSenderLink &actor)  const {
    supervisedRefs.stopOne(params.toString());
}
const ErrorReaction *StopActor::create() { return &singletonElement; }

const StopActor StopActor::singletonElement;

StopAllActor::StopAllActor() = default;
StopAllActor::~StopAllActor() = default;
void StopAllActor::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const SharedSenderLink &actor)  const {
    supervisedRefs.stopAll(); 
}
const ErrorReaction *StopAllActor::create() { return &singletonElement; }

const StopAllActor StopAllActor::singletonElement;

RestartAllActor::RestartAllActor() = default;
RestartAllActor::~RestartAllActor() = default;
void RestartAllActor::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const SharedSenderLink &actor)  const { 
    supervisedRefs.restartAll(); 
}
const ErrorReaction *RestartAllActor::create() { return &singletonElement; }

const RestartAllActor RestartAllActor::singletonElement;

EscalateError::EscalateError() = default;
EscalateError::~EscalateError() = default;
void EscalateError::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const SharedSenderLink &actor)  const {
    notifySupervisor(actor->getName(), error);
}
const ErrorReaction *EscalateError::create() { return &singletonElement; }

const EscalateError EscalateError::singletonElement;


DoNothingError::DoNothingError() = default;
DoNothingError::~DoNothingError() = default;
void DoNothingError::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const SharedSenderLink &actor)  const { }
const ErrorReaction *DoNothingError::create() { return &singletonElement; }

const DoNothingError DoNothingError::singletonElement;

