/*
 * Copyright 2014 Anton Karmanov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//Linux socket definitions.

#ifndef SYNSAMPLE_CORE_PLATFORM_SOCKET_LINUX_H_INCLUDED
#define SYNSAMPLE_CORE_PLATFORM_SOCKET_LINUX_H_INCLUDED

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

namespace syn_script {
	namespace platform {

		typedef int PF_SOCKET_HANDLE;
		const int PF_INVALID_SOCKET = -1;
		const int PF_SOCKET_ERROR = -1;

		inline void pf_socket_startup(){}
		inline void pf_socket_shutdown(int sock) { shutdown(sock, SHUT_RDWR); };
		inline void pf_socket_close(int sock) { close(sock); }
		inline int pf_socket_error_code() { return errno; }

		inline int pf_socket_timeout(PF_SOCKET_HANDLE socket, int option, int timeout_ms) {
			struct timeval tv;
			tv.tv_sec = timeout_ms / 1000;
			tv.tv_usec = (timeout_ms % 1000) * 1000;
			return setsockopt(socket, SOL_SOCKET, option, &tv, sizeof(tv));
		}

	}
}

#endif//SYNSAMPLE_CORE_PLATFORM_SOCKET_LINUX_H_INCLUDED
