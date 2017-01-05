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

#include <proxyContainer.h>
#include <command.h>
#include <uniqueId.h>

#include <tuple>

ProxyContainer::ProxyContainer() : executor([this](MessageType, int code, const std::vector<unsigned char> &not_used) { return (this->deleteProxy(code), StatusCode::ok);}, &executorQueue) { }

ProxyContainer::~ProxyContainer() {
	executorQueue.post(MessageType::COMMAND_MESSAGE, Command::COMMAND_SHUTDOWN);
}

void ProxyContainer::createNewProxy(ActorLink actor, Connection connection) {
	const auto id = UniqueId<ProxyContainer>::newId();
	const auto terminate = [this, id]() { this->executorQueue.post(MessageType::RESTART_MESSAGE, id); };
	proxies.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(actor, std::move(connection), terminate));
}

void ProxyContainer::deleteProxy(uint32_t i) { proxies.erase(i); }
