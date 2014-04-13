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

//EBNF Grammar parser internal definitions.

#ifndef SYN_CORE_GRM_PARSER_IMPL_H_INCLUDED
#define SYN_CORE_GRM_PARSER_IMPL_H_INCLUDED

#include <istream>
#include <vector>

#include "ebnf__dec.h"
#include "grm_parser.h"
#include "noncopyable.h"
#include "primitives.h"
#include "util_string.h"

namespace synbin {
	namespace grm_parser {

		struct TokenRecord;

		//
		//Tokens
		//

		struct Tokens {
			enum E {
				END_OF_FILE,

				NAME,
				NUMBER,
				STRING,

				KW_CLASS,
				KW_TOKEN,
				KW_TYPE,
				KW_THIS,
				KW_FALSE,
				KW_TRUE,

				CH_SEMICOLON,
				CH_AT,
				CH_COLON,
				CH_OBRACE,
				CH_CBRACE,
				CH_OR,
				CH_EQ,
				CH_OPAREN,
				CH_CPAREN,
				CH_QUESTION,
				CH_ASTERISK,
				CH_PLUS,
				CH_LT,
				CH_GT,
				CH_COLON_COLON,
				CH_COMMA,
				CH_DOT,
				CH_MINUS_GT
			};
		};

		typedef int token_number;

		//
		//TokenRecord
		//

		//Describes a token.
		struct TokenRecord {
			Tokens::E token;
			token_number v_number;
			syntax_string v_string;
			TextPos pos;
		};

		//
		//Scanner
		//

		class Scanner {
			NONCOPYABLE(Scanner);

			std::istream& m_in;
			char m_current;
			bool m_eof;

			const util::String m_file_name;
			TextPos m_text_pos;
			TextPos m_next_text_pos;

			std::string m_string_buffer;

			bool scan_blank();
			void scan_single_line_comment();
			void scan_multiline_comment();
			void scan_number(TokenRecord* token_record);
			void scan_name(TokenRecord* token_record);
			void scan_keyword(TokenRecord* token_record);
			void scan_name_buffer();
			void scan_string_literal(TokenRecord* token_record);
			void scan_key_character(TokenRecord* token_record);
			void next_char();

		public:
			explicit Scanner(
				std::istream& in,
				const util::String& file_name);
			
			void scan_token(TokenRecord* token_record);
			
			const util::String& file_name() const {
				return m_file_name;
			}
		};

	}
}

#endif//SYN_CORE_GRM_PARSER_IMPL_H_INCLUDED
