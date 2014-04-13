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

//Part of EBNF Grammar Builder responsible for the detection of void nonterminals and expressions.

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "commons.h"
#include "ebnf__imp.h"
#include "ebnf_bld_common.h"
#include "ebnf_bld_property.h"
#include "ebnf_builder.h"
#include "ebnf_extension__imp.h"
#include "ebnf_visitor__imp.h"
#include "types.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

using util::MPtr;

namespace {

	enum IsVoidValue {
		IS_VOID_RECURSION,
		IS_VOID_FALSE,
		IS_VOID_TRUE
	};

	//
	//IsVoidExpressionPropertyAccessor
	//

	class IsVoidExpressionPropertyAccessor : public ns::ExpressionPropertyAccessor<IsVoidValue> {
		NONCOPYABLE(IsVoidExpressionPropertyAccessor);

	public:
		IsVoidExpressionPropertyAccessor(){}

		std::pair<bool, IsVoidValue> get_property_nt(ebnf::NonterminalDeclaration* nt) override {
			if (nt->get_extension()->is_void_defined()) {
				bool is_void = nt->get_extension()->is_void();
				IsVoidValue result = is_void ? IS_VOID_TRUE : IS_VOID_FALSE;
				return std::make_pair(true, result);
			} else {
				return std::make_pair(false, IsVoidValue());
			}
		}
		
		void set_property_nt(ebnf::NonterminalDeclaration* nt, IsVoidValue value) override {
			bool is_void = IS_VOID_RECURSION == value || IS_VOID_TRUE == value;
			nt->get_extension()->set_is_void(is_void);
		}
		
		void set_property_expr(ebnf::SyntaxExpression* expr, IsVoidValue value) override {
			//Must not be recursion here.
			assert(IS_VOID_RECURSION != value);
			bool is_void = IS_VOID_TRUE == value;
			expr->get_extension()->set_is_void(is_void);
		}
	};

	//
	//IsVoidExpressionPropertyCalculator
	//

	class IsVoidExpressionPropertyCalculator : public ns::ExpressionPropertyCalculator<IsVoidValue> {
		NONCOPYABLE(IsVoidExpressionPropertyCalculator);

		ns::EBNF_Builder* const m_builder;

	public:
		IsVoidExpressionPropertyCalculator(ns::EBNF_Builder* builder) : m_builder(builder){}

		IsVoidValue calculate_Recursion() override {
			return IS_VOID_RECURSION;
		}

		IsVoidValue calculate_Or(ebnf::SyntaxExpression* expr, IsVoidValue value1, IsVoidValue value2) override {
			if (IS_VOID_TRUE == value1) {
				return value2;
			} else if (IS_VOID_TRUE == value2) {
				return value1;
			} else if (IS_VOID_RECURSION == value1) {
				return value2;
			} else if (IS_VOID_RECURSION == value2) {
				return value1;
			}
			assert(IS_VOID_FALSE == value1);
			assert(IS_VOID_FALSE == value2);
			return IS_VOID_FALSE;
		}

		IsVoidValue calculate_PrimitiveType(const types::PrimitiveType* type) override {
			return IS_VOID_FALSE;
		}
		
		IsVoidValue calculate_NameClassType(const types::NameClassType* type) override {
			return IS_VOID_FALSE;
		}
		
		IsVoidValue calculate_NonterminalClassType(const types::NonterminalClassType* type, IsVoidValue sub_value) override {
			if (IS_VOID_TRUE == sub_value) {
				//TODO Specify the name of the nonterminal.
				throw ns::raise_error(m_pos, "Cannot use a void nonterminal as an explicit type");
			}
			return sub_value;
		}

		IsVoidValue calculate_VoidType() override {
			return IS_VOID_TRUE;
		}

		IsVoidValue calculate_NameSyntaxElement(ebnf::NameSyntaxElement* expr, IsVoidValue sub_value) override {
			if (IS_VOID_TRUE == sub_value) {
				throw ns::raise_error(expr->get_name(), "Cannot assign a void expression");
			}
			return IS_VOID_FALSE;
		}

		IsVoidValue calculate_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr, IsVoidValue sub_value) override {
			return sub_value;
		}
		
		IsVoidValue calculate_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) override {
			return IS_VOID_FALSE;
		}

		IsVoidValue calculate_SyntaxAndExpression_Class(ebnf::SyntaxAndExpression* expr) override {
			return IS_VOID_FALSE;
		}
		
		IsVoidValue calculate_SyntaxAndExpression_Type(ebnf::SyntaxAndExpression* expr, IsVoidValue type_value) override {
			const types::Type* type = expr->get_type();
			
			//The type must exist and be a class type here.
			assert(type);
			assert(type->as_class());
			
			//Ensure that the type is not a void nonterminal.
			if (IS_VOID_TRUE == type_value) throw ns::raise_error(m_pos, "Cannot use a void type");

			return IS_VOID_FALSE;
		}

		IsVoidValue calculate_Cast(
			MPtr<const types::Type> cast_type,
			IsVoidValue cast_type_value,
			IsVoidValue actual_value) override
		{
			//The type must not be void.
			if (IS_VOID_TRUE == cast_type_value) throw ns::raise_error(m_pos, "Cannot cast to void type");
			if (IS_VOID_TRUE == actual_value) throw ns::raise_error(m_pos, "Cannot cast a void expression");

			//OR operation is suitable here. If both the values are recursions, the result has to be
			//recursion also. Otherwise, the result is FALSE.
			IsVoidValue result = calculate_Or(nullptr, cast_type_value, actual_value);
			return result;
		}
	};

	void print_is_void(ebnf::NonterminalDeclaration* nt) {
		std::cout << "is_void " << nt->get_name() << " " << nt->get_extension()->is_void() << "\n";
	}

}

//
//EBNF_Builder
//

void ns::EBNF_Builder::calculate_is_void() {
	assert(m_verify_attributes_completed);
	assert(!m_calculate_is_void_completed);

	IsVoidExpressionPropertyAccessor property_accessor;
	IsVoidExpressionPropertyCalculator calculator(this);
	const std::vector<ebnf::NonterminalDeclaration*>& nts = m_grammar->get_nonterminals();
	calculate_expression_property(this, nts, &property_accessor, &calculator);

	if (m_verbose_output) {
		std::cout << "*** VOID ***\n";
		std::cout << '\n';
		for (ebnf::NonterminalDeclaration* nt : nts) print_is_void(nt);
		std::cout << '\n';
	}

	m_calculate_is_void_completed = true;
}
