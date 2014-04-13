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

//EBNF Grammar parser definitions.

#ifndef SYN_CORE_GRM_PARSER_H_INCLUDED
#define SYN_CORE_GRM_PARSER_H_INCLUDED

#include <memory>
#include <string>

#include "commons.h"
#include "ebnf__imp.h"
#include "grm_parser_res.h"
#include "util_mptr.h"
#include "util_string.h"

namespace synbin {
	namespace grm_parser {

		//
		//ParserException
		//

		class ParserException : public TextException {
		public:
			ParserException(const std::string& message, const FilePos& pos)
				: TextException(message, pos)
			{}

			ParserException(const std::string& message, const util::String& file_name)
				: TextException(message, FilePos(file_name, TextPos()))
			{}

			ParserException(const std::string& message, const util::String& file_name, TextPos text_pos)
				: TextException(message, FilePos(file_name, text_pos))
			{}
		};

		//Parse EBNF grammar.
		std::unique_ptr<GrammarParsingResult> parse_grammar(std::istream& in, const util::String& file_name);
	
	}
}

#endif//SYN_CORE_GRM_PARSER_H_INCLUDED
