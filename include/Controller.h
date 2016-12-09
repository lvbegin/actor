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

#ifndef ACTOR_CONTROLLER_H__
#define ACTOR_CONTROLLER_H__

#include <exception.h>

#include <algorithm>
#include <mutex>
#include <functional>

template <typename T>
class Controller {
public:
	Controller() = default;
	~Controller() = default;

	void addActor(T actor) { actors.push_back(actor); }
	void removeActor(const std::string &name) { applyOn(name, [this](const typename std::vector<T>::const_iterator &it) { this->actors.erase(it); }); }
	void restartOne(const std::string &name) { //do something to constify
		applyOn(name, [](const typename std::vector<T>::const_iterator &it) { (*it)->restart(); });
	}
	void restartAll() const { std::for_each(actors.begin(), actors.end(), [](const T &actor){ actor->restart();} ); }
private:
	void applyOn(const std::string & name, std::function<void(const typename std::vector<T>::const_iterator &)> op) {
		typename std::vector<T>::const_iterator it = std::find_if(actors.begin(), actors.end(), [&name](const T &actors) { return (0 == name.compare(actors->getName())); });
		if (actors.end() != it)
			op(it);
	}

	std::vector<T> actors;
};

#endif
