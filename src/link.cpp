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

#include <actor/link.h>

Link::Message::Message(MessageType type, int code, RawData params, SenderLink sender) :
				type(type), code(code), params(std::move(params)), sender(std::move(sender)), valid(true) { }
Link::Message::Message() : type(MessageType::COMMAND_MESSAGE), code(0), valid(false) { }

Link::Message::~Message() = default;
Link::Message::Message(struct Message &&m) = default;

bool Link::Message::isValid() { return valid; }

Link::Link(std::string name) : SenderApi(std::move(name)) { }
Link::~Link() = default;


void Link::post(Command command, SenderLink sender) {
	static const RawData EMPTY_DATA;
	post(command, EMPTY_DATA, std::move(sender));
}

void Link::post(Command command, const RawData &params, SenderLink sender) {
	putMessage(MessageType::COMMAND_MESSAGE, command, params, std::move(sender));
}

void Link::post(MessageType type, Command command, RawData params) {
	static const SenderLink NO_LINK;
	putMessage(type, command, std::move(params), NO_LINK);
}

struct Link::Message Link::get(void) { return queue.get(); }

Link::Message Link::get(unsigned int timeout_in_ms) { return queue.get(timeout_in_ms); }

void Link::putMessage(MessageType type, Command command, RawData params, SenderLink sender) {
	queue.post(Message(type, command, std::move(params), std::move(sender)));
}
