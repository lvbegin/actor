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


Executor::message::message(int c, std::vector<unsigned char> params) : command(c), params(params) {}

Executor::message::~message() = default;

Executor::message::message(struct message &&m) : command(m.command), params(std::move(m.params)), promise(std::move(m.promise)) { }


Executor::Executor(std::function<returnCode(int, std::vector<unsigned char>)> body)  : body(body), thread([this]() { executorBody(this->body); }) { }

Executor::~Executor() {
	post(COMMAND_SHUTDOWN);
	if (thread.joinable())
		thread.join();
};

std::future<returnCode> Executor::putMessage(int i, std::vector<unsigned char> params) {
	std::unique_lock<std::mutex> l(mutexQueue);

	struct message  m(i, std::move(params));
	auto future = m.promise.get_future();
	q.push(std::move(m));
	condition.notify_one();
	return future;
}

returnCode Executor::postSync(int i, std::vector<unsigned char> params) { return putMessage(i, params).get(); }

void Executor::post(int i, std::vector<unsigned char> params) { putMessage(i, params); }


struct Executor::message Executor::getMessage(void) {
	std::unique_lock<std::mutex> l(mutexQueue);

	condition.wait(l, [this]() { return !q.empty();});
	auto message = std::move(q.front());
	q.pop();
	return message;
}

void Executor::executorBody(std::function<returnCode(int, std::vector<unsigned char>)> body) {
	while (true) {
		struct Executor::message message(getMessage());
		if (COMMAND_SHUTDOWN == message.command) {
			message.promise.set_value(returnCode::shutdown);
			return;
		}
		switch (body(message.command, std::move(message.params))) {
		case returnCode::ok:
			message.promise.set_value(returnCode::ok);
			break;
		case returnCode::shutdown:
			message.promise.set_value(returnCode::shutdown);
			return;
		default:
			message.promise.set_value(returnCode::error);
			//notify parent.
			return;
		}
	}
}
