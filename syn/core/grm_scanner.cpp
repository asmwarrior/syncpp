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

//Grammar Scanner implementation.

#include <cctype>
#include <fstream>
#include <limits>
#include <map>
#include <string>

#include "primitives.h"
#include "grm_parser.h"
#include "grm_parser_impl.h"
#include "util_string.h"

namespace ns = synbin;
namespace prs = ns::grm_parser;
namespace util = ns::util;

using util::String;

namespace {

	const std::string g_keyword_class = "class";
	const std::string g_keyword_type = "type";
	const std::string g_keyword_token = "token";
	const std::string g_keyword_this = "this";
	const std::string g_keyword_true = "true";
	const std::string g_keyword_false = "false";

	const prs::token_number g_max_number = std::numeric_limits<prs::token_number>::max();
	const prs::token_number g_max_number_div_10 = g_max_number / 10;

	//Functions is_*(int) now simply call corresponding functions from <cctype>. It is more reliable than
	//writing (k >= 'A' && k <= 'Z'), because such code may work not on all C++ implementations.

	bool is_whitespace(int k) {
		return 0 != std::isspace(k);
	}

	bool is_name_start(int k) {
		return 0 != std::isalpha(k) || '_' == k;
	}

	bool is_name_part(int k) {
		return 0 != std::isalnum(k) || '_' == k;
	}

	bool is_digit(int k) {
		return 0 != std::isdigit(k);
	}

	//TODO Make this code implementation-independent.
	prs::token_number char_to_digit(int k) {
		return k - '0';
	}
}

//
//Scanner
//

prs::Scanner::Scanner(
	std::istream& in,
	const util::String& file_name)
	: m_in(in),
	m_file_name(file_name)
{
	m_in.exceptions(std::ios_base::badbit);
	m_eof = false;
	m_text_pos = TextPos(0, 0);
	m_next_text_pos = TextPos(0, 0);
	next_char();
}

void prs::Scanner::scan_token(TokenRecord* token_record) {
	while (!m_eof && scan_blank()){}

	token_record->pos = m_text_pos;

	if (m_eof) {
		token_record->token = Tokens::END_OF_FILE;
		return;
	}

	char c = m_current;
	if (is_digit(c)) {
		scan_number(token_record);
	} else if (is_name_start(c)) {
		scan_name(token_record);
	} else if (c == '%') {
		scan_keyword(token_record);
	} else if (c == '"') {
		scan_string_literal(token_record);
	} else {
		scan_key_character(token_record);
	}
}

bool prs::Scanner::scan_blank() {
	if (is_whitespace(m_current)) {
		next_char();
		return true;
	} else if ('/' == m_current) {
		next_char();
		if (m_eof) throw ParserException("Unexpected end of file", m_file_name, m_text_pos);
		if ('/' == m_current) {
			next_char();
			scan_single_line_comment();
			return true;
		} else if ('*' == m_current) {
			next_char();
			scan_multiline_comment();
			return true;
		} else {
			throw ParserException("Bad token", m_file_name, m_text_pos);
		}
	} else {
		return false;
	}
}

void prs::Scanner::scan_single_line_comment() {
	while (!m_eof && '\n' != m_current) next_char();
	if (!m_eof) {
		//Skip '\n'
		next_char();
	}
}

void prs::Scanner::scan_multiline_comment() {
	while (!m_eof) {
		if ('*' == m_current) {
			next_char();
			if (!m_eof && '/' == m_current) break;
		} else {
			next_char();
		}
	}
	if (m_eof) throw ParserException("Unexpected end of file", m_file_name, m_text_pos);
	
	//Skip '/'.
	next_char();
}

void prs::Scanner::scan_number(TokenRecord* token_record) {
	TextPos start_text_pos = m_text_pos;

	token_number v = char_to_digit(m_current);
	next_char();
	while (!m_eof && is_digit(m_current)) {
		token_number d = char_to_digit(m_current);
		token_number v_mul_10 = v * 10;
		if (v <= g_max_number_div_10 && d <= g_max_number - v_mul_10) {
			v = v_mul_10 + d;
		} else {
			throw ParserException("Decimal value out of range", m_file_name, start_text_pos);
		}
		next_char();
	}

	token_record->token = Tokens::NUMBER;
	token_record->v_number = v;
}

void prs::Scanner::scan_name(TokenRecord* token_record) {
	scan_name_buffer();
	token_record->token = Tokens::NAME;
	FilePos file_pos(m_file_name, token_record->pos);
	token_record->v_string = syntax_string(file_pos, String(m_string_buffer));
	m_string_buffer.clear();
}

void prs::Scanner::scan_keyword(TokenRecord* token_record) {
	next_char();
	if (!is_name_start(m_current)) throw ParserException("Name expected", m_file_name, m_text_pos);

	scan_name_buffer();

	if (g_keyword_token == m_string_buffer) {
		token_record->token = Tokens::KW_TOKEN;
	} else if (g_keyword_this == m_string_buffer) {
		token_record->token = Tokens::KW_THIS;
	} else if (g_keyword_false == m_string_buffer) {
		token_record->token = Tokens::KW_FALSE;
	} else if (g_keyword_true == m_string_buffer) {
		token_record->token = Tokens::KW_TRUE;
	} else if (g_keyword_type == m_string_buffer) {
		token_record->token = Tokens::KW_TYPE;
	} else if (g_keyword_class == m_string_buffer) {
		token_record->token = Tokens::KW_CLASS;
	} else {
		token_record->token = Tokens::NAME;
		FilePos file_pos(m_file_name, token_record->pos);
		token_record->v_string = syntax_string(file_pos, String(m_string_buffer));
	}

	m_string_buffer.clear();
}

void prs::Scanner::scan_name_buffer() {
	m_string_buffer.clear();
	m_string_buffer.push_back(m_current);
	next_char();
	while (!m_eof && is_name_part(m_current)) {
		m_string_buffer.push_back(m_current);
		next_char();
	}
}

void prs::Scanner::scan_string_literal(TokenRecord* token_record) {
	next_char();
	m_string_buffer.clear();
	while (!m_eof && '"' != m_current) {
		m_string_buffer.push_back(m_current);
		next_char();
	}

	if (m_eof) throw ParserException("End-of-file in the middle of a string literal", m_file_name, m_text_pos);

	next_char();
	token_record->token = Tokens::STRING;
	FilePos file_pos(m_file_name, token_record->pos);
	token_record->v_string = syntax_string(file_pos, String(m_string_buffer));
	m_string_buffer.clear();
}

void prs::Scanner::scan_key_character(TokenRecord* token_record) {
	char c = m_current;
	Tokens::E token = Tokens::END_OF_FILE;
	switch (c) {
	case ';':
		token = Tokens::CH_SEMICOLON;
		break;
	case '@':
		token = Tokens::CH_AT;
		break;
	case '{':
		token = Tokens::CH_OBRACE;
		break;
	case '}':
		token = Tokens::CH_CBRACE;
		break;
	case '|':
		token = Tokens::CH_OR;
		break;
	case '=':
		token = Tokens::CH_EQ;
		break;
	case '(':
		token = Tokens::CH_OPAREN;
		break;
	case ')':
		token = Tokens::CH_CPAREN;
		break;
	case '?':
		token = Tokens::CH_QUESTION;
		break;
	case '*':
		token = Tokens::CH_ASTERISK;
		break;
	case '+':
		token = Tokens::CH_PLUS;
		break;
	case '<':
		token = Tokens::CH_LT;
		break;
	case '>':
		token = Tokens::CH_GT;
		break;
	case ',':
		token = Tokens::CH_COMMA;
		break;
	case '.':
		token = Tokens::CH_DOT;
		break;
	case ':':
		next_char();
		if (':' == m_current) {
			token_record->token = Tokens::CH_COLON_COLON;
			next_char();
		} else {
			token_record->token = Tokens::CH_COLON;
		}
		//next_char() has already been called, so we have to return instead of break.
		return;
	case '-':
		next_char();
		if ('>' == m_current) {
			token_record->token = Tokens::CH_MINUS_GT;
			next_char();
			return;
		}
		//'-' is not a valid token, so we leave the current token END_OF_FILE, what will cause an error after 'break'.
		break;
	}

	if (Tokens::END_OF_FILE == token) {
		//The position m_line:m_column may point not to the first character of the token.
		throw ParserException("Bad token", m_file_name, m_text_pos);
	}

	token_record->token = token;
	next_char();
}

void prs::Scanner::next_char() {
	m_in.get(m_current);
	if (!m_in) {
		if (m_in.eof()) {
			m_current = 0;
			m_eof = true;
		} else {
			throw ParserException("File read error", m_file_name);
		}
	} else {
		m_text_pos = m_next_text_pos;
		if ('\n' == m_current) {
			++m_next_text_pos.m_line;
			m_next_text_pos.m_column = 0;
		} else {
			++m_next_text_pos.m_column;
		}
	}
}
