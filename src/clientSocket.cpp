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

#include <clientSocket.h>
#include <exception.h>

#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <memory.h>

Connection ClientSocket::openHostConnection(std::string host, uint16_t port) {
	sockaddr_in sin = toSockAddr(host, port);
	return openHostConnection(sin);
}

Connection ClientSocket::openHostConnection(const struct sockaddr_in &sin) {
	const int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == fd)
		THROW(std::runtime_error, "socket creation failed.");
	if (-1 == connect(fd, reinterpret_cast<const sockaddr*>(&sin), sizeof(struct sockaddr_in)))
		THROW(std::runtime_error, "cannot connect.");
	return Connection(fd);
}

struct sockaddr_in ClientSocket::toSockAddr(std::string host, uint16_t port) {
	sockaddr_in sin;
	hostent *he = gethostbyname(host.c_str()); //not thread safe ...
	if (nullptr == he)
		THROW(std::runtime_error, "cannot convert hostname.");
	memcpy(&sin.sin_addr.s_addr, he->h_addr, he->h_length);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	return sin;
}
