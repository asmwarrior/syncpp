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

//Common types and functions.

#include <sstream>
#include <string>

#include "commons.h"

namespace ns = synbin;

//
//FilePos
//

void ns::FilePos::print(std::ostream& out) const {
	out << m_file_name;
	if (m_line != -1){
		out << '(' << (m_line + 1);
		if (m_column != -1) out << ':' << (m_column + 1);
		out << ')';
	}
}

//
//TextException
//

std::string ns::TextException::to_string() const {
	std::ostringstream sout;
	print(sout);
	return sout.str();
}

void ns::TextException::print(std::ostream& out) const {
	m_pos.print(out);
	out << ": " << m_message;
}
