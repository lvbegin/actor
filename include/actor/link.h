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

#ifndef LINK_H__
#define LINK_H__

#include <private/sharedQueue.h>
#include <private/types.h>
#include <actor/senderApi.h>

class Link;
using SharedLink = std::shared_ptr<Link>;

class Link : public SenderApi {
public:
	struct Message {
			const MessageType type;
			const Command code;
			RawData params;
			SharedSenderLink sender;
			Message(MessageType type, int code, RawData params, SharedSenderLink sender);
			Message();
			~Message();
			Message(struct Message &&m);
			Message (const struct Message &m) = delete;
			struct Message &operator=(const struct Message &m) = delete;

			bool isValid();
		private:
			const bool valid;
	};
	virtual ~Link();

	void post(Command command, SharedSenderLink sender = SharedSenderLink());
	void post(Command command, const RawData &params, SharedSenderLink sender = SharedSenderLink());

	void post(MessageType type, Command command, RawData params = RawData());

	Message get(void);
	Message get(unsigned int timeout_in_ms);

	static SharedLink create(std::string name = std::string());
private:
	Link(std::string name = std::string());
	SharedQueue<Message> queue;

	void putMessage(MessageType type, Command command, RawData params, SharedSenderLink sender);
};

#endif
