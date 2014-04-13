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

//Unit tests for the EBNF Grammar Builder related to semantic attributes processing.

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

namespace {

	using util::MPtr;

	class Helper : public syn_test::AbstractHelper {
	public:
		Helper(const char* grammar_text) : AbstractHelper(grammar_text, &ns::EBNF_BuilderTestGate::verify_attributes){}
	};

	void test_ok(const char* grammar_text) {
		Helper helper(grammar_text);
		helper.test_ok();
	}

	void test_fail(const char* grammar_text, const char* error_msg) {
		Helper helper(grammar_text);
		helper.test_fail(error_msg);
	}

	void assert_name_expr(const ebnf::SyntaxExpression* expr, const char* name) {
		const ebnf::NameSyntaxExpression* name_expr = dynamic_cast<const ebnf::NameSyntaxExpression*>(expr);
		assertNotNull(name_expr);
		assertEquals(name, name_expr->get_name().str());
	}

}

namespace {

TEST(ok__no_attrs) {
	Helper helper("%token ID {str}; X : ID ID ID ;");
	helper.test_ok();

	//Test AndExpressionMeaning
	const ebnf::Grammar* grammar = helper.get_grammar();
	assertEquals(1, grammar->get_nonterminals().size());

	ebnf::NonterminalDeclaration* nt = grammar->get_nonterminals()[0];
	ebnf::SyntaxExpression* expr = nt->get_expression();
	ebnf::SyntaxAndExpression* and_expr = dynamic_cast<ebnf::SyntaxAndExpression*>(expr);
	assertNotNull(and_expr);

	ns::SyntaxAndExpressionExtension* and_ext = and_expr->get_and_extension();
	assertNotNull(and_ext);

	ns::AndExpressionMeaning* meaning = and_ext->get_meaning();
	assertNotNull(meaning);
	assertEquals(0, meaning->get_non_result_sub_expressions().size());
	
	ns::VoidAndExpressionMeaning* void_meaning = dynamic_cast<ns::VoidAndExpressionMeaning*>(meaning);
	assertNotNull(void_meaning);
}

TEST(ok__many_attrs) {
	Helper helper("%token ID {str}; X : a=ID b=ID c=ID (d=ID|d=ID) ;");
	helper.test_ok();

	//Test AndExpressionMeaning
	const ebnf::Grammar* grammar = helper.get_grammar();
	assertEquals(1, grammar->get_nonterminals().size());

	ebnf::NonterminalDeclaration* nt = grammar->get_nonterminals()[0];
	ebnf::SyntaxExpression* expr = nt->get_expression();
	ebnf::SyntaxAndExpression* and_expr = dynamic_cast<ebnf::SyntaxAndExpression*>(expr);
	assertNotNull(and_expr);

	ns::SyntaxAndExpressionExtension* and_ext = and_expr->get_and_extension();
	assertNotNull(and_ext);

	ns::AndExpressionMeaning* meaning = and_ext->get_meaning();
	assertNotNull(meaning);
	assertEquals(5, meaning->get_non_result_sub_expressions().size());
	
	ns::ClassAndExpressionMeaning* class_meaning = dynamic_cast<ns::ClassAndExpressionMeaning*>(meaning);
	assertNotNull(class_meaning);
	assertTrue(class_meaning->has_attributes());
}

TEST(ok__this) {
	Helper helper(
		"%token ID{str}; %token ID1{str}; %token ID2{str}; %token ID3{str};"
		"X : ID (%this=ID1 | (ID %this=ID2 | %this=ID3 ID)) ;");
	helper.test_ok();

	//Test AndExpressionMeaning
	const ebnf::Grammar* grammar = helper.get_grammar();
	assertEquals(1, grammar->get_nonterminals().size());

	ebnf::NonterminalDeclaration* nt = grammar->get_nonterminals()[0];
	ebnf::SyntaxExpression* expr = nt->get_expression();
	ebnf::SyntaxAndExpression* and_expr = dynamic_cast<ebnf::SyntaxAndExpression*>(expr);
	assertNotNull(and_expr);

	ns::SyntaxAndExpressionExtension* and_ext = and_expr->get_and_extension();
	assertNotNull(and_ext);

	ns::AndExpressionMeaning* meaning = and_ext->get_meaning();
	assertNotNull(meaning);
	assertEquals(0, meaning->get_non_result_sub_expressions().size());
	
	ns::ThisAndExpressionMeaning* this_meaning = dynamic_cast<ns::ThisAndExpressionMeaning*>(meaning);
	assertNotNull(this_meaning);

	const std::vector<MPtr<ebnf::ThisSyntaxElement>>& elems = this_meaning->get_result_elements();
	assertEquals(3, elems.size());
	assert_name_expr(elems[0]->get_expression(), "ID1");
	assert_name_expr(elems[1]->get_expression(), "ID2");
	assert_name_expr(elems[2]->get_expression(), "ID3");
}

TEST(ok__dup_name_in_nested_expr) {
	test_ok("%token ID {str}; X : a=ID b=(a=ID) ;");
}

TEST(ok__dup_name_in_loop_expr) {
	test_ok("%token ID {str}; X : a=ID b=(a=ID)+ ;");
}

TEST(ok__dup_this_in_nested_expr_2) {
	test_ok("%token ID {str}; X : ID %this=(this=ID) ;");
}

TEST(ok__dup_attr_inside_inner_and_with_type) {
	test_ok("%token ID {str}; X : a=ID b=(a=ID {C}|b=ID {C}) ;");
}

TEST(ok__nested_this_or_nested_attr) {
	test_ok("%token ID{str}; X : (%this=ID | a=ID) ;");
}

TEST(conflict__same_and_expr) {
	test_fail("%token ID{str}; X : a=ID a=ID ;", "Attribute name conflict: 'a'");
}

TEST(conflict__top_and_nested) {
	test_fail("%token ID{str}; X : a=ID ( a=ID ) ;", "Attribute name conflict: 'a'");
}

TEST(conflict__top_and_zero_one) {
	test_fail("%token ID{str}; X : a=ID ( a=ID )? ;", "Attribute name conflict: 'a'");
}

TEST(conflict__top_and_nested_alternative) {
	test_fail("%token ID{str}; X : a=ID ( b=ID | a=ID ) ;", "Attribute name conflict: 'a'");
}

TEST(conflict__deep_nested) {
	test_fail("%token ID{str}; X : a=ID ( ID ( ID ( a=ID ) ID ) ID ) ;", "Attribute name conflict: 'a'");
}

TEST(conflict__nested_vs_nested) {
	test_fail("%token ID{str}; X : ((( a=ID ))) ID ((( a=ID ))) ;", "Attribute name conflict: 'a'");
}

TEST(conflict__this_top_and_expr) {
	test_fail("%token ID{str}; X : %this=ID %this=ID ;", "Result element conflict: '%this'");
}

TEST(conflict__this_top_and_nested) {
	test_fail("%token ID{str}; X : %this=ID ( %this=ID ) ;", "Result element conflict: '%this'");
}

TEST(conflict__this_top_and_zero_one) {
	test_fail("%token ID{str}; X : %this=ID ( %this=ID )? ;", "Result element conflict: '%this'");
}

TEST(conflict__this_top_and_nested_alternative) {
	test_fail("%token ID{str}; X : %this=ID ( ID | %this=ID ) ;", "Result element conflict: '%this'");
}

TEST(conflict__this_deep_nested) {
	test_fail("%token ID{str}; X : %this=ID ( ID ( ID ( %this=ID ) ID ) ID ) ;", "Result element conflict: '%this'");
}

TEST(conflict__this_nested_vs_nested) {
	test_fail("%token ID{str}; X : ((( %this=ID ))) ID ((( %this=ID ))) ;", "Result element conflict: '%this'");
}

TEST(conflict__nested_this_vs_nested_attr) {
	test_fail("%token ID{str}; X : ID (%this=ID | a=ID) ID ;", "Attribute and '%this' conflict: 'a'");
}

TEST(conflict__deep_this_vs_deep_attr) {
	test_fail("%token ID{str}; X : ID (((ID ID | ID %this=ID) | ID ID) | ((ID ID | a=ID ID) | ID ID)) ID ;",
		"Attribute and '%this' conflict: 'a'");
}

TEST(conflict__nested_attr_vs_nested_this) {
	test_fail("%token ID{str}; X : ID (a=ID | %this=ID) ID ;", "Attribute and '%this' conflict");
}

TEST(conflict__deep_attr_vs_deep_this) {
	test_fail("%token ID{str}; X : ID (((ID ID | a=ID ID) | ID ID) | ((ID ID | %this=ID ID) | ID ID)) ID ;",
		"Attribute and '%this' conflict");
}

TEST(error__dependent_and_has_explicit_type) {
	test_fail("%token ID{str}; X : ID (a=ID {ZZ}) ID ;", "Nested AND expression cannot have an explicit type");
}

TEST(conflict__this_vs_class) {
	test_fail("%token ID{str}; X : %this=X {ZZ} ;", "AND expression has both '%this' and the class type specified");
}

TEST(conflict__this_vs_attr) {
	test_fail("%token ID{str}; X : %this=X a=ID ;", "Attribute and '%this' conflict: 'a'");
}

TEST(conflict__inside_cast) {
	test_fail("%token ID {str}; X : {C2}(a=ID (b=ID|a=ID)) ;", "Attribute name conflict: 'a'");
}

TEST(conflict__inside_loop) {
	test_fail("%token ID {str}; X : (a=ID (b=ID | a=ID))+ ;", "Attribute name conflict: 'a'");
}

TEST(error__deep_dependent_AND_has_explicit_type) {
	test_fail("%token ID {str}; X : a=(b=( x=ID (y=ID {C}) )) ;", "Nested AND expression cannot have an explicit type");
}

}
