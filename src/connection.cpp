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

#include <connection.h>
#include <descriptorWait.h>
#include <exception.h>

#include <stdexcept>

Connection::Connection() : fd(-1) {}

Connection::Connection(int fd) : fd(fd) {
	FD_ZERO(&set);
	FD_SET(fd, &set);
}

Connection::~Connection() {
	if (-1 != fd)
		close(fd);
}

Connection::Connection(Connection &&connection) : fd(-1) { *this = std::move(connection); }

Connection &Connection::operator=(Connection &&connection) {
	std::swap(fd, connection.fd);
	std::swap(set, connection.set);
	return *this;
}

Connection &Connection::writeRawData(std::vector<unsigned char> data) { return writeInt(data.size()).writeBytes(data.data(), data.size()); }

std::vector<unsigned char> Connection::readRawData(void) {
	std::vector<unsigned char> data(readInt<size_t>());
	if (0 < data.capacity())
		readBytes(data.data(), data.capacity());
	return data;
}

Connection &Connection::writeString(std::string hostValue) {
	const size_t nbBytesToWrite = hostValue.size() + 1; //check max size
	return writeInt(nbBytesToWrite).writeBytes(hostValue.c_str(), nbBytesToWrite);
}

std::string Connection::readString(void) {
	const size_t size = readInt<size_t>();
	char string[size]; //check max size
	readBytesNonBlocking(string, size);
	if (0 != string[size-1])
		THROW(std::runtime_error, "non NULL-terminated string value");
	return std::string(string);
}

Connection &Connection::writeBytes(const void *buffer, size_t count) {
	if (-1 == fd)
		THROW(std::runtime_error, "invalid writeByte");
	const char *ptr = static_cast<const char *>(buffer);
	for (size_t nbTotalWritten = 0; nbTotalWritten < count; ) {
		const ssize_t nbWritten = write(fd, ptr + nbTotalWritten, count - nbTotalWritten);
		if (-1 == nbWritten && !(EINTR == errno))
			THROW(std::runtime_error, "send bytes failed");
		nbTotalWritten += nbWritten;
	}
	return *this;
}

void Connection::readBytes(void *buffer, size_t count, int timeoutInSeconds) {
	waitForRead<std::runtime_error, std::runtime_error>(fd, &set, timeoutInSeconds);
	readBytesNonBlocking(buffer, count);
}

void Connection::readBytesNonBlocking(void *buffer, size_t count) {
	static const int timeoutOnRead = 60;
	if (-1 == fd)
		THROW(std::runtime_error, "invalid fd in readBytesNonBlocking");
	char *ptr = static_cast<char *>(buffer);
	struct timeval timeout { timeoutOnRead, 0 };
	for (size_t nbTotalRead = 0; nbTotalRead < count; ) {
		waitForRead<std::runtime_error, std::runtime_error>(fd, &set, &timeout);
		const ssize_t nbRead = read(fd, ptr + nbTotalRead, count - nbTotalRead);
		if (0 >= nbRead)
			THROW(std::runtime_error, "read bytes failed");
		nbTotalRead += nbRead;
	}
}
