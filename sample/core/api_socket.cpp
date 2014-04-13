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

//Socket API.

#include <iostream>
#include <memory>
#include <ostream>

#include "api.h"
#include "api_basic.h"
#include "common.h"
#include "gc.h"
#include "platform_socket.h"
#include "scope.h"
#include "sysclass.h"
#include "sysclassbld__imp.h"
#include "value_core.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace rt = ss::rt;
namespace pf = ss::platform;

namespace syn_script {
	namespace rt {
		class SocketValue;
		class ServerSocketValue;
	}
}

//
//SocketValue : definition
//

class rt::SocketValue : public SysObjectValue {
	NONCOPYABLE(SocketValue);

	gc::Ref<pf::Socket> m_socket;

public:
	SocketValue(){}
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<pf::Socket>& socket);

protected:
	std::size_t get_sys_class_id() const override;

private:
	int read();
	int read(const gc::Local<ByteArray>& buffer);
	int read(const gc::Local<ByteArray>& buffer, std::size_t ofs, std::size_t len);

	void write(int value);
	void write(const gc::Local<ByteArray>& buffer);
	void write(const gc::Local<ByteArray>& buffer, std::size_t ofs, std::size_t len);

	static gc::Local<SocketValue> api_create(
		const gc::Local<ExecContext>& context,
		const StringLoc& host,
		ScriptIntegerType port);

	StringLoc api_get_remote_host(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_get_remote_port(const gc::Local<ExecContext>& context);

	ScriptIntegerType api_read_byte(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_read_1(const gc::Local<ExecContext>& context, const gc::Local<ByteArray>& buffer);
	
	ScriptIntegerType api_read_3(
		const gc::Local<ExecContext>& context,
		const gc::Local<ByteArray>& buffer,
		ScriptIntegerType ofs,
		ScriptIntegerType len);

	void api_write_byte(const gc::Local<ExecContext>& context, ScriptIntegerType value);
	void api_write_1(const gc::Local<ExecContext>& context, const gc::Local<ByteArray>& buffer);

	void api_write_3(
		const gc::Local<ExecContext>& context,
		const gc::Local<ByteArray>& buffer,
		ScriptIntegerType ofs,
		ScriptIntegerType len);

	void api_close(const gc::Local<ExecContext>& context);

public:
	class API;
};

//
//ServerSocketValue : definition
//

class rt::ServerSocketValue : public SysObjectValue {
	NONCOPYABLE(ServerSocketValue);

	gc::Ref<pf::ServerSocket> m_server_socket;

public:
	ServerSocketValue(){}
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<pf::ServerSocket>& server_socket);

protected:
	std::size_t get_sys_class_id() const override;

private:
	static gc::Local<ServerSocketValue> api_create(const gc::Local<ExecContext>& context, ScriptIntegerType port);
	gc::Local<SocketValue> api_accept(const gc::Local<ExecContext>& context);
	void api_close(const gc::Local<ExecContext>& context);

public:
	class API;
};

//
//SocketValue::API
//

class rt::SocketValue::API : public SysAPI<SocketValue> {
	void init() override {
		bld->add_constructor(api_create);
		bld->add_method("get_remote_host", &SocketValue::api_get_remote_host);
		bld->add_method("get_remote_port", &SocketValue::api_get_remote_port);
		bld->add_method("read_byte", &SocketValue::api_read_byte);
		bld->add_method("read", &SocketValue::api_read_1);
		bld->add_method("read", &SocketValue::api_read_3);
		bld->add_method("write_byte", &SocketValue::api_write_byte);
		bld->add_method("write", &SocketValue::api_write_1);
		bld->add_method("write", &SocketValue::api_write_3);
		bld->add_method("close", &SocketValue::api_close);
	}
};

//
//SocketValue : implementation
//

void rt::SocketValue::gc_enumerate_refs() {
	gc_ref(m_socket);
}

void rt::SocketValue::initialize(const gc::Local<pf::Socket>& socket) {
	assert(!!socket);
	m_socket = socket;
}

std::size_t rt::SocketValue::get_sys_class_id() const {
	return API::get_class_id();
}

int rt::SocketValue::read() {
	char buffer;
	std::size_t count = m_socket->read(&buffer, 1);
	unsigned char uc = reinterpret_cast<unsigned char&>(buffer);
	return count ? uc : -1;
}

int rt::SocketValue::read(const gc::Local<ss::ByteArray>& buffer) {
	return read(buffer, 0, buffer->length());
}

int rt::SocketValue::read(const gc::Local<ss::ByteArray>& buffer, std::size_t ofs, std::size_t len) {
	static_assert(sizeof(int) <= sizeof(std::size_t), "Wrong type size");
	std::size_t intmax = static_cast<std::size_t>(std::numeric_limits<int>::max());
	if (len > intmax) len = intmax;

	std::size_t count = m_socket->read(buffer->raw_array() + ofs, len);
	if (!count) return -1;
	return static_cast<int>(count);
}

void rt::SocketValue::write(int value) {
	unsigned char uc = value;
	char buffer = reinterpret_cast<char&>(uc);
	m_socket->write(&buffer, 1);
}

void rt::SocketValue::write(const gc::Local<ss::ByteArray>& buffer) {
	write(buffer, 0, buffer->length());
}

void rt::SocketValue::write(const gc::Local<ss::ByteArray>& buffer, std::size_t ofs, std::size_t len) {
	m_socket->write(buffer->raw_array() + ofs, len);
}

gc::Local<rt::SocketValue> rt::SocketValue::api_create(
	const gc::Local<ExecContext>& context,
	const ss::StringLoc& host,
	ss::ScriptIntegerType port)
{
	if (!host) throw RuntimeError("Host is null");
	int iport = scriptint_to_int_ex(port);
	gc::Local<pf::Socket> socket = pf::create_socket(host, iport);
	return gc::create<SocketValue>(socket);
}

ss::StringLoc rt::SocketValue::api_get_remote_host(const gc::Local<ExecContext>& context) {
	return m_socket->get_remote_host();
}

ss::ScriptIntegerType rt::SocketValue::api_get_remote_port(const gc::Local<ExecContext>& context) {
	return m_socket->get_remote_port();
}

ss::ScriptIntegerType rt::SocketValue::api_read_byte(const gc::Local<ExecContext>& context) {
	return int_to_scriptint_ex(read());
}

ss::ScriptIntegerType rt::SocketValue::api_read_1(
	const gc::Local<ExecContext>& context,
	const gc::Local<ss::ByteArray>& buffer)
{
	if (!buffer) throw RuntimeError("Buffer is null");
	return int_to_scriptint_ex(read(buffer));
}
	
ss::ScriptIntegerType rt::SocketValue::api_read_3(
	const gc::Local<ExecContext>& context,
	const gc::Local<ss::ByteArray>& buffer,
	ss::ScriptIntegerType ofs,
	ss::ScriptIntegerType len)
{
	if (!buffer) throw RuntimeError("Buffer is null");
	std::size_t buffer_size = buffer->length();
	if (len > buffer_size || ofs > buffer_size - len) throw RuntimeError("Index out of bounds");

	std::size_t sofs = scriptint_to_size_ex(ofs);
	std::size_t slen = scriptint_to_size_ex(len);
	return read(buffer, sofs, slen);
}

void rt::SocketValue::api_write_byte(const gc::Local<ExecContext>& context, ss::ScriptIntegerType value) {
	int ivalue = scriptint_to_int_ex(value);
	write(ivalue);
}

void rt::SocketValue::api_write_1(
	const gc::Local<ExecContext>& context,
	const gc::Local<ss::ByteArray>& buffer)
{
	if (!buffer) throw RuntimeError("Buffer is null");
	write(buffer);
}

void rt::SocketValue::api_write_3(
	const gc::Local<ExecContext>& context,
	const gc::Local<ss::ByteArray>& buffer,
	ss::ScriptIntegerType ofs,
	ss::ScriptIntegerType len)
{
	if (!buffer) throw RuntimeError("Buffer is null");

	std::size_t sofs = scriptint_to_size_ex(ofs);
	std::size_t slen = scriptint_to_size_ex(len);
	write(buffer, sofs, slen);
}

void rt::SocketValue::api_close(const gc::Local<ExecContext>& context) {
	m_socket->close();
}

//
//ServerSocketValue::API
//

class rt::ServerSocketValue::API : public SysAPI<ServerSocketValue> {
	void init() override {
		bld->add_constructor(api_create);
		bld->add_method("accept", &ServerSocketValue::api_accept);
		bld->add_method("close", &ServerSocketValue::api_close);
	}
};

//
//ServerSocketValue : implementation
//

void rt::ServerSocketValue::gc_enumerate_refs() {
	gc_ref(m_server_socket);
}

void rt::ServerSocketValue::initialize(const gc::Local<pf::ServerSocket>& server_socket) {
	assert(!!server_socket);
	m_server_socket = server_socket;
}

std::size_t rt::ServerSocketValue::get_sys_class_id() const {
	return API::get_class_id();
}

gc::Local<rt::ServerSocketValue> rt::ServerSocketValue::api_create(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType port)
{
	int iport = scriptint_to_int_ex(port);
	gc::Local<pf::ServerSocket> server_socket = pf::create_server_socket(iport);
	return gc::create<ServerSocketValue>(server_socket);
}

gc::Local<rt::SocketValue> rt::ServerSocketValue::api_accept(const gc::Local<ExecContext>& context) {
	gc::Local<pf::Socket> socket = m_server_socket->accept();
	return gc::create<SocketValue>(socket);
}

void rt::ServerSocketValue::api_close(const gc::Local<ExecContext>& context) {
	m_server_socket->close();
}

//
//(Functions)
//

namespace {
	rt::SysNamespaceInitializer s_sys_namespace_initializer([](rt::SysClassBuilder<rt::SysNamespaceValue>& bld){
		bld.add_class<rt::SocketValue>("Socket");
		bld.add_class<rt::ServerSocketValue>("ServerSocket");
	});
}

void link__api_socket(){}
