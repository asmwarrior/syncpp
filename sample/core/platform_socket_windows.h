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

//Windows socket definitions.

#ifndef SYNSAMPLE_CORE_PLATFORM_SOCKET_WINDOWS_H_INCLUDED
#define SYNSAMPLE_CORE_PLATFORM_SOCKET_WINDOWS_H_INCLUDED

//This definition must come before #include <Windows.h>, etc.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

namespace syn_script {
	namespace platform {

		typedef SOCKET PF_SOCKET_HANDLE;
		const PF_SOCKET_HANDLE PF_INVALID_SOCKET = INVALID_SOCKET;
		const int PF_SOCKET_ERROR = SOCKET_ERROR;

		void pf_socket_startup();
		inline void pf_socket_shutdown(SOCKET sock) { shutdown(sock, SD_BOTH); }
		inline void pf_socket_close(SOCKET sock) { closesocket(sock); }
		inline int pf_socket_error_code() { return WSAGetLastError(); }

		inline int pf_socket_timeout(PF_SOCKET_HANDLE socket, int option, int timeout_ms) {
			DWORD timeout = timeout_ms;
			const char* ptr = static_cast<const char*>(static_cast<const void*>(&timeout));
			return setsockopt(socket, SOL_SOCKET, option, ptr, sizeof(timeout));
		}

	}
}


#endif//SYNSAMPLE_CORE_PLATFORM_SOCKET_WINDOWS_H_INCLUDED
