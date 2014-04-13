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

//Part of the EBNF Grammar Builder responsible for general types calculation.

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//GeneralTypeExpressionPropertyAccessor, etc.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	enum GTValue {
		GT_RECURSION,
		GT_VOID,
		GT_PRIMITIVE,
		GT_ARRAY,
		GT_CLASS
	};

	GTValue convert_general_type(ns::GeneralType general_type) {
		//Table conversion would be faster and shorter, but it is not safe, since it depends on the order of values,
		//and would compile even if a source value is deleted.
		switch (general_type) {
		case ns::GENERAL_TYPE_VOID:
			return GT_VOID;
		case ns::GENERAL_TYPE_PRIMITIVE:
			return GT_PRIMITIVE;
		case ns::GENERAL_TYPE_ARRAY:
			return GT_ARRAY;
		case ns::GENERAL_TYPE_CLASS:
			return GT_CLASS;
		default:
			throw ns::err_illegal_state();
		}
	}

	ns::GeneralType convert_general_type(GTValue value) {
		switch (value) {
		case GT_VOID:
			return ns::GENERAL_TYPE_VOID;
		case GT_PRIMITIVE:
			return ns::GENERAL_TYPE_PRIMITIVE;
		case GT_ARRAY:
			return ns::GENERAL_TYPE_ARRAY;
		case GT_CLASS:
			return ns::GENERAL_TYPE_CLASS;
		default:
			throw ns::err_illegal_state();
		}
	}

	const char* gt_value_to_string(GTValue value) {
		switch (value) {
		case GT_RECURSION:
			return "recursion";
		case GT_VOID:
			return "void";
		case GT_PRIMITIVE:
			return "primitive";
		case GT_ARRAY:
			return "array";
		case GT_CLASS:
			return "class";
		default:
			throw ns::err_illegal_state();
		}
	}

	//
	//GeneralTypeExpressionPropertyAccessor
	//

	class GeneralTypeExpressionPropertyAccessor : public ns::ExpressionPropertyAccessor<GTValue> {
		NONCOPYABLE(GeneralTypeExpressionPropertyAccessor);

	public:
		GeneralTypeExpressionPropertyAccessor(){}

		std::pair<bool, GTValue> get_property_nt(ebnf::NonterminalDeclaration* nt) override {
			if (nt->get_extension()->general_type_defined()) {
				ns::GeneralType general_type = nt->get_extension()->get_general_type();
				GTValue result = convert_general_type(general_type);
				return std::make_pair(true, result);
			} else {
				return std::make_pair(false, GTValue());
			}
		}
		
		void set_property_nt(ebnf::NonterminalDeclaration* nt, GTValue value) override {
			ns::GeneralType general_type = GT_RECURSION == value ? ns::GENERAL_TYPE_VOID : convert_general_type(value);
			nt->get_extension()->set_general_type(general_type);
		}
		
		void set_property_expr(ebnf::SyntaxExpression* expr, GTValue value) override {
			//There must not be recursion here.
			assert(GT_RECURSION != value);
			ns::GeneralType general_type = convert_general_type(value);
			expr->get_extension()->set_general_type(general_type);
		}
	};

	//
	//GeneralTypeExpressionPropertyCalculator
	//

	class GeneralTypeExpressionPropertyCalculator : public ns::ExpressionPropertyCalculator<GTValue> {
		NONCOPYABLE(GeneralTypeExpressionPropertyCalculator);

		ns::EBNF_Builder* const m_builder;

	public:
		GeneralTypeExpressionPropertyCalculator(ns::EBNF_Builder* builder) : m_builder(builder){}

		GTValue calculate_Recursion() override {
			return GT_RECURSION;
		}

		GTValue calculate_Or(ebnf::SyntaxExpression* expr, GTValue type1, GTValue type2) override {
			if (GT_VOID == type1) return type2;
			if (GT_VOID == type2) return type1;

			//Recursion is more 'privileged' than void, because it may have special meaning in type cast.
			if (GT_RECURSION == type1) return type2;
			if (GT_RECURSION == type2) return type1;
			if (type1 == type2) return type1;

			std::string msg = std::string("Incompatible types of alternative rules: ") +
				gt_value_to_string(type1) + " and " + gt_value_to_string(type2);
			throw ns::raise_error(m_pos, msg);
		}

		GTValue calculate_PrimitiveType(const types::PrimitiveType* type) override {
			return GT_PRIMITIVE;
		}
		
		GTValue calculate_NameClassType(const types::NameClassType* type) override {
			return GT_CLASS;
		}
		
		GTValue calculate_NonterminalClassType(const types::NonterminalClassType* type, GTValue sub_value) override {
			if (GT_CLASS != sub_value && GT_RECURSION != sub_value) {
				std::string msg = std::string() + "Cannot use a non-class nonterminal '" +
					type->get_class_name().str() + "' as an explicit type";
				throw ns::raise_error(m_pos, msg);
			}
			return sub_value;
		}

		GTValue calculate_VoidType() override {
			return GT_VOID;
		}

		GTValue calculate_NameSyntaxElement(ebnf::NameSyntaxElement* expr, GTValue sub_value) override {
			//The sub-value must not be void, since this must has been checked on void calculation phase.
			assert(GT_VOID != sub_value);
			return GT_CLASS;
		}

		GTValue calculate_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr, GTValue sub_value) override {
			bool is_void = expr->get_extension()->is_void();
			GTValue result = is_void ? GT_VOID : GT_ARRAY;
			return result;
		}
		
		GTValue calculate_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) override {
			return GT_PRIMITIVE;
		}

		GTValue calculate_SyntaxAndExpression_Class(ebnf::SyntaxAndExpression* expr) override {
			return GT_CLASS;
		}
		
		GTValue calculate_SyntaxAndExpression_Type(ebnf::SyntaxAndExpression* expr, GTValue type_value) override {
			const types::Type* type = expr->get_type();
			
			//The type must exist and be a class type here.
			assert(type);
			assert(type->as_class());

			//It must have already been checked that the type is not void.
			assert(GT_VOID != type_value);

			if (GT_RECURSION != type_value && GT_CLASS != type_value) {
				throw ns::raise_error(m_pos, "Cannot use a non-class type as an AND expression type");
			}

			return GT_CLASS;
		}

		GTValue calculate_Cast(MPtr<const types::Type> cast_type, GTValue cast_type_value, GTValue actual_value) override {
			//It must have already been verified (during the is_void calculation phase) that the types are not void.
			assert(GT_VOID != cast_type_value);
			assert(GT_VOID != actual_value);

			//It is impossible to use an array type as a cast type.
			assert(GT_ARRAY != cast_type_value);
			
			if (GT_RECURSION != cast_type_value && GT_RECURSION != actual_value && cast_type_value != actual_value) {
				std::string msg = std::string() + "Cannot cast incompatible types: " +
					gt_value_to_string(actual_value) + " to " + gt_value_to_string(cast_type_value);
				throw ns::raise_error(m_pos, msg);
			}

			//OR operation is suitable here - it will handle recursions appropriately.
			GTValue result = calculate_Or(nullptr, cast_type_value, actual_value);
			return result;
		}
	};

}

namespace {

	const char* general_type_to_str(ns::GeneralType type) {
		switch (type) {
			case ns::GENERAL_TYPE_VOID:
				return "VOID";
			case ns::GENERAL_TYPE_PRIMITIVE:
				return "PRIMITIVE";
			case ns::GENERAL_TYPE_ARRAY:
				return "ARRAY";
			case ns::GENERAL_TYPE_CLASS:
				return "CLASS";
			default:
				return "?";
		}
	}

	void print_general_type(ebnf::NonterminalDeclaration* nt) {
		ns::GeneralType general_type = nt->get_extension()->get_general_type();
		std::cout << "general_type " << nt->get_name() << " " << general_type_to_str(general_type) << "\n";
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//EBNF_Builder
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ns::EBNF_Builder::calculate_general_types() {
	assert(m_verify_recursion_completed);
	assert(!m_calculate_general_types_completed);

	const std::vector<ebnf::NonterminalDeclaration*>& nts = m_grammar->get_nonterminals();
	GeneralTypeExpressionPropertyAccessor property_accessor;
	GeneralTypeExpressionPropertyCalculator calculator(this);
	calculate_expression_property(this, nts, &property_accessor, &calculator);

	if (m_verbose_output) {
		std::cout << "*** GENERAL TYPES ***\n";
		std::cout << '\n';
		for (ebnf::NonterminalDeclaration* nt : nts) print_general_type(nt);
		std::cout << '\n';
	}

	m_calculate_general_types_completed = true;
}
