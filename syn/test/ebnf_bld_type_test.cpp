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

//Unit tests for the EBNF Grammar Builder related to types calculation.

#include <istream>
#include <memory>
#include <sstream>
#include <string>

#include "core/ebnf_builder.h"
#include "core/ebnf_bld_common.h"
#include "core/grm_parser.h"
#include "core/grm_parser_impl.h"
#include "core/types.h"
#include "core/util_mptr.h"

#include "ebnf_builder_test.h"

#include "unittest.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace prs = ns::grm_parser;
namespace types = ns::types;
namespace util = ns::util;

using util::MPtr;

namespace {

	typedef ebnf::NonterminalDeclaration NtDecl;

	class Helper : public syn_test::AbstractHelper {
	public:
		Helper(const char* grammar_text)
			: AbstractHelper(grammar_text, &ns::EBNF_BuilderTestGate::calculate_types)
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

	void assert_types_equal(const char* expected_type, MPtr<const types::Type> actual_type) {
		if (actual_type.get()) {
			std::ostringstream out;
			actual_type->print(out);
			assertEquals(expected_type, out.str());
		} else {
			assertEquals(expected_type, 0);
		}
	}

	void test_value(const char* grammar_text, const char* expected_nt_type, const char* expected_expr_type) {
		Helper helper(grammar_text);
		helper.test_ok();

		assertTrue(!!helper.get_grammar()->get_nonterminals().size());
		const NtDecl* nt = helper.get_grammar()->get_nonterminals()[0];

		MPtr<const types::Type> nt_type = nt->get_extension()->get_concrete_type();
		assert_types_equal(expected_nt_type, nt_type);
		MPtr<const types::Type> expr_type = nt->get_expression()->get_extension()->get_concrete_type();
		assert_types_equal(expected_expr_type, expr_type);
	}

	void dump_type(const types::Type* type, std::ostringstream& out_str) {
		if (type) {
			type->print(out_str);
		} else {
			out_str << "0";
		}
	}

	void dump_expr_type(ebnf::SyntaxExpression* expr, std::ostringstream& out_str) {

		struct DumpVisitor : public ns::SyntaxExpressionVisitor<void> {
			std::ostringstream& m_out_str;
			bool m_visited;

			DumpVisitor(std::ostringstream& out_str) : m_out_str(out_str), m_visited(false){}

			void visit_SyntaxExpression(ebnf::SyntaxExpression* expr) {
				if (!m_visited) {
					m_visited = true;
					m_out_str << "(";
				} else {
					m_out_str << ",";
				}
				dump_expr_type(expr, m_out_str);
			}
		};

		if (expr->get_extension()->is_concrete_type_set()) {
			dump_type(expr->get_extension()->get_concrete_type().get(), out_str);
		} else {
			out_str << "?";
		}

		DumpVisitor visitor(out_str);
		ns::SubSyntaxExpressionVisitor sub_visitor(&visitor);
		expr->visit(&sub_visitor);
		if (visitor.m_visited) {
			out_str << ")";
		}
	}

	void dump_nt_type(NtDecl* nt, std::ostringstream& out_str) {
		dump_type(nt->get_extension()->get_concrete_type().get(), out_str);
		out_str << "(";
		dump_expr_type(nt->get_expression(), out_str);
		out_str << ")";
	}

	void test_tree(const char* grammar_text, const char* exp_type_dump) {
		Helper helper(grammar_text);
		helper.test_ok();

		assertTrue(!!helper.get_grammar()->get_nonterminals().size());
		NtDecl* nt = helper.get_grammar()->get_nonterminals()[0];

		std::ostringstream act_type_dump_stream;
		dump_nt_type(nt, act_type_dump_stream);

		std::string act_type_dump = act_type_dump_stream.str();
		assertEquals(exp_type_dump, act_type_dump);
	}

}

namespace {//anonymous

TEST(calc__or_void_void) {
	test_value("A : | ;", "void", "void");
}

TEST(calc__or_void_recursion) {
	test_value("A : | A ;", "void", "void");
}

TEST(calc__or_void_primitive) {
	test_value("%token ID{str}; A : | ID ;", "user:str", "user:str");
}

TEST(calc__or_void_array) {
	test_value("%token ID{str}; A : | ID+ ;", "array[user:str]", "array[user:str]");
}

TEST(calc__or_void_class) {
	test_value("%token ID{str}; A : | a=ID ;", "nt:A", "nt:A");
}

TEST(calc__or_recursion_recursion) {
	test_value("A : A | A ;", "void", "void");
}

TEST(calc__or_recursion_primitive) {
	test_value("%token ID{str}; A : A | ID ;", "user:str", "user:str");
}

TEST(calc__or_recursion_array) {
	test_value("%token ID{str}; A : A | ID+ ;", "array[user:str]", "array[user:str]");
}

TEST(calc__or_recursion_class) {
	test_value("%token ID{str}; A : A | a=ID ;", "nt:A", "nt:A");
}

TEST(calc__or_primitive_primitive) {
	test_value("%token ID{str}; A : ID | ID ;", "user:str", "user:str");
}

TEST(calc__or_array_array) {
	test_value("%token ID{str}; A : ID+ | ID+ ;", "array[user:str]", "array[user:str]");
}

TEST(calc__or_class_class) {
	test_value("%token ID{str}; A : a=ID | a=ID ;", "nt:A", "nt:A");
}

TEST(calc__empty_expr) {
	test_value("X : ;", "void", "void");
}

TEST(calc__and_empty_expr) {
	test_value("X : () () () ;", "void", "void");
}

TEST(calc__and_attrs_expr) {
	test_value("%token ID{str}; X : a=ID b=ID c=ID ;", "nt:X", "nt:X");
}

TEST(calc__and_class_expr) {
	test_value("%token NOTHING; X : NOTHING {Z} ;", "nt:X", "cl:Z");
}

TEST(calc__and_empty_result_expr) {
	test_value("%token NOTHING; X : NOTHING %this=() ;", "void", "void");
}

TEST(calc__and_nonempty_result_expr) {
	test_value("%token NOTHING; %token ID{str}; X : NOTHING %this=ID ;", "user:str", "user:str");
}

TEST(calc__this_void_expr) {
	test_value("X : %this=() ;", "void", "void");
}

TEST(calc__this_nonvoid_expr) {
	test_value("X : %this=({Z}) ;", "nt:X", "cl:Z");
}

TEST(calc__attr_expr) {
	test_value("X : a=({Z}) ;", "nt:X", "nt:X");
}

TEST(calc__cast_expr) {
	test_value("X : {Z}({Q}) ;", "nt:X", "cl:Z");
}

TEST(calc__zero_one_void_expr) {
	test_value("X : ()? ;", "void", "void");
}

TEST(calc__zero_one_nonvoid_expr) {
	test_value("X : ({Z})? ;", "nt:X", "cl:Z");
}

TEST(calc__zero_many_void_expr) {
	test_value("X : ()* ;", "void", "void");
}

TEST(calc__zero_many_nonvoid_expr) {
	test_value("X : ({Z})* ;", "array[cl:Z]", "array[cl:Z]");
}

TEST(calc__string_expr) {
	test_value("X : \"aaa\" ;", "void", "void");
}

TEST(calc__name_void_token_expr) {
	test_value("%token NOTHING; X : NOTHING ;", "void", "void");
}

TEST(calc__name_nonvoid_token_expr) {
	test_value("%token ID{str}; X : ID ;", "user:str", "user:str");
}

TEST(calc__name_void_nt_expr) {
	test_value("X : Y ; Y : ;", "void", "void");
}

TEST(calc__name_nonvoid_nt_expr) {
	test_value("X : Y ; Y : {Z} ;", "nt:X", "nt:Y");
}

TEST(calc__nt_explicit_type_primitive) {
	test_value("%token ID{str}; X{str} : ID ;", "user:str", "user:str");
}

TEST(calc__nt_explicit_type_nt) {
	test_value("X{Z} : Y ; Y : {P} ; Z : {Q} ;", "nt:Z", "nt:Y");
}

TEST(calc__nt_explicit_type_class) {
	test_value("X{C} : Y ; Y : {Z} ;", "cl:C", "nt:Y");
}

TEST(calc__const_expr) {
	test_value("X : <123> ;", "sys:const_int", "sys:const_int");
}

TEST(error__or_undefined_type) {
	test_fail("%token ID{str}; A : ID z=(a=ID | b=ID) ;", "Type of expression is undefined");
}

TEST(error__or_different_primitives) {
	test_fail("%token ID{str}; %token NUM{num}; A : ID | NUM ;", "Types of alternative expressions are incompatible");
}

TEST(error__or_arrays_of_different_primitives) {
	test_fail("%token ID{str}; %token NUM{num}; A : ID+ | NUM+ ;", "Types of alternative expressions are incompatible");
}

TEST(error__or_arrays_of_different_classes) {
	test_fail("A : P+ | Q+ ; P : {X} ; Q : {Y} ; ", "Types of alternative expressions are incompatible");
}

TEST(error__cast_different_primitives) {
	test_fail("%token ID{str}; %token NUM{num}; A : {str}(NUM) ;", "Cannot cast incompatible types");
}

TEST(error__nt_explicit_type_different_primitives) {
	test_fail("%token ID{str}; %token NUM{num}; A{str} : NUM ;", "Cannot cast incompatible types");
}

TEST(error___type_of_AND_inside_attr_is_not_nspecified) {
	test_fail("%token ID {str}; X : x=ID b=(a=ID {C}|b=ID) ;", "Type of expression is undefined");
}

TEST(ok__type_of_AND_inside_THIS_is_not_specified) {
	test_ok("%token ID {str}; X : ID %this=(a=ID {C}|b=ID) ;");
}

TEST(error__attribute_expression_undefined_type) {
	test_fail("%token ID {str}; X : a=(b=ID) ;", "Type of attribute expression is undefined");
}

TEST(error__attribute_expression_undefined_type_2) {
	test_fail("%token ID {str}; X : a=(b=ID c=ID) ;", "Type of attribute expression is undefined");
}

TEST(error__loop_expression_undefined_type) {
	test_fail("%token ID {str}; X : (b=ID)+ ;", "Type of loop body is undefined");
}

TEST(error__loop_expression_undefined_type_2) {
	test_fail("%token ID {str}; X : (b=ID c=ID)+ ;", "Type of loop body is undefined");
}

TEST(calc__and_non_result_void) {
	test_tree("%token VOID; A : VOID VOID VOID ;", "void(void(?,?,?))");
}

TEST(calc__and_non_result_zero_one_void) {
	test_tree("%token VOID; A : VOID VOID VOID? ;", "void(void(?,?,?(?)))");
}

TEST(calc__and_non_result_and_void) {
	test_tree("%token VOID; A : VOID VOID (VOID VOID VOID) ;", "void(void(?,?,?(?,?,?)))");
}

TEST(calc__nt_type_vs_and_type1) {
	test_value("%token ID{str}; X{A} : k=ID {B};", "cl:A", "cl:B");
}

TEST(calc__nt_type_vs_and_type2) {
	test_tree("%token ID{str}; X{A} : k=ID {B} | q=ID {C} ;", "cl:A(cl:A(cl:B(0(user:str)),cl:C(0(user:str))))");
}

TEST(calc__nt_type_vs_and_type3) {
	test_tree("%token ID{str}; X{A} : k=ID {B} | C ; C : {C} ;", "cl:A(cl:A(cl:B(0(user:str)),nt:C))");
}

TEST(calc__nt_type_vs_and_type4) {
	test_tree("%token ID{str}; X{A} : k=ID {X} | C ; C : {C} ;", "cl:A(cl:A(nt:X(0(user:str)),nt:C))");
}

}
