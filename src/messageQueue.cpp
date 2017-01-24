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

#include <messageQueue.h>

MessageQueue::message::message(MessageType type, int code, RawData params, ActorLink sender) :
				type(type), code(code), params(std::move(params)), sender(std::move(sender)) { }

MessageQueue::message::~message() = default;
MessageQueue::message::message(struct message &&m) = default;

MessageQueue::MessageQueue(std::string name) : name(std::move(name)) { }
MessageQueue::~MessageQueue() = default;

void MessageQueue::post(int code, ActorLink sender) {
	static const RawData emptyData;
	post(code, emptyData, std::move(sender));
}

void MessageQueue::post(int code, const RawData &params, ActorLink sender) {
	putMessage(MessageType::COMMAND_MESSAGE, code, params, std::move(sender));
}

const std::string &MessageQueue::getName(void) const { return name; }

void MessageQueue::post(MessageType type, int code, RawData params) {
	putMessage(type, code, std::move(params), ActorLink());
}

struct MessageQueue::message MessageQueue::get(void) { return queue.get(); }

void MessageQueue::putMessage(MessageType type, int code, RawData params, ActorLink sender) {
	queue.post(message(type, code, std::move(params), std::move(sender)));
}
