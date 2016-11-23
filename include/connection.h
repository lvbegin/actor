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

#ifndef CONNECTION_H__
#define CONNECTION_H__

#include <cstdint>
#include <unistd.h>
#include <string>
#include <vector>

#include <arpa/inet.h>

class Connection {
public:
	Connection();
	Connection(int fd);
	~Connection();
	Connection(const Connection &connection) = delete;
	Connection &operator=(const Connection &connection) = delete;
	Connection(Connection &&connection);
	Connection &operator=(Connection &&connection);
	template<typename T>
	Connection &writeInt(T hostValue) {
		const uint32_t sentValue = htonl(static_cast<uint32_t>(hostValue));
		return writeBytes(&sentValue, sizeof(sentValue));
	}

	template<typename T>
	T readInt(void) {
		uint32_t value;
		readBytes(&value, sizeof(value));
		return static_cast<T>(ntohl(value));
	}
	Connection &writeString(const std::string &hostValue);
	std::string readString(void);
	Connection &writeRawData(const std::vector<unsigned char> &data);
	std::vector<unsigned char> readRawData(void);
	Connection &writeBytes(const void *buffer, size_t count);
	void readBytes(void *buffer, size_t count, int timeout = 5);
private:
	void readBytesNonBlocking(void *buffer, size_t count);

	int fd;
	fd_set set;
};

#endif
