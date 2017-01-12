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

#ifndef UNIQUE_ID_H__
#define UNIQUE_ID_H__

#include <cstdint>
#include <vector>
#include <algorithm>
#include <exception.h>

typedef uint32_t Id;

template <typename N>
class UniqueId {
public:
	static Id newId(void) { return id++; }

	static std::vector<uint8_t> serialize(Id value) {
		void *ptr = &value;
		return std::vector<uint8_t>(static_cast<uint8_t *>(ptr), static_cast<uint8_t *>(ptr) + 4);
	}

	static Id unserialize(const std::vector<uint8_t> &value) {
		if (4 != value.size())
			THROW(std::runtime_error, "serialized unteger does not have correct size.");
		uint32_t rc;
		void *ptr = &rc;
		const uint8_t *first = static_cast<const uint8_t *>(ptr);
		std::for_each(static_cast<uint8_t *>(ptr), static_cast<uint8_t *>(ptr) + 4, [first, &value](uint8_t &p) { p = value[&p - first]; });
		return rc;
	}
private:
	static std::atomic<uint32_t> id;
};

template <typename N>
std::atomic<uint32_t> UniqueId<N>::id { 0 };


#endif

