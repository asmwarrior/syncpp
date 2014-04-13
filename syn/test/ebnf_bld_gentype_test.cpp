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

//Unit tests for the EBNF Grammar Builder related to general types calculations.

#include <istream>
#include <memory>
#include <sstream>
#include <string>

#include "core/ebnf_builder.h"
#include "core/grm_parser.h"
#include "core/grm_parser_impl.h"

#include "ebnf_builder_test.h"

#include "unittest.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace prs = ns::grm_parser;

namespace {

	typedef ebnf::NonterminalDeclaration NtDecl;

	class Helper : public syn_test::AbstractHelper {
	public:
		Helper(const char* grammar_text)
			: AbstractHelper(grammar_text, &ns::EBNF_BuilderTestGate::calculate_general_types)
		{}
	};

	void test_ok(const char* grammar_text) {
		Helper helper(grammar_text);
		helper.test_ok();
	}

	void test_fail(const char* grammar_text, const char* error_msg) {
		Helper helper(grammar_text);
		helper.test_fail(error_msg);
	}

	void test_value(const char* grammar_text, ns::GeneralType expected_value) {
		Helper helper(grammar_text);
		helper.test_ok();

		assertTrue(!!helper.get_grammar()->get_nonterminals().size());
		const NtDecl* nt = helper.get_grammar()->get_nonterminals()[0];
		assertEquals(expected_value, nt->get_extension()->get_general_type());
	}

}

namespace {//anonymous

TEST(calc__or_void_void) {
	test_value("A : | ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__or_void_recursion) {
	test_value("A : | A ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__or_void_primitive) {
	test_value("%token ID{str}; A : | ID ;", ns::GENERAL_TYPE_PRIMITIVE);
}

TEST(calc__or_void_array) {
	test_value("%token ID{str}; A : | ID+ ;", ns::GENERAL_TYPE_ARRAY);
}

TEST(calc__or_void_class) {
	test_value("%token ID{str}; A : | a=ID ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__or_recursion_recursion) {
	test_value("A : A | A ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__or_recursion_primitive) {
	test_value("%token ID{str}; A : A | ID ;", ns::GENERAL_TYPE_PRIMITIVE);
}

TEST(calc__or_recursion_array) {
	test_value("%token ID{str}; A : A | ID+ ;", ns::GENERAL_TYPE_ARRAY);
}

TEST(calc__or_recursion_class) {
	test_value("%token ID{str}; A : A | a=ID ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__or_primitive_primitive) {
	test_value("%token ID{str}; A : ID | ID ;", ns::GENERAL_TYPE_PRIMITIVE);
}

TEST(calc__or_array_array) {
	test_value("%token ID{str}; A : ID+ | ID+ ;", ns::GENERAL_TYPE_ARRAY);
}

TEST(calc__or_class_class) {
	test_value("%token ID{str}; A : a=ID | a=ID ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__empty_expr) {
	test_value("X : ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__and_empty_expr) {
	test_value("X : () () () ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__and_attrs_expr) {
	test_value("%token ID{str}; X : a=ID b=ID c=ID ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__and_class_expr) {
	test_value("%token NOTHING; X : NOTHING {Z} ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__and_empty_result_expr) {
	test_value("%token NOTHING; X : NOTHING %this=() ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__and_nonempty_result_expr) {
	test_value("%token NOTHING; %token ID{str}; X : NOTHING %this=ID ;", ns::GENERAL_TYPE_PRIMITIVE);
}

TEST(calc__this_void_expr) {
	test_value("X : %this=() ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__this_nonvoid_expr) {
	test_value("X : %this=({Z}) ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__attr_expr) {
	test_value("X : a=({Z}) ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__cast_expr) {
	test_value("X : {Z}({Q}) ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__zero_one_void_expr) {
	test_value("X : ()? ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__zero_one_nonvoid_expr) {
	test_value("X : ({Z})? ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__zero_many_void_expr) {
	test_value("X : ()* ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__zero_many_nonvoid_expr) {
	test_value("X : ({Z})* ;", ns::GENERAL_TYPE_ARRAY);
}

TEST(calc__string_expr) {
	test_value("X : \"aaa\" ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__name_void_token_expr) {
	test_value("%token NOTHING; X : NOTHING ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__name_nonvoid_token_expr) {
	test_value("%token ID{str}; X : ID ;", ns::GENERAL_TYPE_PRIMITIVE);
}

TEST(calc__name_void_nt_expr) {
	test_value("X : Y ; Y : ;", ns::GENERAL_TYPE_VOID);
}

TEST(calc__name_nonvoid_nt_expr) {
	test_value("X : Y ; Y : {Z} ;", ns::GENERAL_TYPE_CLASS);
}

TEST(calc__const_expr) {
	test_value("X : <123> ;", ns::GENERAL_TYPE_PRIMITIVE);
}

TEST(error__or_primitive_array) {
	test_fail("%token ID{str}; A : ID | ID+ ;", "Incompatible types of alternative rules: primitive and array");
}

TEST(error__or_primitive_class) {
	test_fail("%token ID{str}; A : ID | a=ID ;", "Incompatible types of alternative rules: primitive and class");
}

TEST(error__or_array_class) {
	test_fail("%token ID{str}; A : ID+ | a=ID ;", "Incompatible types of alternative rules: array and class");
}

TEST(ok__cast_type_primitive) {
	test_ok("%token ID{str}; A : {str}(ID) ;");
}

TEST(ok__nt_explicit_type_primitive) {
	test_ok("%token ID{str}; A{str} : ID ;");
}

TEST(error__cast_type_primitive_nt) {
	test_fail("%token ID{str}; A : {K}(ID) ; K : ID ;", "Cannot use a non-class nonterminal 'K' as an explicit type");
}

TEST(error__cast_type_array_nt) {
	test_fail("%token ID{str}; A : {K}(ID+) ; K : ID+ ;", "Cannot use a non-class nonterminal 'K' as an explicit type");
}

TEST(error__nt_explicit_type_primitive_nt) {
	test_fail("%token ID{str}; A{K} : ID ; K : ID ;", "Cannot use a non-class nonterminal 'K' as an explicit type");
}

TEST(error__nt_explicit_type_array_nt) {
	test_fail("%token ID{str}; A{K} : ID+ ; K : ID+ ;", "Cannot use a non-class nonterminal 'K' as an explicit type");
}

TEST(error__cast_primitive_to_class_nt) {
	test_fail("%token ID {str}; X : a=ID b=ID ; Y : {X}(ID) ;", "Cannot cast incompatible types: primitive to class");
}

TEST(error__cast_array_to_class_nt) {
	test_fail("%token ID {str}; X : a=ID b=ID ; Y : {X}(ID+) ;", "Cannot cast incompatible types: array to class");
}

TEST(ok__cast_class_to_class_nt) {
	test_ok("%token ID {str}; X : a=ID b=ID ; Y : {X}(x=ID) ;");
}

TEST(error__cast_to_primitive_nt) {
	test_fail("%token ID {str}; X : ID ; Y : {X}(ID) ;", "Cannot use a non-class nonterminal 'X' as an explicit type");
}

TEST(error__cast_to_array_nt) {
	test_fail("%token ID {str}; X : ID+ ; Y : {X}(ID+) ;", "Cannot use a non-class nonterminal 'X' as an explicit type");
}

TEST(ok__cast_primitive_to_primitive) {
	test_ok("%token ID {str}; X : {str}(ID) ;");
}

TEST(error__cast_array_to_primitive) {
	test_fail("%token ID {str}; X : {str}(ID+) ;", "Cannot cast incompatible types: array to primitive");
}

TEST(error__cast_class_to_primitive) {
	test_fail("%token ID {str}; X : {str}(a=ID b=ID c=ID) ;", "Cannot cast incompatible types: class to primitive");
}

TEST(error__nt_cast_primitive_to_class_nt) {
	test_fail("%token ID {str}; X : a=ID b=ID ; Y{X} : (ID) ;", "Cannot cast incompatible types: primitive to class");
}

TEST(error__nt_cast_array_to_class_nt) {
	test_fail("%token ID {str}; X : a=ID b=ID ; Y{X} : (ID+) ;", "Cannot cast incompatible types: array to class");
}

TEST(ok__nt_cast_class_to_class_nt) {
	test_ok("%token ID {str}; X : a=ID b=ID ; Y{X} : (x=ID) ;");
}

TEST(error__nt_cast_to_primitive_nt) {
	test_fail("%token ID {str}; X : ID ; Y{X} : (ID) ;", "Cannot use a non-class nonterminal 'X' as an explicit type");
}

TEST(error__nt_cast_to_array_nt) {
	test_fail("%token ID {str}; X : ID+ ; Y{X} : (ID+) ;", "Cannot use a non-class nonterminal 'X' as an explicit type");
}

TEST(ok__nt_cast_primitive_to_primitive) {
	test_ok("%token ID {str}; X{str} : (ID) ;");
}

TEST(error__nt_cast_array_to_primitive) {
	test_fail("%token ID {str}; X{str} : (ID+) ;", "Cannot cast incompatible types: array to primitive");
}

TEST(error__nt_cast_class_to_primitive) {
	test_fail("%token ID {str}; X{str} : (a=ID b=ID c=ID) ;", "Cannot cast incompatible types: class to primitive");
}

TEST(error__production_type_is_array) {
	test_fail("%token ID {str}; X : a=ID {Z} ; Z : ID+ ;", "Cannot use a non-class nonterminal 'Z' as an explicit type");
}

TEST(error__cast_native_const_to_class) {
	test_fail("%token ID {str}; X : {Z}(<native_const>) ;", "Cannot cast incompatible types: primitive to class");
}

TEST(cast_native_const_to_primitive) {
	test_ok("%token ID {str}; X : {str}(<native_const>) ;");
}

TEST(error__cast_native_const_to_nt) {
	test_fail("%token ID {str}; X : {Z}(<native_const>) ; Z : a=ID ;", "Cannot cast incompatible types: primitive to class");
}

}
