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

//Common classes.

#include <sstream>
#include <string>

#include "common.h"
#include "gc.h"
#include "stringex.h"

namespace ss = syn_script;
namespace gc = ss::gc;

//
//TextPos
//

void ss::TextPos::gc_enumerate_refs() {
	gc_ref(m_file_name);
}

void ss::TextPos::initialize(const StringLoc& file_name, int line, int column) {
	m_file_name = file_name;
	m_line = line;
	m_column = column;
}

ss::StringLoc ss::TextPos::file_name() const {
	return m_file_name;
}

int ss::TextPos::line() const {
	return m_line;
}

int ss::TextPos::column() const {
	return m_column;
}

ss::TextPos::operator bool() const {
	return !!m_file_name;
}

namespace {
	void print_text_pos(std::ostream& out, const ss::TextPos* text_pos) {
		if (!text_pos) {
			out << "?";
		} else {
			out << text_pos->file_name();
			int line = text_pos->line();
			if (line != -1) out << "(" << (line + 1) << ")";
		}
	}
}

std::ostream& ss::operator<<(std::ostream& out, const gc::Local<TextPos>& text_pos) {
	print_text_pos(out, text_pos.get());
	return out;
}

std::ostream& ss::operator<<(std::ostream& out, const gc::Ref<TextPos>& text_pos) {
	print_text_pos(out, text_pos.get());
	return out;
}

//
//BasicError
//

ss::BasicError::BasicError(const char* msg)
: m_msg(msg)
{}

ss::BasicError::BasicError(const std::string& msg)
: m_msg(msg)
{}

ss::BasicError::BasicError(const gc::Local<TextPos>& pos, const char* msg)
: m_pos(pos), m_msg(msg)
{}

ss::BasicError::BasicError(const gc::Local<TextPos>& pos, const std::string& msg)
: m_pos(pos), m_msg(msg)
{}

gc::Local<ss::TextPos> ss::BasicError::get_pos() const {
	return m_pos;
}

const std::string& ss::BasicError::get_msg() const {
	return m_msg;
}

std::string ss::BasicError::to_string() const {
	std::ostringstream out;
	out << *this;
	return out.str();
}

std::ostream& ss::operator<<(std::ostream& out, const ss::BasicError& error) {
	if (!!error.get_pos()) out << error.get_pos() << " ";
	out << error.get_error_type() << " error: " << error.get_msg();
	return out;
}

//
//CompilationError
//

ss::CompilationError::CompilationError(const std::string& msg)
: BasicError(msg)
{}

ss::CompilationError::CompilationError(const gc::Local<TextPos>& pos, const char* msg)
: BasicError(pos, msg)
{}

ss::CompilationError::CompilationError(const gc::Local<TextPos>& pos, const std::string& msg)
: BasicError(pos, msg)
{}

const char* ss::CompilationError::get_error_type() const {
	return "Compilation";
}

//
//RuntimeError
//

ss::RuntimeError::RuntimeError(const char* msg)
: BasicError(msg)
{}

ss::RuntimeError::RuntimeError(const std::string& msg)
: BasicError(msg)
{}

ss::RuntimeError::RuntimeError(const gc::Local<TextPos>& pos, const char* msg)
: BasicError(pos, msg)
{}

ss::RuntimeError::RuntimeError(const gc::Local<TextPos>& pos, const std::string& msg)
: BasicError(pos, msg)
{}

const char* ss::RuntimeError::get_error_type() const {
	return "Run-time";
}

//
//FatalError
//

ss::FatalError::FatalError(const std::string& msg)
: BasicError(msg)
{}

const char* ss::FatalError::get_error_type() const {
	return "Fatal";
}

//
//SystemError
//

ss::SystemError::SystemError(const char* msg)
: BasicError(msg)
{}

ss::SystemError::SystemError(const gc::Local<TextPos>& pos, const char* msg)
: BasicError(pos, msg)
{}

const char* ss::SystemError::get_error_type() const {
	return "System";
}
//
//ObjectEx
//
ss::ObjectEx::ObjectEx(){}

std::size_t ss::ObjectEx::hash_code() const {
	return reinterpret_cast<std::size_t>(this);
}

bool ss::ObjectEx::equals(const gc::Local<const ss::ObjectEx>& obj) const {
	return this == obj.get();
}
