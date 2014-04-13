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

//Unit tests for the EBNF Grammar Builder related to void nonterminals detection.

#include <istream>
#include <memory>
#include <sstream>
#include <string>

#include "core/ebnf_builder.h"
#include "core/grm_parser.h"
#include "core/grm_parser_impl.h"
#include "core/util_mptr.h"

#include "ebnf_builder_test.h"

#include "unittest.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace prs = ns::grm_parser;
namespace util = ns::util;

using util::MPtr;

namespace {

	typedef ebnf::NonterminalDeclaration NtDecl;

	class Helper : public syn_test::AbstractHelper {
	public:
		Helper(const char* grammar_text)
			: AbstractHelper(grammar_text, &ns::EBNF_BuilderTestGate::calculate_is_void)
		{}

		const ebnf::NonterminalDeclaration* get_nt(std::size_t nt_index, const char* nt_name) {
			const ebnf::Grammar* grammar = get_grammar();
			assertTrue(grammar->get_nonterminals().size() > nt_index);
			const ebnf::NonterminalDeclaration* nt = grammar->get_nonterminals()[nt_index];
			assertEquals(nt_name, nt->get_name().str());
			return nt;
		}
	};

	void test_ok(const char* grammar_text) {
		Helper helper(grammar_text);
		helper.test_ok();
	}

	void test_fail(const char* grammar_text, const char* error_msg) {
		Helper helper(grammar_text);
		helper.test_fail(error_msg);
	}

	void test_value(const char* grammar_text, bool expected_value) {
		Helper helper(grammar_text);
		helper.test_ok();

		assertTrue(!!helper.get_grammar()->get_nonterminals().size());
		const NtDecl* nt = helper.get_grammar()->get_nonterminals()[0];
		assertEquals(expected_value, nt->get_extension()->is_void());
	}

}

namespace {//anonymous

TEST(value__void_recursion) {
	Helper helper("%token NOTHING; X : Y ; Y : Z ; Z : X | NOTHING ;");
	helper.test_ok();

	const NtDecl* nt_x = helper.get_nt(0, "X");
	assertTrue(nt_x->get_extension()->is_void());
	const NtDecl* nt_y = helper.get_nt(1, "Y");
	assertTrue(nt_y->get_extension()->is_void());
	const NtDecl* nt_z = helper.get_nt(2, "Z");
	assertTrue(nt_z->get_extension()->is_void());
}

TEST(value__nonvoid_recursion) {
	Helper helper("%token ID{str}; X : Y ; Y : Z ; Z : X | ID ;");
	helper.test_ok();

	const NtDecl* nt_x = helper.get_nt(0, "X");
	assertFalse(nt_x->get_extension()->is_void());
	const NtDecl* nt_y = helper.get_nt(1, "Y");
	assertFalse(nt_y->get_extension()->is_void());
	const NtDecl* nt_z = helper.get_nt(2, "Z");
	assertFalse(nt_z->get_extension()->is_void());
}

TEST(value__void_loop_recursion) {
	Helper helper("%token NOTHING; X : Y ; Y : Z ; Z : X+ | NOTHING ;");
	helper.test_ok();

	const NtDecl* nt_x = helper.get_nt(0, "X");
	assertTrue(nt_x->get_extension()->is_void());
	const NtDecl* nt_y = helper.get_nt(1, "Y");
	assertTrue(nt_y->get_extension()->is_void());
	const NtDecl* nt_z = helper.get_nt(2, "Z");
	assertTrue(nt_z->get_extension()->is_void());
}

TEST(value__nonvoid_loop_recursion) {
	Helper helper("%token ID{str}; X : Y ; Y : Z ; Z : X+ | ID ;");
	helper.test_ok();

	const NtDecl* nt_x = helper.get_nt(0, "X");
	assertFalse(nt_x->get_extension()->is_void());
	const NtDecl* nt_y = helper.get_nt(1, "Y");
	assertFalse(nt_y->get_extension()->is_void());
	const NtDecl* nt_z = helper.get_nt(2, "Z");
	assertFalse(nt_z->get_extension()->is_void());
}

TEST(value__empty_expr) {
	test_value("X : ;", true);
}

TEST(value__or_empty_expr) {
	test_value("%token NOTHING; X : | NOTHING ;", true);
}

TEST(value__or_nonempty_expr) {
	test_value("%token ID{str}; X : | ID ;", false);
}

TEST(value__and_empty_expr) {
	test_value("X : () () () ;", true);
}

TEST(value__and_attrs_expr) {
	test_value("%token ID{str}; X : a=ID b=ID c=ID ;", false);
}

TEST(value__and_class_expr) {
	test_value("%token NOTHING; X : NOTHING {Z} ;", false);
}

TEST(value__and_empty_result_expr) {
	test_value("%token NOTHING; X : NOTHING %this=() ;", true);
}

TEST(value__and_nonempty_result_expr) {
	test_value("%token NOTHING; %token ID{str}; X : NOTHING %this=ID ;", false);
}

TEST(value__this_void_expr) {
	test_value("X : %this=() ;", true);
}

TEST(value__this_nonvoid_expr) {
	test_value("X : %this=({Z}) ;", false);
}

TEST(value__attr_expr) {
	test_value("X : a=({Z}) ;", false);
}

TEST(value__cast_expr) {
	test_value("X : {Z}({Q}) ;", false);
}

TEST(value__zero_one_void_expr) {
	test_value("X : ()? ;", true);
}

TEST(value__zero_one_nonvoid_expr) {
	test_value("X : ({Z})? ;", false);
}

TEST(value__zero_many_void_expr) {
	test_value("X : ()* ;", true);
}

TEST(value__zero_many_nonvoid_expr) {
	test_value("X : ({Z})* ;", false);
}

TEST(value__string_expr) {
	test_value("X : \"aaa\" ;", true);
}

TEST(value__name_void_token_expr) {
	test_value("%token NOTHING; X : NOTHING ;", true);
}

TEST(value__name_nonvoid_token_expr) {
	test_value("%token ID{str}; X : ID ;", false);
}

TEST(value__name_void_nt_expr) {
	test_value("X : Y ; Y : ;", true);
}

TEST(value__name_nonvoid_nt_expr) {
	test_value("X : Y ; Y : {Z} ;", false);
}

TEST(value__const_expr) {
	test_value("X : <123> ;", false);
}

TEST(ok__nt_as_nt_type) {
	test_ok("%token ID{str}; X : ID ; Y{X} : ID ;");
}

TEST(ok__nt_as_cast_type) {
	test_ok("%token ID{str}; X : ID ; Y : {X}(ID) ;");
}

TEST(ok__normal_attribute_value) {
	test_ok("%token ID{str}; X : ID ; Y : a=ID ;");
}

TEST(ok__nonvoid_nt_as_and_type) {
	test_ok("%token ID{str}; X : a=ID ; Y : a=ID {X} ;");
}

TEST(ok__cast_nonvoid_expr) {
	test_ok("%token ID{str}; Y : {Z}(ID) ;");
}

TEST(error__void_nt_as_nt_type) {
	test_fail("%token NOTHING; X : NOTHING ; Y{X} : NOTHING ;", "Cannot use a void nonterminal as an explicit type");
}

TEST(error__void_nt_as_cast_type) {
	test_fail("%token NOTHING; X : NOTHING ; Y : {X}(NOTHING) ;", "Cannot use a void nonterminal as an explicit type");
}

TEST(error__void_token_attribute_value) {
	test_fail("%token NOTHING; X : a=NOTHING ;", "Cannot assign a void expression");
}

TEST(error__void_nt_attribute_value) {
	test_fail("%token NOTHING; X : NOTHING ; Y : a=X ;", "Cannot assign a void expression");
}

TEST(error__void_nt_as_and_type) {
	test_fail("%token NOTHING; %token ID{str}; X : NOTHING ; Y : a=ID {X} ;",
		"Cannot use a void nonterminal as an explicit type");
}

TEST(error__cast_void_expression) {
	test_fail("%token NOTHING; X : {Z}(NOTHING) ;", "Cannot cast a void expression");
}

TEST(error__cast_to_void_nt) {
	test_fail("%token ID {str}; X : ID ID ID ; Y : {X}(ID) ;", "Cannot use a void nonterminal as an explicit type");
}

TEST(error__nt_cast_type_recursion_expr_recursion_indirect) {
	test_fail("%token VOID; X : Y ; Y : Z ; Z{X} : VOID | \"(\" %this=X \")\" ;",
		"Cannot use a void nonterminal as an explicit type");
}

TEST(error__nt_cast_type_recursion_expr_recursion) {
	test_fail("%token VOID; X{X} : VOID | \"(\" %this=X \")\" ;", "Cannot use a void nonterminal as an explicit type");
}

TEST(error__nt_cast_void_to_primitive) {
	test_fail("%token ID {str}; X{str} : (ID ID ID) ;", "Cannot cast a void expression");
}

TEST(error__nt_cast_to_void_nt) {
	test_fail("%token ID {str}; X : ID ID ID ; Y{X} : (ID) ;", "Cannot use a void nonterminal as an explicit type");
}

TEST(error__cast_void_to_primitive) {
	test_fail("%token ID {str}; X : {str}(ID ID ID) ;", "Cannot cast a void expression");
}

}
