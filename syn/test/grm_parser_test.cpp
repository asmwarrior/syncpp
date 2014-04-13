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

//Unit tests for the Grammar Parser.

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "core/ebnf__imp.h"
#include "core/grm_parser.h"
#include "core/grm_parser_res.h"
#include "core/grm_parser_impl.h"
#include "core/util_mptr.h"
#include "core/util_string.h"

#include "unittest.h"

namespace ns = synbin;
namespace prs = ns::grm_parser;
namespace ebnf = ns::ebnf;
namespace util = ns::util;

namespace {

	using std::unique_ptr;

	using util::MPtr;
	using util::MRoot;

	typedef MRoot<ebnf::Grammar> GrammarRoot;
	typedef std::unique_ptr<GrammarRoot> GrammarRootAutoPtr;

	unique_ptr<ns::GrammarParsingResult> parse_grammar(const char* grammar) {
		std::istringstream in(grammar);
		util::String file_name("test");
		return prs::parse_grammar(in, file_name);
	}

	void test_fail(const char* grammar_text, const char* error_text) {
		try {
			parse_grammar(grammar_text);
			fail();
		} catch (prs::ParserException& e) {
			assertEquals(error_text, e.to_string());
		}
	}

}

namespace {//anonymous

TEST(empty_grammar) {
	test_fail("", "test(1:1): Syntax error");
}

TEST(bad_token) {
	test_fail("!@#$%^&", "test(1:1): Bad token");
}

TEST(syntax_error) {
	test_fail("%this %this %this", "test(1:1): Syntax error");
}

unique_ptr<ns::GrammarParsingResult> parse_test_grammar() {
	unique_ptr<ns::GrammarParsingResult> result = parse_grammar(
		"%type number;"
		"%token NUM {number};"
		"Expr : Expr \"+\" Term | Term ;"
		"Term : NUM | \"(\" Expr \")\" | \"-\" Term ;"
	);
	return result;
}

TEST(correct_declarations) {
	unique_ptr<ns::GrammarParsingResult> parsing_result = parse_test_grammar();
	MPtr<ebnf::Grammar> grm = parsing_result->get_grammar();
	assertNotNull(grm.get());
	
	const std::vector<MPtr<ebnf::Declaration>>& decls = grm->get_declarations();
	assertEquals(4, decls.size());
	
	assertNotNull(dynamic_cast<ebnf::TypeDeclaration*>(decls[0].get()));
	assertEquals("number", dynamic_cast<ebnf::TypeDeclaration*>(decls[0].get())->get_name().str());
	assertNotNull(dynamic_cast<ebnf::TerminalDeclaration*>(decls[1].get()));
	assertEquals("NUM", dynamic_cast<ebnf::TerminalDeclaration*>(decls[1].get())->get_name().str());
	assertNotNull(dynamic_cast<ebnf::NonterminalDeclaration*>(decls[2].get()));
	assertEquals("Expr", dynamic_cast<ebnf::NonterminalDeclaration*>(decls[2].get())->get_name().str());
	assertNotNull(dynamic_cast<ebnf::NonterminalDeclaration*>(decls[3].get()));
	assertEquals("Term", dynamic_cast<ebnf::NonterminalDeclaration*>(decls[3].get())->get_name().str());
}

TEST(correct_terminal) {
	unique_ptr<ns::GrammarParsingResult> parsing_result = parse_test_grammar();
	MPtr<ebnf::Grammar> grm = parsing_result->get_grammar();
	assertNotNull(grm.get());
	assertEquals(4, grm->get_declarations().size());
	
	ebnf::TerminalDeclaration* tr = dynamic_cast<ebnf::TerminalDeclaration*>(grm->get_declarations()[1].get());
	assertNotNull(tr);
	assertEquals("NUM", tr->get_name().str());
	assertNotNull(tr->get_raw_type());
	assertEquals("number", tr->get_raw_type()->get_name().str());
}

TEST(correct_nt_expr) {
	unique_ptr<ns::GrammarParsingResult> parsing_result = parse_test_grammar();
	MPtr<ebnf::Grammar> grm = parsing_result->get_grammar();
	assertNotNull(grm.get());
	assertEquals(4, grm->get_declarations().size());
	
	ebnf::NonterminalDeclaration* nt = dynamic_cast<ebnf::NonterminalDeclaration*>(grm->get_declarations()[2].get());
	assertNotNull(nt);
	assertEquals("Expr", nt->get_name().str());

	ebnf::SyntaxExpression* expr = nt->get_expression();
	assertNotNull(expr);

	ebnf::SyntaxOrExpression* or_expr = dynamic_cast<ebnf::SyntaxOrExpression*>(expr);
	assertNotNull(or_expr);

	assertEquals(2, or_expr->get_sub_expressions().size());

	{
	ebnf::SyntaxExpression* sub_expr = or_expr->get_sub_expressions()[0].get();
	ebnf::SyntaxAndExpression* and_expr = dynamic_cast<ebnf::SyntaxAndExpression*>(sub_expr);
	assertNotNull(and_expr);

	const std::vector<MPtr<ebnf::SyntaxExpression>>& elems = and_expr->get_sub_expressions();
	assertEquals(3, elems.size());

	ebnf::NameSyntaxExpression* name_expr_1 = dynamic_cast<ebnf::NameSyntaxExpression*>(elems[0].get());
	assertNotNull(name_expr_1);
	assertEquals("Expr", name_expr_1->get_name().str());

	ebnf::StringSyntaxExpression* str_expr = dynamic_cast<ebnf::StringSyntaxExpression*>(elems[1].get());
	assertNotNull(str_expr);
	assertEquals("+", str_expr->get_string().str());

	ebnf::NameSyntaxExpression* name_expr_2 = dynamic_cast<ebnf::NameSyntaxExpression*>(elems[2].get());
	assertNotNull(name_expr_2);
	assertEquals("Term", name_expr_2->get_name().str());
	}

	{
	ebnf::SyntaxExpression* sub_expr = or_expr->get_sub_expressions()[1].get();
	ebnf::NameSyntaxExpression* name_expr = dynamic_cast<ebnf::NameSyntaxExpression*>(sub_expr);
	assertNotNull(name_expr);
	assertEquals("Term", name_expr->get_name().str());
	}
}

TEST(correct_nt_term) {
	unique_ptr<ns::GrammarParsingResult> parsing_result = parse_test_grammar();
	MPtr<ebnf::Grammar> grm = parsing_result->get_grammar();
	assertNotNull(grm.get());
	assertEquals(4, grm->get_declarations().size());
	
	ebnf::NonterminalDeclaration* nt = dynamic_cast<ebnf::NonterminalDeclaration*>(grm->get_declarations()[3].get());
	assertNotNull(nt);
	assertEquals("Term", nt->get_name().str());

	ebnf::SyntaxExpression* expr = nt->get_expression();
	assertNotNull(expr);

	ebnf::SyntaxOrExpression* or_expr = dynamic_cast<ebnf::SyntaxOrExpression*>(expr);
	assertNotNull(or_expr);

	assertEquals(3, or_expr->get_sub_expressions().size());

	{
	ebnf::SyntaxExpression* sub_expr = or_expr->get_sub_expressions()[0].get();
	ebnf::NameSyntaxExpression* name_expr = dynamic_cast<ebnf::NameSyntaxExpression*>(sub_expr);
	assertNotNull(name_expr);
	assertEquals("NUM", name_expr->get_name().str());
	}

	{
	ebnf::SyntaxExpression* sub_expr = or_expr->get_sub_expressions()[1].get();
	ebnf::SyntaxAndExpression* and_expr = dynamic_cast<ebnf::SyntaxAndExpression*>(sub_expr);
	assertNotNull(and_expr);

	const std::vector<MPtr<ebnf::SyntaxExpression>>& elems = and_expr->get_sub_expressions();
	assertEquals(3, elems.size());

	ebnf::StringSyntaxExpression* str_expr_1 = dynamic_cast<ebnf::StringSyntaxExpression*>(elems[0].get());
	assertNotNull(str_expr_1);
	assertEquals("(", str_expr_1->get_string().str());

	ebnf::NameSyntaxExpression* name_expr = dynamic_cast<ebnf::NameSyntaxExpression*>(elems[1].get());
	assertNotNull(name_expr);
	assertEquals("Expr", name_expr->get_name().str());

	ebnf::StringSyntaxExpression* str_expr_2 = dynamic_cast<ebnf::StringSyntaxExpression*>(elems[2].get());
	assertNotNull(str_expr_2);
	assertEquals(")", str_expr_2->get_string().str());
	}

	{
	ebnf::SyntaxExpression* sub_expr = or_expr->get_sub_expressions()[2].get();
	ebnf::SyntaxAndExpression* and_expr = dynamic_cast<ebnf::SyntaxAndExpression*>(sub_expr);
	assertNotNull(and_expr);

	const std::vector<MPtr<ebnf::SyntaxExpression>>& elems = and_expr->get_sub_expressions();
	assertEquals(2, elems.size());

	ebnf::StringSyntaxExpression* str_expr = dynamic_cast<ebnf::StringSyntaxExpression*>(elems[0].get());
	assertNotNull(str_expr);
	assertEquals("-", str_expr->get_string().str());

	ebnf::NameSyntaxExpression* name_expr = dynamic_cast<ebnf::NameSyntaxExpression*>(elems[1].get());
	assertNotNull(name_expr);
	assertEquals("Term", name_expr->get_name().str());
	}
}

}
