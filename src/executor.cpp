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

#include <executor.h>
#include <exception.h>
#include <command.h>

Executor::Executor(ExecutorBody body, MessageQueue &queue, std::function<void(void)> atStart) : messageQueue(queue),
					thread([this, body, atStart]() { atStart(); executeBody(body); }) { }

Executor::~Executor() {
	thread.join();
};

void Executor::executeBody(ExecutorBody body) {

	while (true) {
		struct MessageQueue::message message(messageQueue.get());
		const StatusCode status = (MessageType::COMMAND_MESSAGE == message.type && Command::COMMAND_SHUTDOWN == message.code) ?
				StatusCode::shutdown : body(message.type, message.code, message.params, message.sender);

		switch (status) {
			case StatusCode::ok:
				message.promise.set_value(StatusCode::ok);
				break;
			case StatusCode::shutdown:
				message.promise.set_value(StatusCode::ok);
				return;
			case StatusCode::error:
				message.promise.set_value(StatusCode::error);
				break;
			default:
				THROW(std::runtime_error, "Unknown status code.");
		}
	}
}
