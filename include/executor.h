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

#ifndef EXECUTOR_H__
#define EXECUTOR_H__

#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <queue>

#include <rc.h>

class Executor {
public:
	Executor(std::function<returnCode(int, std::vector<unsigned char>)> body);
	~Executor();

	Executor(const Executor &a) = delete;
	Executor &operator=(const Executor &a) = delete;
	returnCode postSync(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	void post(int i, std::vector<unsigned char> params = std::vector<unsigned char>());
	static const uint32_t COMMAND_SHUTDOWN = 0;
private:
	struct message {
		int command;
		std::vector<unsigned char> params;
		std::promise<returnCode> promise;
		message(int c, std::vector<unsigned char> params);
		~message();
		message(struct message &&m);
		message (const struct message &m) = delete;
		struct message &operator=(const struct message &m) = delete;
	};

	std::future<returnCode> putMessage(int i, std::vector<unsigned char> params);
	void executorBody(std::function<returnCode(int, std::vector<unsigned char>)> body);
	message getMessage(void);
	std::mutex mutexQueue;
	std::condition_variable condition;
	std::queue<message> q;
	std::thread thread;

};


#endif
