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

#ifndef DESCRIPTOR_WAIT_H__
#define DESCRIPTOR_WAIT_H__

#include <exception.h>

#include <sys/select.h>
#include <stdexcept>

template <typename E1, typename E2>
void waitForRead(int fd, fd_set set, struct timeval *timeout) {
	if (-1 == fd)
		THROW(std::runtime_error, "invalid fd.");
	switch(select(fd + 1, &set, NULL, NULL, timeout)) {
		case 0:
			THROW(E1, "timeout on read.");
		case -1:
			THROW(E2, "error while waiting for read.");
		default:
			return ;
	}
}

template <typename E1, typename E2>
void waitForRead(int fd, const fd_set &set, int timeoutInSeconds) {
	if (-1 == fd)
		THROW(std::runtime_error, "invalid fd.");
	struct timeval timeout { .tv_sec = timeoutInSeconds, .tv_usec = 0, };
	waitForRead<E1, E2>(fd, set, &timeout);
}

#endif
