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

#ifndef SHARED_MAP_H__
#define SHARED_MAP_H__

#include <exception.h>

#include <map>
#include <mutex>
#include <algorithm>

template<typename K, typename T>
class SharedMap {
public:
	SharedMap() = default;
	~SharedMap() = default;

	SharedMap(const SharedMap &m) = delete;
	SharedMap &operator=(const SharedMap &m) = delete;
	SharedMap(SharedMap &&m) = delete;
	SharedMap &operator=(SharedMap &&m) = delete;

	template <typename L, typename M>
	void insert(L&& key, M &&value) {
		std::unique_lock<std::mutex> l(mutex);

		map.insert(std::make_pair(std::forward<L>(key), std::forward<M>(value)));
	}
	template <typename... Args>
	void emplace(Args&&... args) {
		std::unique_lock<std::mutex> l(mutex);

		map.emplace(std::forward<Args>(args)...);
	}
	void erase(const K &key) {
		std::unique_lock<std::mutex> l(mutex);

		const auto it = map.find(key);
		if (map.end() == it)
			THROW(std::runtime_error, "element to erase does not exist. WTF");
		map.erase(it);
	}
	T find (K key) const {
		std::unique_lock<std::mutex> l(mutex);

		const auto it = map.find(key);
		if (map.end() == it)
			THROW(std::out_of_range, "element not found.");
		return it->second;
	}
	void for_each(std::function<void(const std::pair<const K, T> &)> f) const {
		std::unique_lock<std::mutex> l(mutex);

		std::for_each(map.begin(), map.end(), f);
	}
private:
	std::map<K, T> map;
	mutable std::mutex mutex;
};

#endif
