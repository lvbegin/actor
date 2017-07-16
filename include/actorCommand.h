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

#ifndef ACTOR_COMMAND_H__
#define ACTOR_COMMAND_H__

#include <map>
#include <functional>

#include <rawData.h>
#include <linkApi.h>
#include <commandValue.h>

using CommandFunction = std::function<StatusCode(const RawData &, const ActorLink &)>;

typedef struct {
	Command commandCode;
	CommandFunction command;
} commandMap;

class ActorCommand {
public:
	ActorCommand() : commands(ActorCommand::buildMap(nullptr, 0)) { }
	ActorCommand(const commandMap map[], size_t size) : commands(ActorCommand::buildMap(map, size)) { }
	~ActorCommand() = default;

	StatusCode execute(Command commandCode, const RawData &data, const ActorLink &actorLink) const {
		CommandFunction f;
		try {
			f = commands.at(commandCode);
		} catch (std::out_of_range &e) {
			if (nullptr != actorLink)
				actorLink->post(CommandValue::UNKNOWN_COMMAND);
			return StatusCode::OK;
		}
		return f(data, actorLink);
	}
private:
	const std::map<Command, CommandFunction> commands;

	static std::map<Command, CommandFunction> buildMap(const commandMap array[], size_t size) {
		std::map<Command, CommandFunction> map;
		for (size_t i = 0; i < size; i ++) {
			if (CommandValue::isInternalCommand(array[i].commandCode))
				THROW(std::runtime_error, "command reserved for internal use.");
			map[array[i].commandCode] = array[i].command;
		}
		map[static_cast<Command>(CommandValue::SHUTDOWN)] = shutdownCase;
		return map;
	}
	static StatusCode shutdownCase(const RawData &, const ActorLink &) { return StatusCode::SHUTDOWN; };
};

#endif
