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

#include <serverSocket.h>
#include <exception.h>
#include <descriptorWait.h>

#include <stdexcept>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <unistd.h>

ServerSocket::ServerSocket(uint16_t port) : acceptFd(listenOnSocket(port)) {
	FD_ZERO(&set);
	FD_SET(acceptFd, &set);
}

ServerSocket::ServerSocket(ServerSocket&& s) { *this = std::move(s); }

ServerSocket &ServerSocket::operator=(ServerSocket&& s) {
	closeSocket();
	std::swap(this->acceptFd,s.acceptFd);
	std::swap(this->set,s.set);
	return *this;
}

ServerSocket::~ServerSocket() { closeSocket(); }

void ServerSocket::closeSocket(void) {
	if (-1 == acceptFd)
		return ;
	close(acceptFd);
	acceptFd = -1;
}

Connection ServerSocket::getConnection(int port) { return ServerSocket(port).acceptOneConnection(); }

Connection ServerSocket::acceptOneConnection(int timeout, struct NetAddr *client_addr) const {
	struct NetAddr client_addr_struct { };
	auto NetAddr_ptr = (nullptr == client_addr) ? &client_addr_struct : client_addr;
	socklen_t length = sizeof(NetAddr_ptr->ai_addr);
	waitForRead<ConnectionTimeout, std::runtime_error>(acceptFd, set, timeout);
	const auto newsockfd = accept(acceptFd, &NetAddr_ptr->ai_addr, &length);
	if (-1 == newsockfd)
		THROW(std::runtime_error, "accept failed.");
	NetAddr_ptr->ai_addrlen = length;
	return Connection(newsockfd);
}

int ServerSocket::listenOnSocket(uint16_t port) {
	const auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
		THROW(std::runtime_error, "cannot create socket.");
	static const int ENABLE = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ENABLE, sizeof(ENABLE)) < 0)
		THROW(std::runtime_error, "setsockopt(SO_REUSEADDR) failed.");
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, reinterpret_cast<struct sockaddr *> (&serv_addr), sizeof(serv_addr)) < 0)
		THROW(std::runtime_error, "cannot bind socket.");
	if (-1 == listen(sockfd,5))
		THROW(std::runtime_error, "cannot listen.");
	return sockfd;
}
