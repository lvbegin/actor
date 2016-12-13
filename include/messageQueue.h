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

#ifndef MESSAGE_QUEUE_H__
#define MESSAGE_QUEUE_H__

#include <sharedQueue.h>
#include <rc.h>

#include <future>


class MessageQueue {
public:
	enum class type:uint32_t { COMMAND_MESSAGE, ERROR_MESSAGE, RESTART_MESSAGE, };
	struct message {
		MessageQueue::type type;
		int code;
		std::vector<unsigned char> params;
		std::promise<returnCode> promise;
		message(MessageQueue::type type, int c, std::vector<unsigned char> params);
		~message();
		message(struct message &&m);
		message (const struct message &m) = delete;
		struct message &operator=(const struct message &m) = delete;
	};
	MessageQueue();
	~MessageQueue();
	std::future<returnCode> putMessage(MessageQueue::type type, int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	message getMessage(void);
private:
	SharedQueue<message> queue;
};

#endif
