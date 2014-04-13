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

//Primitive types and other declarations and definitions.

#ifndef SYN_CORE_PRIMITIVES_H_INCLUDED
#define SYN_CORE_PRIMITIVES_H_INCLUDED

#include <ostream>
#include <string>

#include "commons.h"
#include "util_string.h"

namespace synbin {

		typedef unsigned long syntax_number;
		
		//
		//syntax_string
		//

		//A string with a text position.
		class syntax_string {
			FilePos m_pos;
			util::String m_str;

		public:
			syntax_string(){}

		private:
			explicit syntax_string(const util::String& str) : m_str(str){}

		public:
			syntax_string(const FilePos& pos, const util::String& str) : m_pos(pos), m_str(str){}

			const util::String& get_string() const { return m_str; }
			const FilePos& pos() const { return m_pos; }
			const std::string& str() const { return m_str.str(); }
		};

		inline std::ostream& operator<<(std::ostream& out, const syntax_string& syn_str) {
			return out << syn_str.str();
		}

		extern const syntax_string g_empty_syntax_string;

		bool raise_error(const FilePos& pos, const std::string& message);
		bool raise_error(const syntax_string& cause, const std::string& message);
}

#endif//SYN_CORE_PRIMITIVES_H_INCLUDED
