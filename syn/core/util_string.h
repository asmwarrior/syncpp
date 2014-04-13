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

//Fast, thread-unsafe reference-counting string.

#ifndef SYN_CORE_UTIL_STRING_H_INCLUDED
#define SYN_CORE_UTIL_STRING_H_INCLUDED

#include <memory>
#include <ostream>
#include <string>

#include "cntptr.h"

namespace synbin {
	namespace util {
		class String;

		bool operator==(const String& str1, const String& str2);
		bool operator!=(const String& str1, const String& str2);
		bool operator<(const String& str1, const String& str2);
		bool operator>(const String& str1, const String& str2);
		bool operator<=(const String& str1, const String& str2);
		bool operator>=(const String& str1, const String& str2);

		std::ostream& operator<<(std::ostream& out, const String& str);
	}
}

//
//String
//

class synbin::util::String {

	struct SharedBlock : public RefCnt {
		const std::string m_str;

		SharedBlock(){}
		SharedBlock(const std::string& str) : m_str(str){}
		SharedBlock(const char* str) : m_str(str){}
	};

	CntPtr<SharedBlock> m_shared_block;

public:
	String(const String& other_string);
	String& operator=(const String& other_string);

	String();
	explicit String(const std::string& std_string);
	explicit String(const char* c_string);

	const std::string& str() const;
	bool empty() const;
};

#endif//SYN_CORE_UTIL_STRING_H_INCLUDED
