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

#ifndef SYN_CORE_COMMONS_H_INCLUDED
#define SYN_CORE_COMMONS_H_INCLUDED

#include <cassert>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>

#include "util_string.h"

namespace synbin {

	extern const std::string g_empty_std_string;
	
	//
	//Exception
	//

	class Exception {
	protected:
		const std::string m_message;

	public:
		Exception(const std::string& message = "") : m_message(message){}
		virtual ~Exception(){}

		const std::string& message() const { return m_message; }
		virtual std::string to_string() const { return m_message; }
		virtual void print(std::ostream& out) const { out << m_message; }
	};

	//
	//TextPos
	//

	struct TextPos {
		int m_line;
		int m_column;

		TextPos() : m_line(-1), m_column(-1){}
		TextPos(int line, int column) : m_line(line), m_column(column){}
	};

	//
	//FilePos
	//

	class FilePos {
		util::String m_file_name;
		int m_line;
		int m_column;

	public:
		FilePos() : m_line(-1), m_column(-1){}

		FilePos(const util::String& file_name, TextPos text_pos)
			: m_file_name(file_name), m_line(text_pos.m_line), m_column(text_pos.m_column)
		{}

		void print(std::ostream& out) const;
	};

	//
	//TextException
	//

	class TextException : public Exception {
		const FilePos m_pos;

	public:
		TextException(const std::string& message, const FilePos& pos) : Exception(message), m_pos(pos){}

		const FilePos& pos() const { return m_pos; }

		std::string to_string() const override;
		void print(std::ostream& out) const override;
	};

	//
	//(Functions)
	//

	//GCC 4.8 does not support std::make_unique, so a custom one has to be provided (it is not complete, but acceptable).
	template<class T, class... Args>
	std::unique_ptr<T> make_unique1(Args... args) {
		return std::unique_ptr<T>(new T(std::move(args)...));
	}

	inline std::exception err_illegal_state() {
		assert(false);
		throw std::logic_error("illegal state");
	}
}

#endif//SYN_CORE_COMMONS_H_INCLUDED
