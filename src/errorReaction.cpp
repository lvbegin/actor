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

#include <actor/errorReaction.h>

const RestartActor RestartActor::singletonElement;

RestartActor::RestartActor() = default;
RestartActor::~RestartActor() = default;
const ErrorReaction *RestartActor::create() { return &singletonElement; }
void RestartActor::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,			
		ErrorCode error, const RawData &params, const ActorLink &actor)  const {
	supervisedRefs.restartOne(params.toString());
}

const StopActor StopActor::singletonElement;

StopActor::StopActor() = default;
StopActor::~StopActor() = default;
const ErrorReaction *StopActor::create() { return &singletonElement; }
void StopActor::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const ActorLink &actor)  const {
	supervisedRefs.stopOne(params.toString());
}
	

const StopAllActor StopAllActor::singletonElement;

StopAllActor::StopAllActor() = default;
StopAllActor::~StopAllActor() = default;
const ErrorReaction *StopAllActor::create() { return &singletonElement; }
void StopAllActor::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const ActorLink &actor)  const {
	supervisedRefs.stopAll(); 
}

const RestartAllActor RestartAllActor::singletonElement;

RestartAllActor::RestartAllActor() = default;
RestartAllActor::~RestartAllActor() = default;
const ErrorReaction *RestartAllActor::create() { return &singletonElement; }
void RestartAllActor::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const ActorLink &actor)  const { 
	supervisedRefs.restartAll(); 
}

const EscalateError EscalateError::singletonElement;

EscalateError::EscalateError() = default;
EscalateError::~EscalateError() = default;
const ErrorReaction *EscalateError::create() { return &singletonElement; }
void EscalateError::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const ActorLink &actor)  const {
	notifySupervisor(actor->getName(), error);
}

const DoNothingError DoNothingError::singletonElement;

DoNothingError::DoNothingError() = default;
DoNothingError::~DoNothingError() = default;
const ErrorReaction *DoNothingError::create() { return &singletonElement; }
void DoNothingError::executeAction(const ControllerApi &supervisedRefs, 
		NotifySupervisor notifySupervisor,
		ErrorCode error, const RawData &params, const ActorLink &actor)  const { }

