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

#ifndef SHARED_QUEUE_H__
#define SHARED_QUEUE_H__

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <chrono>

template<typename T>
class SharedQueue {
public:
	SharedQueue(std::function<T(void)> invalidValue = [](){ return T(); }) : invalidValue(invalidValue) { }
	~SharedQueue() = default;

	void post (T &&v) {
		std::unique_lock<std::mutex> l(mutexQueue);

		q.push(std::forward<T>(v));
		condition.notify_one();
	}
	T get(void) {
		return get([this](std::unique_lock<std::mutex> &l) { 
			condition.wait(l, [this]() { return !q.empty();}); 
			return true;
		});
	}

	T get (unsigned int timeout_in_ms) {
		std::chrono::milliseconds timeout(timeout_in_ms);
		return get([this, &timeout](std::unique_lock<std::mutex> &l) { 
			return condition.wait_for(l, timeout, [this]() { return !q.empty();}); 
		});
	}
private:
	using WaitMessage = std::function<bool(std::unique_lock<std::mutex> &)> ;

	T get(WaitMessage &&waitMessage) {
		std::unique_lock<std::mutex> l(mutexQueue);

		if (!waitMessage(l)) 
			return invalidValue();	
		auto message = std::move(q.front());
		q.pop();
		return message;
		
	}
	std::mutex mutexQueue;
	std::condition_variable condition;
	std::queue<T> q;
	std::function<T(void)> invalidValue;
};

#endif
