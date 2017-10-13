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

#ifndef COMMAND_EXECUTOR_H__
#define COMMAND_EXECUTOR_H__

#include <actor/context.h>
#include <actor/rawData.h>
#include <actor/linkApi.h>

#include <functional>

using PreCommandHook = std::function<StatusCode(Context &, Command, const RawData &, const ActorLink &)>;
using PostCommandHook = std::function<void(Context &, Command, const RawData &, const ActorLink &)>;

extern const PreCommandHook DEFAULT_PRECOMMAND_HOOK;
extern const PostCommandHook DEFAULT_POSTCOMMAND_HOOK;

struct commandMap;

class CommandExecutor {
	public:
	CommandExecutor(const commandMap map[] = nullptr);
	CommandExecutor(PreCommandHook preCommand, PostCommandHook postCommand, const commandMap map[] = nullptr);
	CommandExecutor(CommandExecutor &&c);
	CommandExecutor &operator=(CommandExecutor &&c);

	~CommandExecutor();

	StatusCode execute(Context &context, Command commandCode, const RawData &data,
						const ActorLink &actorLink) const;

	private:
	struct CommandExecutorImpl;
	CommandExecutorImpl *pImpl;
};

#endif
