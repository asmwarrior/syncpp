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

//Input/output API.

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>

#include "api.h"
#include "api_basic.h"
#include "api_io.h"
#include "common.h"
#include "gc.h"
#include "scope.h"
#include "stringex.h"
#include "sysclass.h"
#include "sysclassbld__imp.h"
#include "value_core.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace rt = ss::rt;

namespace syn_script {
	namespace rt {
		class StdOutTextOutputValue;
		class FileTextOutputValue;
	}
}

//
//TextOutputValue::API
//

class rt::TextOutputValue::API : public SysAPI<TextOutputValue> {
	void init() override {
		bld->add_method("print", &TextOutputValue::api_print);
		bld->add_method("println", &TextOutputValue::api_println_0);
		bld->add_method("println", &TextOutputValue::api_println_1);
		bld->add_method("close", &TextOutputValue::api_close);
	}
};

//
//TextOutputValue
//

std::size_t rt::TextOutputValue::get_sys_class_id() const {
	return API::get_class_id();
}

void rt::TextOutputValue::close(){}

void rt::TextOutputValue::api_print(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	std::ostream& out = get_out();
	rt::OperandType type = value->get_operand_type();
	if (rt::OperandType::INTEGER == type) {
		out << value->get_integer();
	} else if (rt::OperandType::STRING == type) {
		out << value->get_string();
	} else if (rt::OperandType::FLOAT == type) {
		out << value->get_float();
	} else if (rt::OperandType::BOOLEAN == type) {
		out << (value->get_boolean() ? "true" : "false");
	} else {
		out << value->to_string(context);
	}
	if (out.fail()) throw RuntimeError("Write error");
}

void rt::TextOutputValue::api_println_0(const gc::Local<ExecContext>& context) {
	std::ostream& out = get_out();
	out << std::endl;
	if (out.fail()) throw RuntimeError("Write error");
}

void rt::TextOutputValue::api_println_1(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	api_print(context, value);
	api_println_0(context);
}

void rt::TextOutputValue::api_close(const gc::Local<ExecContext>& context) {
	close();
}

//
//StdOutTextOutputValue : definition
//

class rt::StdOutTextOutputValue : public TextOutputValue {
	NONCOPYABLE(StdOutTextOutputValue);

public:
	StdOutTextOutputValue(){}

	std::ostream& get_out() override;
};

//
//StdOutTextOutputValue : implementation
//

std::ostream& rt::StdOutTextOutputValue::get_out() {
	return std::cout;
}

//
//FileTextOutputValue
//

void rt::FileTextOutputValue::initialize(const ss::StringLoc& path, bool append) {
	const std::string path_str = path->get_std_string();
	std::ios_base::openmode mode = append ? (std::ios_base::app | std::ios_base::ate)
		: static_cast<std::ios_base::openmode>(0);
	m_out.open(path_str, std::ios_base::out | mode);
	if (m_out.fail()) throw RuntimeError(std::string("Unable to open file: ") + path_str);
}

std::ostream& rt::FileTextOutputValue::get_out() {
	return m_out;
}

void rt::FileTextOutputValue::close() {
	m_out.close();
}

//
//BinaryInputValue::API
//

class rt::BinaryInputValue::API : public SysAPI<BinaryInputValue> {
	void init() override {
		bld->add_method("read_byte", &BinaryInputValue::api_read_byte);
		bld->add_method("read", &BinaryInputValue::api_read_1);
		bld->add_method("read", &BinaryInputValue::api_read_3);
		bld->add_method("close", &BinaryInputValue::api_close);
	}
};

//
//BinaryInputValue
//

std::size_t rt::BinaryInputValue::get_sys_class_id() const {
	return API::get_class_id();
}

int rt::BinaryInputValue::read(const gc::Local<ss::ByteArray>& buffer, std::size_t ofs, std::size_t len) {
	std::size_t array_len = buffer->length();
	if (len > array_len || ofs > array_len - len) throw RuntimeError("Index out of bounds");

	static_assert(sizeof(int) <= sizeof(std::size_t), "Wrong type size");
	std::size_t intmax = static_cast<std::size_t>(std::numeric_limits<int>::max());
	if (len > intmax) len = intmax;

	char* ptr = buffer->raw_array() + ofs;
	std::size_t initial_len = len;
	while (len--) {
		int k = read();
		if (k == -1) break;

		assert(k >= 0);
		unsigned char uc = k;
		*ptr++ = reinterpret_cast<char&>(uc);
	}

	std::size_t count = initial_len - len;
	return static_cast<int>(count);
}

int rt::BinaryInputValue::read(const gc::Local<ss::ByteArray>& buffer) {
	return read(buffer, 0, buffer->length());
}

void rt::BinaryInputValue::close(){}

ss::ScriptIntegerType rt::BinaryInputValue::api_read_byte(const gc::Local<ExecContext>& context) {
	return int_to_scriptint_ex(read());
}

ss::ScriptIntegerType rt::BinaryInputValue::api_read_3(
	const gc::Local<ExecContext>& context,
	const gc::Local<ss::ByteArray>& buffer,
	ss::ScriptIntegerType ofs,
	ss::ScriptIntegerType len)
{
	std::size_t sofs = scriptint_to_size_ex(ofs);
	std::size_t slen = scriptint_to_size_ex(len);
	return read(buffer, sofs, slen);
}

ss::ScriptIntegerType rt::BinaryInputValue::api_read_1(
	const gc::Local<ExecContext>& context,
	const gc::Local<ss::ByteArray>& buffer)
{
	return read(buffer);
}

void rt::BinaryInputValue::api_close(const gc::Local<ExecContext>& context) {
	close();
}

//
//BinaryOutputValue::API
//

class rt::BinaryOutputValue::API : public SysAPI<BinaryOutputValue> {
	void init() override {
		bld->add_method("write_byte", &BinaryOutputValue::api_write_byte);
		bld->add_method("write", &BinaryOutputValue::api_write_1);
		bld->add_method("write", &BinaryOutputValue::api_write_3);
		bld->add_method("close", &BinaryOutputValue::api_close);
	}
};

//
//BinaryOutputValue
//

std::size_t rt::BinaryOutputValue::get_sys_class_id() const {
	return API::get_class_id();
}

void rt::BinaryOutputValue::write(const gc::Local<ss::ByteArray>& buffer, std::size_t ofs, std::size_t len) {
	std::size_t array_len = buffer->length();
	if (len > array_len || ofs > array_len - len) throw RuntimeError("Index out of bounds");

	const char* ptr = buffer->raw_array() + ofs;
	while (len--) {
		char c = *ptr++;
		int k = reinterpret_cast<unsigned char&>(c);
		write(k);
	}
}

void rt::BinaryOutputValue::write(const gc::Local<ss::ByteArray>& buffer) {
	write(buffer, 0, buffer->length());
}

void rt::BinaryOutputValue::close(){}

void rt::BinaryOutputValue::api_write_byte(const gc::Local<ExecContext>& context, ss::ScriptIntegerType value) {
	int ivalue = scriptint_to_int_ex(value);
	write(ivalue);
}

void rt::BinaryOutputValue::api_write_3(
	const gc::Local<ExecContext>& context,
	const gc::Local<ss::ByteArray>& buffer,
	ss::ScriptIntegerType ofs,
	ss::ScriptIntegerType len)
{
	std::size_t sofs = scriptint_to_size_ex(ofs);
	std::size_t slen = scriptint_to_size_ex(len);
	write(buffer, sofs, slen);
}

void rt::BinaryOutputValue::api_write_1(const gc::Local<ExecContext>& context, const gc::Local<ss::ByteArray>& buffer) {
	write(buffer);
}

void rt::BinaryOutputValue::api_close(const gc::Local<ExecContext>& context) {
	close();
}

//
//FileBinaryInputValue
//

void rt::FileBinaryInputValue::initialize(const ss::StringLoc& path) {
	const std::string path_str = path->get_std_string();
	m_in.open(path_str, std::ios_base::out | std::ios_base::binary);
	if (m_in.fail()) throw RuntimeError(std::string("Unable to open file: ") + path_str);
}

int rt::FileBinaryInputValue::read() {
	char c;
	m_in.read(&c, 1);
	if (m_in.eof()) return -1;
	if (m_in.fail()) throw RuntimeError("Read error");

	return reinterpret_cast<unsigned char&>(c);
}

int rt::FileBinaryInputValue::read(const gc::Local<ss::ByteArray>& buffer, std::size_t ofs, std::size_t len) {
	std::size_t array_len = buffer->length();
	if (len > array_len || ofs > array_len - len) throw RuntimeError("Index out of bounds");

	const std::streamsize sizemax = std::numeric_limits<std::streamsize>::max();
	if (len > sizemax) len = static_cast<std::size_t>(sizemax);

	static_assert(sizeof(int) <= sizeof(std::size_t), "Wrong sizeof");
	const std::size_t intmax = static_cast<std::size_t>(std::numeric_limits<int>::max());
	if (len > intmax) len = static_cast<std::size_t>(intmax);

	char* ptr = buffer->raw_array() + ofs;
	m_in.read(ptr, len);

	if (m_in.eof()) {
		std::streamsize count = m_in.gcount();
		return count > 0 ? static_cast<int>(count) : -1;
	} else if (m_in.fail()) {
		throw RuntimeError("Read error");
	} else {
		return static_cast<int>(len);
	}
}

void rt::FileBinaryInputValue::close() {
	m_in.close();
}

//
//FileBinaryOutputValue
//

void rt::FileBinaryOutputValue::initialize(const ss::StringLoc& path, bool append) {
	const std::string path_str = path->get_std_string();
	std::ios_base::openmode mode = append ? (std::ios_base::app | std::ios_base::ate)
		: static_cast<std::ios_base::openmode>(0);
	m_out.open(path_str, std::ios_base::out | std::ios_base::binary | mode);
	if (m_out.fail()) throw RuntimeError(std::string("Unable to open file: ") + path_str);
}

void rt::FileBinaryOutputValue::write(int value) {
	assert(value >= 0);
	unsigned char uc = value;
	m_out.put(reinterpret_cast<char&>(uc));
	if (m_out.fail()) throw RuntimeError("Write error");
}

void rt::FileBinaryOutputValue::write(const gc::Local<ss::ByteArray>& buffer, std::size_t ofs, std::size_t len) {
	std::size_t array_len = buffer->length();
	if (len > array_len || ofs > array_len - len) throw RuntimeError("Index out of bounds");

	const char* ptr = buffer->raw_array() + ofs;
	m_out.write(ptr, len);

	if (m_out.fail()) throw RuntimeError("Write error");
}

void rt::FileBinaryOutputValue::close() {
	m_out.close();
}

//
//(Functions)
//

namespace {
	rt::SysNamespaceInitializer s_sys_namespace_initializer([](rt::SysClassBuilder<rt::SysNamespaceValue>& bld){
		bld.add_static_field("out", gc::create<rt::StdOutTextOutputValue>());
	});
}

void link__api_io(){}
