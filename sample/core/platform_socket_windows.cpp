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

//Windows platform-dependent socket functions.

//Winsock headers must be included at the beginning.
#ifdef _MSC_VER
#include "platform_socket_windows.h"
#else
#include "platform_socket_linux.h"
#endif

#include <cstring>
#include <iostream>

#include "gc.h"
#include "platform.h"
#include "platform_socket.h"
#include "stringex.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace pf = ss::platform;

namespace {
	class WinsockStartup {
		NONCOPYABLE(WinsockStartup);

		bool m_ok;

		WinsockStartup() : m_ok(false) {
			WSADATA wsaData;
			int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
			m_ok = !iResult;
		}

		//WSACleanup is not called for simplicity.

	public:
		static void startup() {
			static WinsockStartup startup;
			if (!startup.m_ok) throw ss::RuntimeError("WSAStartup() failed");
		}
	};
}

void pf::pf_socket_startup() {
	WinsockStartup::startup();
}
