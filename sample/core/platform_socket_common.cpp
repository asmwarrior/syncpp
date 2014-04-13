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

//Platform-independent socket functions.

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

namespace syn_script {
	namespace platform {
		class SocketAddrInfo;
		class ConcreteSocket;
		class ConcreteServerSocket;
	}
}

namespace {
	ss::RuntimeError socket_error(const std::string& msg) {
		std::string full_msg = std::string("Socket error (")
			+ std::to_string(pf::pf_socket_error_code()) + "): " + msg;
		throw ss::RuntimeError(full_msg);
	}
}

//
//SocketAddrInfo
//

class pf::SocketAddrInfo {
	NONCOPYABLE(SocketAddrInfo);

	struct addrinfo* m_addr;

public:
	SocketAddrInfo(const char* node_name, const char* service_name) : m_addr(nullptr) {
		struct addrinfo hints;
		std::memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		int iResult = getaddrinfo(node_name, service_name, &hints, &m_addr);
		if (iResult) throw socket_error("getaddrinfo() failed");
		if (AF_INET != m_addr->ai_family) {
			freeaddrinfo(m_addr);
			throw RuntimeError("Socket error: wrong address family");
		}
	}

	~SocketAddrInfo() {
		freeaddrinfo(m_addr);
	}

	const struct addrinfo* get_addr() const {
		return m_addr;
	}
};

//
//ConcreteSocket : definition
//

class pf::ConcreteSocket : public Socket {
	NONCOPYABLE(ConcreteSocket);

	PF_SOCKET_HANDLE m_socket;
	StringRef m_remote_host;
	int m_remote_port;

public:
	ConcreteSocket();
	void gc_enumerate_refs() override;
	void initialize(PF_SOCKET_HANDLE socket, const StringLoc& remote_host, int remote_port);
	void initialize(const ss::StringLoc& host, int port);

private:
	void init_timeouts();

public:
	StringLoc get_remote_host() const override;
	int get_remote_port() const override;
	void write(const char* buffer, std::size_t count) override;
	std::size_t read(char* buffer, std::size_t count) override;
	void close() override;
};

//
//ConcreteSocket : implementation
//

pf::ConcreteSocket::ConcreteSocket() : m_socket(PF_INVALID_SOCKET){}

void pf::ConcreteSocket::gc_enumerate_refs() {
	Socket::gc_enumerate_refs();
	gc_ref(m_remote_host);
}

void pf::ConcreteSocket::initialize(
	PF_SOCKET_HANDLE socket,
	const ss::StringLoc& remote_host,
	int remote_port)
{
	m_socket = socket;
	m_remote_host = remote_host;
	m_remote_port = remote_port;

	init_timeouts();
}

void pf::ConcreteSocket::initialize(const ss::StringLoc& host, int port) {
	pf::pf_socket_startup();

	const std::string host_str = host->get_std_string();
	const std::string port_str = std::to_string(port);
	SocketAddrInfo addr_guard(host_str.c_str(), port_str.c_str());

	const struct addrinfo* addr = addr_guard.get_addr();

	m_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (PF_INVALID_SOCKET == m_socket) throw socket_error("socket() failed");

	int iResult = connect(m_socket, addr->ai_addr, (int)addr->ai_addrlen);
	if (PF_SOCKET_ERROR == iResult) {
		pf_socket_close(m_socket);
		throw socket_error("connect() failed");
	}

	m_remote_host = host;
	m_remote_port = port;

	init_timeouts();
}

void pf::ConcreteSocket::init_timeouts() {
	const int timeout = 3000;

	if (PF_SOCKET_ERROR == pf_socket_timeout(m_socket, SO_RCVTIMEO, timeout)) {
		pf_socket_close(m_socket);
		throw socket_error("setsockopt(SO_RCVTIMEO) failed");
	}

	if (PF_SOCKET_ERROR == pf_socket_timeout(m_socket, SO_SNDTIMEO, timeout)) {
		pf_socket_close(m_socket);
		throw socket_error("setsockopt(SO_SNDTIMEO) failed");
	}
}

ss::StringLoc pf::ConcreteSocket::get_remote_host() const {
	return m_remote_host;
}

int pf::ConcreteSocket::get_remote_port() const {
	return m_remote_port;
}

void pf::ConcreteSocket::write(const char* buffer, std::size_t count) {
	if (!count) return;

	const unsigned int max_int = std::numeric_limits<int>::max();
	const unsigned int block_size = count < max_int ? static_cast<unsigned int>(count) : max_int;
	while (count) {
		int len = count < block_size ? static_cast<int>(count) : block_size;
		int iResult = send(m_socket, buffer, len, 0);
		if (PF_SOCKET_ERROR == iResult) throw socket_error("send() failed");
		count -= iResult;
		buffer += iResult;
	}
}

std::size_t pf::ConcreteSocket::read(char* buffer, std::size_t count) {
	if (!count) return 0;

	const unsigned int max_int = std::numeric_limits<int>::max();
	int len = count < max_int ? static_cast<int>(count) : max_int;
	int iResult = recv(m_socket, buffer, len, 0);
	if (PF_SOCKET_ERROR == iResult) throw socket_error("recv() failed");

	return iResult;
}

void pf::ConcreteSocket::close() {
	pf_socket_shutdown(m_socket);
	pf_socket_close(m_socket);
}

//
//ConcreteServerSocket : definition
//

class pf::ConcreteServerSocket : public ServerSocket {
	NONCOPYABLE(ConcreteServerSocket);

	PF_SOCKET_HANDLE m_server_socket;

public:
	ConcreteServerSocket();
	void initialize(int port);

	gc::Local<Socket> accept() override;
	void close() override;
};

//
//ConcreteServerSocket : implementation
//

pf::ConcreteServerSocket::ConcreteServerSocket() : m_server_socket(PF_INVALID_SOCKET){}

void pf::ConcreteServerSocket::initialize(int port) {
	pf::pf_socket_startup();

	const std::string port_str = std::to_string(port);

	sockaddr_in addr;//TODO Ensure that using sockaddr_in is OK for Linux.
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	m_server_socket = socket(addr.sin_family, SOCK_STREAM, IPPROTO_TCP);
	if (PF_INVALID_SOCKET == m_server_socket) throw socket_error("socket() failed");

	int iResult = bind(m_server_socket, (struct sockaddr*)&addr, sizeof(addr));
	if (PF_SOCKET_ERROR == iResult) {
		pf_socket_close(m_server_socket);
		throw socket_error("bind() failed");
	}
	
	if (PF_SOCKET_ERROR == listen(m_server_socket, SOMAXCONN)) {
		pf_socket_close(m_server_socket);
		throw socket_error("listen() failed");
	}
}

gc::Local<pf::Socket> pf::ConcreteServerSocket::accept() {
	PF_SOCKET_HANDLE socket = ::accept(m_server_socket, NULL, NULL);
	if (PF_INVALID_SOCKET == socket) throw socket_error("accept() failed");

	sockaddr_in addr;
	socklen_t size = sizeof(addr);
	std::memset(&addr, 0, size);
	if (getpeername(socket, (struct sockaddr*)&addr, &size)) throw socket_error("getpeername() failed");

	const char* ip = inet_ntoa(addr.sin_addr);
	StringLoc host = gc::create<String>(ip);
	int port = ntohs(addr.sin_port);

	return gc::create<ConcreteSocket>(socket, host, port);
}

void pf::ConcreteServerSocket::close() {
	pf_socket_shutdown(m_server_socket);
	pf_socket_close(m_server_socket);
}

//
//(Functions).
//

gc::Local<pf::Socket> pf::create_socket(const ss::StringLoc& host, int port) {
	return gc::create<ConcreteSocket>(host, port);
}

gc::Local<pf::ServerSocket> pf::create_server_socket(int port) {
	return gc::create<ConcreteServerSocket>(port);
}
