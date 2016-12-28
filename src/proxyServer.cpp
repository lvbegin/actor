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

#include <proxyServer.h>
#include <serverSocket.h>
#include <proxyContainer.h>
#include <command.h>

#include <arpa/inet.h>

proxyServer::proxyServer(GenericActorPtr actor, Connection connection, std::function<void(void)> notifyTerminate) :
	t([actor, connection {std::move(connection)}, notifyTerminate]() mutable
						{ startThread(std::move(actor), std::move(connection), notifyTerminate); }) { }

proxyServer::~proxyServer() {
	if (t.joinable())
		t.join();
};


void proxyServer::startThread(GenericActorPtr actor, Connection connection, std::function<void(void)> notifyTerminate) {
	while (true) {
		uint32_t command;
		postType type;
		try {
			type = connection.readInt<postType>();
		} catch (ConnectionTimeout &e) { continue ; }
		  catch (std::runtime_error &e) {  return; }
		switch (type) {
			case postType::Async:
				command = connection.readInt<uint32_t>();
				actor->post(command, connection.readRawData());
				break;
			case postType::Sync:
				command = connection.readInt<uint32_t>();
				connection.writeInt(actor->postSync(command, connection.readRawData()));
				break;
			case postType::Restart:
				actor->post(Command::COMMAND_RESTART);
				continue;
			default:
				continue;
		}
		if (AbstractActor::COMMAND_SHUTDOWN == command) {
			notifyTerminate();
			return;
		}
	}
}
