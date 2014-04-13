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

//Platform-dependent socket functions.

#ifndef SYNSAMPLE_CORE_PLATFORM_SOCKET_H_INCLUDED
#define SYNSAMPLE_CORE_PLATFORM_SOCKET_H_INCLUDED

#include "gc.h"
#include "stringex.h"

namespace syn_script {
	namespace platform {

		//
		//Socket
		//

		class Socket : public gc::Object {
			NONCOPYABLE(Socket);

		protected:
			Socket(){}

		public:
			virtual StringLoc get_remote_host() const = 0;
			virtual int get_remote_port() const = 0;
			virtual void write(const char* buffer, std::size_t count) = 0;
			virtual std::size_t read(char* buffer, std::size_t count) = 0;
			virtual void close() = 0;
		};

		//
		//ServerSocket
		//

		class ServerSocket : public gc::Object {
			NONCOPYABLE(ServerSocket);

		protected:
			ServerSocket(){}

		public:
			virtual gc::Local<Socket> accept() = 0;
			virtual void close() = 0;
		};

		gc::Local<Socket> create_socket(const StringLoc& host, int port);
		gc::Local<ServerSocket> create_server_socket(int port);

	}
}

#endif//SYNSAMPLE_CORE_PLATFORM_SOCKET_H_INCLUDED
