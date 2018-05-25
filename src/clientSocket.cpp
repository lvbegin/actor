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

#include <private/clientSocket.h>
#include <private/exception.h>

#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

Connection ClientSocket::openHostConnection(const std::string &host, uint16_t port) {
	return openHostConnection(toNetAddr(host, port));
}

Connection ClientSocket::openHostConnection(const struct NetAddr &sin) {
	const auto fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == fd)
		THROW(std::runtime_error, "socket creation failed.");
	if (-1 == connect(fd, &sin.ai_addr, sin.ai_addrlen))
		THROW(std::runtime_error, "cannot connect.");
	return Connection(fd);
}

struct NetAddr ClientSocket::toNetAddr(const std::string &host, uint16_t port) {
	struct addrinfo *addr;
	if (0 > getaddrinfo(host.c_str(), std::to_string(port).c_str(), NULL, &addr))
		THROW(std::runtime_error, "cannot convert hostname.");
	const auto rc = NetAddr(*addr->ai_addr, addr->ai_addrlen);
	freeaddrinfo(addr);
	return rc;
}
