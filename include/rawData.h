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

#ifndef RAW_DATA_H__
#define RAW_DATA_H__

#include <types.h>
#include <exception.h>

#include <algorithm>
#include <vector>

class RawData : public std::vector<uint8_t> {
public:
	using v = std::vector<uint8_t>;
	using v::v;
	RawData() = default;
	RawData(Id value) :  v(reinterpret_cast<uint8_t *>(&value), reinterpret_cast<uint8_t *>((&value) + 1)) { }
	RawData(const std::string &s) : v(s.begin(), s.end()) { }
	~RawData() = default;

	std::string toString() const { return std::string(begin(), end()); }
	Id toId() const {
		Id rc;

		if (sizeof(rc) != size())
			THROW(std::runtime_error, "serialized integer does not have correct size.");
		void * const ptr = &rc;
		const auto first = static_cast<uint8_t *>(ptr);
		std::for_each(first, first + sizeof(rc), [first, this](uint8_t &p) { p = (*this)[&p - first]; });
		return rc;
	}
};

#endif
