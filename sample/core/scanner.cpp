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

//Scanner implementation.

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include "common.h"
#include "syn.h"
#include "syngen.h"
#include "gc.h"
#include "name.h"
#include "scanner.h"

namespace ss = syn_script;
namespace gc = ss::gc;

using ss::syngen::Token;
using ss::syngen::Tokens;
using ss::syngen::TokenValue;

namespace {

	struct RowCol {
		int m_row;
		int m_col;
	};

	typedef ss::StringIterator str_iter;

	ss::StringIterator str_begin(const ss::StringLoc& str) {
		return ss::StringIterator::begin(str.get());
	}

	ss::StringIterator str_end(const ss::StringLoc& str) {
		return ss::StringIterator::end(str.get());
	}

	//TODO Use tables to check character type.

	bool is_dec_digit(char c) {
		return c >= '0' && c <= '9';
	}

	bool is_hex_digit(char c) {
		return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
	}

	bool is_identifier_start(char c) {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
	}

	bool is_identifier_part(char c) {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$';
	}

	bool is_whitespace(char c) {
		return c == '\t' || c == '\r' || c == '\n' || c == ' ';
	}

	bool is_valid_string_char(char c0) {
		unsigned char c = c0;
		return c != '\n' && c != '\r' && c >= 0x20 && c < 0x80 && c != '\\';
	}
}

//
// InternalScanner : definition
//

class ss::InternalScanner {
	NONCOPYABLE(InternalScanner);

	std::list<StdStringKey> m_keyword_keys;
	std::map<StringKeyValue, Token> m_keyword_map;

	NameRegistry& m_name_registry;
	const StringLoc m_file_name;
	const StringLoc m_str;
	const str_iter m_end;
	str_iter m_cur;
	char m_curch;
	bool m_eof;

	RowCol m_pos;

	str_iter m_start;
	RowCol m_start_pos;

	std::string m_buffer;
	std::set<StringKeyValue> m_string_literal_table;
	std::list<GCStringKey> m_string_literal_keys;

public:
	explicit InternalScanner(
		NameRegistry& name_registry,
		const StringLoc& file_name,
		const StringLoc &str);

	gc::Local<TextPos> get_text_pos() const;
	syngen::Token scan(syngen::TokenValue& token_value);

private:
	syngen::Token scan_0(syngen::TokenValue& token_value);

	void init_keyword_map();

	void scan_blank();
	void scan_single_line_comment();
	void scan_multiline_comment();
	syngen::Token scan_number(syngen::TokenValue& token_value);
	syngen::Token scan_number_dec(syngen::TokenValue& token_value);
	syngen::Token scan_number_hex(syngen::TokenValue& token_value);
	syngen::Token scan_name(syngen::TokenValue& token_value);
	syngen::Token scan_string(syngen::TokenValue& token_value);
	syngen::Token scan_char(syngen::TokenValue& token_value);
	void scan_string_char();
	char lookup_char() const;
	void nextch();
	void update_pos(char c);
	void update_curch();
	void copy_to_buffer(const str_iter& start, const str_iter& end);
	gc::Local<TextPos> text_pos(const RowCol& row_col) const;
};

//
// InternalScanner : implementation
//

ss::InternalScanner::InternalScanner(
	NameRegistry& name_registry,
	const StringLoc& file_name,
	const StringLoc& str)
	: m_name_registry(name_registry),
	m_file_name(file_name),
	m_str(str),
	m_end(str_end(str)),
	m_cur(str_begin(str))
{
	init_keyword_map();

	m_pos.m_row = 0;
	m_pos.m_col = 0;

	m_eof = false;
	update_curch();
}

void ss::InternalScanner::init_keyword_map() {
	for (const syngen::Keyword* kw = syngen::g_keyword_table; !kw->keyword.empty(); ++kw) {
		const std::string* str = &kw->keyword;
		m_keyword_keys.push_back(StdStringKey(*str));
		m_keyword_map[StringKeyValue(m_keyword_keys.back())] = kw->token;
	}
}

gc::Local<ss::TextPos> ss::InternalScanner::get_text_pos() const {
	return text_pos(m_pos);
}

ss::syngen::Token ss::InternalScanner::scan(TokenValue& token_value) {
	try {
		return scan_0(token_value);
	} catch (syn::SynLexicalError&) {
		throw CompilationError(get_text_pos(), "Lexical error");
	}
}

ss::syngen::Token ss::InternalScanner::scan_0(TokenValue& token_value) {
	scan_blank();
	if (m_eof) return Tokens::SYS_EOF;

	m_start = m_cur;
	m_start_pos = m_pos;

	Token token;
	char c = m_curch;
	if (is_dec_digit(c)) {
		token = scan_number(token_value);
	} else if (is_identifier_start(c)) {
		token = scan_name(token_value);
	} else if (c == '"') {
		token = scan_string(token_value);
	} else if (c == '\'') {
		token = scan_char(token_value);
	} else {
		token = syngen::scan_concrete_token_basic(&m_cur, m_end);
		for (str_iter i = m_start; i != m_cur; ++i) update_pos(*i);
		update_curch();
		token_value.v_SynPos = gc::create<ss::TextPos>(m_file_name, m_start_pos.m_row, m_start_pos.m_col);
	}

	//std::string token_str;
	//m_start.get_std_string(m_cur, token_str);
	//std::cout << (m_start_pos.m_row + 1) << ":" << (m_start_pos.m_col + 1) << " "
	//	<< syngen::g_token_descriptors[token].name
	//	<< " '" << token_str << "'\n";

	return token;
}

void ss::InternalScanner::scan_blank() {
	while (!m_eof) {
		char c = m_curch;
		if (c == '/') {
			char nextc = lookup_char();
			if (nextc == '/') {
				nextch();
				nextch();
				scan_single_line_comment();
			} else if (nextc == '*') {
				nextch();
				nextch();
				scan_multiline_comment();
			} else {
				break;
			}
		} else if (is_whitespace(m_curch)) {
			nextch();
		} else {
			break;
		}
	}
}

void ss::InternalScanner::scan_single_line_comment() {
	while (!m_eof && m_curch != '\n') nextch();
}

void ss::InternalScanner::scan_multiline_comment() {
	while (!m_eof) {
		char c = m_curch;
		nextch();
		if (c == '*' && !m_eof && m_curch == '/') {
			nextch();
			break;
		}
	}

	if (m_eof) throw syn::SynLexicalError();
}

ss::syngen::Token ss::InternalScanner::scan_number(TokenValue& token_value) {
	syngen::Token token;
	if (m_curch == '0') {
		nextch();
		if (m_curch == 'x' || m_curch == 'X') {
			nextch();
			token = scan_number_hex(token_value);
		} else {
			token = scan_number_dec(token_value);
		}
	} else {
		token = scan_number_dec(token_value);
	}
	return token;
}

ss::syngen::Token ss::InternalScanner::scan_number_dec(TokenValue& token_value) {
	bool floating_point = false;

	while (!m_eof && is_dec_digit(m_curch)) nextch();
	if (m_curch == '.') {
		nextch();
		while (!m_eof && is_dec_digit(m_curch)) nextch();
		floating_point = true;
	}
	if (m_curch == 'E' || m_curch == 'e') {
		nextch();
		if (m_curch == '+' || m_curch == '-') nextch();
		if (!is_dec_digit(m_curch)) throw syn::SynLexicalError();
		while (!m_eof && is_dec_digit(m_curch)) nextch();
		floating_point = true;
	}

	copy_to_buffer(m_start, m_cur);
	gc::Local<ss::TextPos> text_pos = gc::create<ss::TextPos>(m_file_name, m_start_pos.m_row, m_start_pos.m_col);

	syngen::Token token;
	if (floating_point) {
		token = Tokens::T_FLOAT;
		ScriptFloatType v = std::stod(m_buffer);
		token_value.v_SynFloat = gc::create<ast::AstFloat>(text_pos, v);
	} else {
		token = Tokens::T_INTEGER;
		ScriptIntegerType v = std::stol(m_buffer);
		token_value.v_SynInteger = gc::create<ast::AstInteger>(text_pos, v);
	}

	return token;
}

ss::syngen::Token ss::InternalScanner::scan_number_hex(TokenValue& token_value) {
	str_iter start = m_cur;
	if (!is_hex_digit(m_curch)) throw syn::SynLexicalError();

	nextch();
	while (!m_eof && is_hex_digit(m_curch)) nextch();

	copy_to_buffer(start, m_cur);
	gc::Local<ss::TextPos> text_pos = gc::create<ss::TextPos>(m_file_name, m_start_pos.m_row, m_start_pos.m_col);
	ScriptIntegerType v = std::stol(m_buffer, nullptr, 16);
	token_value.v_SynInteger = gc::create<ast::AstInteger>(text_pos, v);

	return syngen::Token::T_INTEGER;
}

ss::syngen::Token ss::InternalScanner::scan_name(TokenValue& token_value) {
	nextch();
	while (!m_eof && is_identifier_part(m_curch)) nextch();

	gc::Local<ss::TextPos> text_pos = gc::create<ss::TextPos>(m_file_name, m_start_pos.m_row, m_start_pos.m_col);
	syngen::Token token = Tokens::T_ID;

	GCPtrStringKey str_key = m_start.get_string_key(m_cur);
	auto iter = m_keyword_map.find(StringKeyValue(str_key));
	if (iter != m_keyword_map.end()) {
		token = iter->second;
		token_value.v_SynPos = text_pos;
	}

	if (token == Tokens::T_ID) {
		gc::Local<const NameInfo> name_info = m_name_registry.register_name(m_start, m_cur);
		token_value.v_SynName = gc::create<ast::AstName>(text_pos, name_info);
	}

	return token;
}

ss::syngen::Token ss::InternalScanner::scan_string(TokenValue& token_value) {
	nextch();

	m_buffer.clear();
	while (!m_eof) {
		if ('"' == m_curch) {
			nextch();
			break;
		}
		scan_string_char();
	}

	StringLoc v;

	StdStringKey key(m_buffer);
	auto iter = m_string_literal_table.find(StringKeyValue(key));
	if (iter == m_string_literal_table.end()) {
		v = gc::create<String>(m_buffer);
		m_string_literal_keys.push_back(GCStringKey(v));
		const GCStringKey& gc_key = m_string_literal_keys.back();
		m_string_literal_table.insert(StringKeyValue(gc_key));
	} else {
		v = iter->get_key().get_gc_string();
	}

	gc::Local<ss::TextPos> text_pos = gc::create<ss::TextPos>(m_file_name, m_start_pos.m_row, m_start_pos.m_col);
	token_value.v_SynString = gc::create<ast::AstString>(text_pos, v);
	return Tokens::T_STRING;
}

ss::syngen::Token ss::InternalScanner::scan_char(TokenValue& token_value) {
	nextch();
	if (m_eof || '\'' == m_curch) throw syn::SynLexicalError();

	m_buffer.clear();
	scan_string_char();

	if (m_eof || '\'' != m_curch) throw syn::SynLexicalError();
	nextch();

	ScriptIntegerType v = m_buffer[0];
	gc::Local<ss::TextPos> text_pos = gc::create<ss::TextPos>(m_file_name, m_start_pos.m_row, m_start_pos.m_col);
	token_value.v_SynInteger = gc::create<ast::AstInteger>(text_pos, v);
	return Tokens::T_INTEGER;
}

void ss::InternalScanner::scan_string_char() {
	char c = m_curch;
	nextch();

	if ('\\' == c) {
		c = m_curch;
		nextch();
		char appc;
		if ('r' == c) {
			appc = '\r';
		} else if ('n' == c) {
			appc = '\n';
		} else if ('t' == c) {
			appc = '\t';
		} else if ('"' == c || '\'' == c || '\\' == c) {
			appc = c;
		} else {
			//TODO Support other escape sequences.
			throw syn::SynLexicalError();
		}
		m_buffer += appc;
	} else if (is_valid_string_char(c)) {
		m_buffer += c;
	} else {
		throw syn::SynLexicalError();
	}
}

char ss::InternalScanner::lookup_char() const {
	str_iter next = m_cur;
	++next;
	return next == m_end ? 0 : *next;
}

void ss::InternalScanner::nextch() {
	if (!m_eof) {
		update_pos(m_curch);
		++m_cur;
		update_curch();
	}
}

void ss::InternalScanner::update_pos(char c) {
	if (c == '\n') {
		++m_pos.m_row;
		m_pos.m_col = 0;
	} else {
		++m_pos.m_col;
	}
}

void ss::InternalScanner::update_curch() {
	if (m_cur == m_end) {
		m_eof = true;
		m_curch = 0;
	} else {
		m_curch = *m_cur;
	}
}

void ss::InternalScanner::copy_to_buffer(const str_iter& start, const str_iter& end) {
	start.get_std_string(end, m_buffer);
}

gc::Local<ss::TextPos> ss::InternalScanner::text_pos(const RowCol& row_col) const {
	return gc::create<TextPos>(m_file_name, row_col.m_row, row_col.m_col);
}

//
// Scanner
//

ss::Scanner::Scanner(NameRegistry& name_registry, const StringLoc& file_name, const StringLoc& text)
	: m_internal_scanner(new InternalScanner(name_registry, file_name, text))
{}

ss::Scanner::~Scanner(){}

ss::syngen::Token ss::Scanner::scan(TokenValue& token_value) {
	return m_internal_scanner->scan(token_value);
}

gc::Local<ss::TextPos> ss::Scanner::get_text_pos() const {
	return m_internal_scanner->get_text_pos();
}

