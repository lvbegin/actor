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

#include <actor/commandExecutor.h>

#include <private/actorCommand.h>

struct 	CommandExecutor::CommandExecutorImpl {
	PreCommandHook preCommand;
	PostCommandHook postCommand;
	ActorCommand actorCommand;
	CommandExecutorImpl(PreCommandHook preCommand, PostCommandHook postCommand, ActorCommand actorCommand) :
		preCommand(preCommand), postCommand(postCommand), actorCommand(actorCommand) { }
	~CommandExecutorImpl() = default;
};


const PreCommandHook DEFAULT_PRECOMMAND_HOOK = [](Context &, Command, const RawData &, const ActorLink &) {
	return StatusCode::OK;
};

const PostCommandHook DEFAULT_POSTCOMMAND_HOOK = [](Context &, Command, const RawData &, const ActorLink &) { };

CommandExecutor::CommandExecutor(const commandMap map[]) :
	CommandExecutor(DEFAULT_PRECOMMAND_HOOK, DEFAULT_POSTCOMMAND_HOOK, map) { }

CommandExecutor::CommandExecutor(PreCommandHook preCommand, PostCommandHook postCommand, const commandMap map[]) :
		pImpl(new CommandExecutorImpl(preCommand, postCommand, ActorCommand(map))) { }

CommandExecutor::CommandExecutor(CommandExecutor &&c) : pImpl(nullptr)
{
	this->pImpl = c.pImpl;
	c.pImpl = nullptr;
}
CommandExecutor &CommandExecutor::operator=(CommandExecutor &&c)
{
	std::swap(this->pImpl, c.pImpl);
	return *this;
}


CommandExecutor::~CommandExecutor() {
	delete pImpl;
}

StatusCode CommandExecutor::execute(Context &context, Command commandCode, const RawData &data,
										const ActorLink &actorLink) const {
	auto rc = pImpl->preCommand(context, commandCode, data, actorLink);
	if (StatusCode::OK != rc)
		return rc;
	try {
		rc  = pImpl->actorCommand.execute(context, commandCode, data, actorLink);
	} catch (std::exception &e) {
		pImpl->postCommand(context, commandCode, data, actorLink);
		throw ;
	}

	return (pImpl->postCommand(context, commandCode, data, actorLink), rc);
}
