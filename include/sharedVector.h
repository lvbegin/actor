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

#ifndef SHARED_VECTOR_H__
#define SHARED_VECTOR_H__

#include <vector>
#include <mutex>
#include <algorithm>

template <typename T>
using FindTest=std::function<bool(const T&)>;

template <typename T>
using VectorIterator= typename std::vector<T>::const_iterator;

template <typename T>
class SharedVector {
public:
	SharedVector() = default;
	~SharedVector() = default;

	SharedVector(const SharedVector &m) = delete;
	SharedVector &operator=(const SharedVector &m) = delete;
	SharedVector(SharedVector &&m) = delete;
	SharedVector &operator=(SharedVector &&m) = delete;

	void push_back(T&& value) {
		std::unique_lock<std::mutex> l(mutex);

		vector.push_back(value);
	}

	void erase(FindTest<T> findTest) {
		std::unique_lock<std::mutex> l(mutex);

		vector.erase(internalFind_if(findTest));
	}

	T find_if(FindTest<T> findTest) const {
		std::unique_lock<std::mutex> l(mutex);

		return *internalFind_if(findTest);
	}
private:
	mutable std::mutex mutex;
	std::vector<T> vector;

	VectorIterator<T> internalFind_if(FindTest<T> findTest) const {
		const auto it = std::find_if(vector.begin(), vector.end(), findTest);
		if (vector.end() == it)
			THROW(std::out_of_range, "Actor not found locally");
		return it;
	}
};

#endif
