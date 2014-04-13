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

//Common definitions.

#ifndef SYNSAMPLE_CORE_COMMON_H_INCLUDED
#define SYNSAMPLE_CORE_COMMON_H_INCLUDED

#include <cassert>
#include <memory>
#include <ostream>
#include <string>

#include "gc.h"
#include "noncopyable.h"
#include "stringex__dec.h"

namespace syn_script {

	typedef gc::PrimitiveArray<char> ByteArray;

	//
	//TextPos
	//

	class TextPos : public gc::Object {
		NONCOPYABLE(TextPos);

		StringRef m_file_name;
		int m_line;
		int m_column;

	public:
		TextPos(){}
		void gc_enumerate_refs() override;
		void initialize(const StringLoc& file_name = nullptr, int line = -1, int column = -1);

		StringLoc file_name() const;
		int line() const;
		int column() const;

		operator bool() const;
	};

	std::ostream& operator<<(std::ostream& out, const gc::Local<TextPos>& text_pos);
	std::ostream& operator<<(std::ostream& out, const gc::Ref<TextPos>& text_pos);

	//
	//BasicError
	//

	class BasicError {
		const gc::Local<TextPos> m_pos;
		const std::string m_msg;

	public:
		BasicError(const char* msg);
		BasicError(const std::string& msg);
		BasicError(const gc::Local<TextPos>& pos, const char* msg);
		BasicError(const gc::Local<TextPos>& pos, const std::string& msg);

		gc::Local<TextPos> get_pos() const;
		const std::string& get_msg() const;
		std::string to_string() const;

		virtual const char* get_error_type() const = 0;
	};

	std::ostream& operator<<(std::ostream& out, const BasicError& error);

	//
	//CompilationError
	//

	class CompilationError : public BasicError {
	public:
		CompilationError(const std::string& msg);
		CompilationError(const gc::Local<TextPos>& pos, const char* msg);
		CompilationError(const gc::Local<TextPos>& pos, const std::string& msg);

		const char* get_error_type() const override;
	};

	//
	//RuntimeError
	//

	class RuntimeError : public BasicError {
	public:
		RuntimeError(const char* msg);
		RuntimeError(const std::string& msg);
		RuntimeError(const gc::Local<TextPos>& pos, const char* msg);
		RuntimeError(const gc::Local<TextPos>& pos, const std::string& msg);

		const char* get_error_type() const override;
	};

	//
	//FatalError
	//

	class FatalError : public BasicError {
	public:
		FatalError(const std::string& msg);

		const char* get_error_type() const override;
	};

	//
	//SystemError
	//

	class SystemError : public BasicError {
	public:
		SystemError(const char* msg);
		SystemError(const gc::Local<TextPos>& pos, const char* msg);

		const char* get_error_type() const override;
	};

	//
	//ObjectEx
	//

	class ObjectEx : public gc::Object {
		NONCOPYABLE(ObjectEx);

	protected:
		ObjectEx();

	public:
		virtual bool equals(const gc::Local<const ObjectEx>& obj) const;
		virtual std::size_t hash_code() const;
	};

}

#endif//SYNSAMPLE_CORE_COMMON_H_INCLUDED
