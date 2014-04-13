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

//Part of EBNF Grammar Builder responsible for types calculation.

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
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

namespace {

	using util::MPtr;

	void define_explicit_concrete_type(ns::EBNF_Builder& builder, ebnf::NonterminalDeclaration* nt) {
		ns::NonterminalDeclarationExtension* nt_extension = nt->get_extension();

		MPtr<const types::Type> type = nt->get_explicit_type();
		if (!type.get()) {
			ns::GeneralType general_type = nt_extension->get_general_type();
			if (ns::GENERAL_TYPE_CLASS == general_type) {
				type = builder.create_nt_class_type(nt);
			} else if (ns::GENERAL_TYPE_VOID == general_type) {
				type = builder.get_void_type();
			}
		}

		if (type.get()) nt_extension->set_concrete_type(type);
	}

	void define_expected_type(ebnf::SyntaxExpression* expr, MPtr<const types::Type> expected_type);

	template<class E>
	void define_expected_type_for_all(const std::vector<MPtr<E>>& exprs, MPtr<const types::Type> expected_type) {
		struct DefiningVisitor : public ns::SyntaxExpressionVisitor<void> {
			MPtr<const types::Type> const m_expected_type;
			DefiningVisitor(MPtr<const types::Type> expected_type) : m_expected_type(expected_type){}

			void visit_SyntaxExpression(ebnf::SyntaxExpression* expr) override {
				define_expected_type(expr, m_expected_type);
			}
		};

		DefiningVisitor defining_visitor(expected_type);
		ebnf::SyntaxExpression::visit_all(exprs, &defining_visitor);
	}

	void define_expected_type(ebnf::SyntaxExpression* expr, MPtr<const types::Type> expected_type) {
		expr->get_extension()->set_expected_type(expected_type);

		struct DistributingVisitor : public ns::SyntaxExpressionVisitor<void> {
			MPtr<const types::Type> const m_expected_type;
			DistributingVisitor(MPtr<const types::Type> expected_type) : m_expected_type(expected_type){}

			void visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) override {
				define_expected_type_for_all(expr->get_sub_expressions(), m_expected_type);
			}
			
			void visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override {
				ns::AndExpressionMeaning* meaning = expr->get_and_extension()->get_meaning();
				define_expected_type_for_all(meaning->get_non_result_sub_expressions(), MPtr<const types::Type>());

				struct MeaningVisitor : public ns::AndExpressionMeaningVisitor<void> {
					MPtr<const types::Type> const m_expected_type;
					MeaningVisitor(MPtr<const types::Type> expected_type) : m_expected_type(expected_type){}
					
					void visit_ThisAndExpressionMeaning(ns::ThisAndExpressionMeaning* meaning) override {
						define_expected_type_for_all(meaning->get_result_elements(), m_expected_type);
					}
				};
				MeaningVisitor meaning_visitor(m_expected_type);
				expr->get_and_extension()->get_meaning()->visit(&meaning_visitor);
			}
			
			void visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) override {
				define_expected_type(expr->get_expression(), MPtr<const types::Type>());
			}
			
			void visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) override {
				define_expected_type(expr->get_expression(), m_expected_type);
			}
			
			void visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) override {
				define_expected_type(expr->get_expression(), expr->get_type());
			}
			
			void visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override {
				define_expected_type(expr->get_sub_expression(), m_expected_type);
			}
			
			void visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) override {
				const ebnf::LoopBody* body = expr->get_body();
				define_expected_type(body->get_expression(), nullptr);
				if (body->get_separator()) define_expected_type(body->get_separator(), nullptr);
			}
		};

		DistributingVisitor distributing_visitor(expected_type);
		expr->visit(&distributing_visitor);
	}

	void define_expected_types_for_nt(ebnf::NonterminalDeclaration* nt) {
		ns::NonterminalDeclarationExtension* ext = nt->get_extension();

		//If the concrete type of the nonterminal has not been defined yet, 0 will be as as the expected
		//type for the expression.
		MPtr<const types::Type> type = ext->concrete_type_defined() ? ext->get_concrete_type() : nullptr;
		define_expected_type(nt->get_expression(), type);
	}

	bool types_equal(MPtr<const types::Type> type1, MPtr<const types::Type> type2) {

		class EqualVisitor : public ns::TypeVisitor<bool> {
			MPtr<const types::Type> m_other_type;

		public:
			EqualVisitor(MPtr<const types::Type> other_type)
				: m_other_type(other_type)
			{}

			bool visit_Type(const types::Type* type) override {
				throw ns::err_illegal_state();
			}
			
			bool visit_PrimitiveType(const types::PrimitiveType* type) override {
				return m_other_type.get() == type; //pointers comparison.
			}
			
			bool visit_ClassType(const types::ClassType* type) override {
				return m_other_type.get() == type; //pointers comparison.
			}
			
			bool visit_VoidType(const types::VoidType* type) override {
				return m_other_type.get() == type; //pointers comparison.
			}
			
			bool visit_ArrayType(const types::ArrayType* type) override {

				class ArrayEqualVisitor : public ns::TypeVisitor<bool> {
					MPtr<const types::Type> const m_other_element_type;

				public:
					ArrayEqualVisitor(MPtr<const types::Type> other_element_type)
						: m_other_element_type(other_element_type)
					{}

					bool visit_Type(const types::Type* type) {
						return false;
					}
					
					bool visit_ArrayType(const types::ArrayType* type) {
						MPtr<const types::Type> element_type = type->get_element_type();
						bool equal = types_equal(m_other_element_type, element_type);
						return equal;
					}
				};

				ArrayEqualVisitor array_visitor(type->get_element_type());
				bool equal = m_other_type->visit(&array_visitor);
				return equal;
			}

		};

		EqualVisitor equal_visitor(type2);
		bool equal = type1->visit(&equal_visitor);
		return equal;
	}

	//
	//TypeValue
	//

	class TypeValue {
		bool m_recursion;
		MPtr<const types::Type> m_type;

	public:
		explicit TypeValue(MPtr<const types::Type> type = MPtr<const types::Type>(), bool recursion = false)
			: m_type(type),
			m_recursion(recursion)
		{}

		MPtr<const types::Type> type() const { return m_type; }
		bool recursion() const { return m_recursion; }
		static TypeValue make_recursion() { return TypeValue(nullptr, true); }
	};

	//
	//TypeExpressionPropertyAccessor
	//

	class TypeExpressionPropertyAccessor : public ns::ExpressionPropertyAccessor<TypeValue> {
		NONCOPYABLE(TypeExpressionPropertyAccessor);

		ns::EBNF_Builder* const m_builder;

	public:
		TypeExpressionPropertyAccessor(ns::EBNF_Builder* builder) : m_builder(builder){}

		std::pair<bool, TypeValue> get_property_nt(ebnf::NonterminalDeclaration* nt) override {
			ns::NonterminalDeclarationExtension* ext = nt->get_extension();
			MPtr<const types::Type> type = ext->concrete_type_defined() ? ext->get_concrete_type() : nullptr;
			return std::make_pair(!!type.get(), TypeValue(type));
		}
		
		void set_property_nt(ebnf::NonterminalDeclaration* nt, TypeValue value) override {
			MPtr<const types::Type> type;
			if (value.recursion()) {
				type = m_builder->get_void_type();
			} else {
				type = value.type();
				//The type must be defined here (this must have been already checked).
				assert(type.get());
			}
			nt->get_extension()->set_concrete_type(type);
		}
		
		void set_property_expr(ebnf::SyntaxExpression* expr, TypeValue value) override {
			//There must not be recursion here.
			assert(!value.recursion());
			
			//The type can be undefined.
			expr->get_extension()->set_concrete_type(value.type());
		}
	};

	//
	//TypeExpressionPropertyCalculator
	//

	class TypeExpressionPropertyCalculator : public ns::ExpressionPropertyCalculator<TypeValue> {
		NONCOPYABLE(TypeExpressionPropertyCalculator);

		MPtr<ebnf::Grammar> const m_grammar;
		ns::EBNF_Builder* const m_builder;

	public:
		TypeExpressionPropertyCalculator(MPtr<ebnf::Grammar> grammar, ns::EBNF_Builder* builder)
			: m_grammar(grammar), m_builder(builder)
		{}

		TypeValue calculate_Recursion() override {
			return TypeValue::make_recursion();
		}

		TypeValue calculate_Or(ebnf::SyntaxExpression* expr, TypeValue type_value1, TypeValue type_value2) override {
			
			MPtr<const types::Type> type1 = type_value1.type();
			MPtr<const types::Type> type2 = type_value2.type();

			if (!type_value1.recursion() && !type1.get() || !type_value2.recursion() && !type2.get()) {
				throw ns::raise_error(m_pos, "Type of expression is undefined");
			}

			if (!type_value1.recursion() && type1->is_void()) {
				return type_value2;
			} else if (!type_value2.recursion() && type2->is_void()) {
				return type_value1;
			} else if (type_value1.recursion()) {
				return type_value2;
			} else if (type_value2.recursion()) {
				return type_value1;
			}

			if (types_equal(type1, type2)) {
				return type_value1;
			} else if (type1->as_class() && type2->as_class()) {
				MPtr<const types::Type> expected_type = expr->get_extension()->get_expected_type();
				//The type can be undefined.
				return TypeValue(expected_type);
			}

			//TODO Specify which types are used.
			throw ns::raise_error(m_pos, "Types of alternative expressions are incompatible");
		}

		TypeValue calculate_PrimitiveType(const types::PrimitiveType* type) override {
			return TypeValue(MPtr<const types::Type>::unsafe_cast(type));
		}
		
		TypeValue calculate_NameClassType(const types::NameClassType* type) override {
			return TypeValue(MPtr<const types::Type>::unsafe_cast(type));
		}
		
		TypeValue calculate_NonterminalClassType(const types::NonterminalClassType* type, TypeValue sub_value) override {
			//There must not be recursion - types of all class nonterminals have already been defined.
			assert(!sub_value.recursion());

			//The actual type must have already been verified to be a class.
			MPtr<const types::Type> sub_type = sub_value.type();
			assert(!sub_type.get() || sub_type->as_class());
			
			return sub_value;
		}

		TypeValue calculate_VoidType() override {
			return TypeValue(m_builder->get_void_type());
		}

		TypeValue calculate_NameSyntaxElement(ebnf::NameSyntaxElement* expr, TypeValue sub_value) override {
			if (!sub_value.recursion()) {
				MPtr<const types::Type> sub_type = sub_value.type();
				if (!sub_type.get()) throw ns::raise_error(m_pos, "Type of attribute expression is undefined");
				assert(!sub_value.type()->is_void());
			}
			
			//If there is no expected type, return 'undefined' type.
			MPtr<const types::Type> expected_type = expr->get_extension()->get_expected_type();
			assert(!expected_type.get() || expected_type->as_class());
			return TypeValue(expected_type);
		}

		TypeValue calculate_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr, TypeValue sub_value) override {
			//There must not be recursion through a loop. This must have already been verified.
			assert(!sub_value.recursion());
			MPtr<const types::Type> sub_type = sub_value.type();
			if (!sub_type.get()) throw ns::raise_error(m_pos, "Type of loop body is undefined");

			TypeValue result;
			if (sub_type->is_void()) {
				result = TypeValue(m_builder->get_void_type());
			} else {
				//TODO Optimize: reuse identical array type instances.
				MPtr<const types::Type> type = m_builder->manage_type(new types::ArrayType(sub_type));
				result = TypeValue(type);
			}
			return result;
		}
		
		TypeValue calculate_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) override {
			class ConstVisitor : public ns::ConstExpressionVisitor<MPtr<const types::Type>> {
				ns::EBNF_Builder* const m_builder;
				MPtr<const types::Type> const m_expected_type;

			public:
				ConstVisitor(ns::EBNF_Builder* builder, MPtr<const types::Type> expected_type)
					: m_builder(builder),
					m_expected_type(expected_type)
				{}

				MPtr<const types::Type> visit_ConstExpression(const ebnf::ConstExpression* expr) override {
					throw ns::err_illegal_state();
				}
				MPtr<const types::Type> visit_IntegerConstExpression(const ebnf::IntegerConstExpression* expr) override {
					return m_builder->get_const_integer_type();
				}
				MPtr<const types::Type> visit_StringConstExpression(const ebnf::StringConstExpression* expr) override {
					return m_builder->get_const_string_type();
				}
				MPtr<const types::Type> visit_BooleanConstExpression(const ebnf::BooleanConstExpression* expr) override {
					return m_builder->get_const_boolean_type();
				}
				MPtr<const types::Type> visit_NativeConstExpression(const ebnf::NativeConstExpression* expr) override {
					if (!m_expected_type.get()) {
						const ns::syntax_string& name = expr->get_name()->get_name();
						throw ns::raise_error(name, "Type of native constant expression is undefined");
					}
					return m_expected_type;
				}
			};

			MPtr<const types::Type> expected_type = expr->get_extension()->get_expected_type();
			ConstVisitor const_visitor(m_builder, expected_type);
			MPtr<const types::Type> type = expr->get_expression()->visit(&const_visitor);
			return TypeValue(type);
		}

		TypeValue calculate_SyntaxAndExpression_Class(ebnf::SyntaxAndExpression* expr) override {
			MPtr<const types::Type> expected_type = expr->get_extension()->get_expected_type();
			assert(!expected_type.get() || expected_type->as_class());
			return TypeValue(expected_type);
		}
		
		TypeValue calculate_SyntaxAndExpression_Type(ebnf::SyntaxAndExpression* expr, TypeValue type_value) override {
			const types::Type* type = expr->get_type();
			
			//There cannot be a recursion here, since the explicit type is a class type.
			assert(!type_value.recursion());
			assert(type_value.type()->as_class());

			//The type must exist and be a class type here.
			assert(type);
			assert(type->as_class());

			//Return the explicit type, not the type of the corresponding nonterminal (if any).
			//(Not doing so is a bug, since "AddExpr{Expr} : AddExpr '+' MulExpr {AddExpr}" must return AddExpr, not Expr.
			return TypeValue(MPtr<const types::ClassType>::unsafe_cast(type->as_class()));
		}

		TypeValue calculate_Cast(MPtr<const types::Type> cast_type, TypeValue cast_type_value, TypeValue actual_value) override {
			//There cannot be a recursion here, since only a class nonterminal can be used as a type, while
			//types of all the class nonterminals must have already been defined.
			assert(!cast_type_value.recursion());
			assert(!actual_value.recursion());

			MPtr<const types::Type> actual_type = actual_value.type();

			//Actual type cannot be undefined, because it has the expected type provided by this cast expression.
			assert(actual_type.get());
			
			//It must have already been verified that the types are not void.
			assert(!cast_type->is_void());
			assert(!actual_type->is_void());

			if (cast_type->as_class() && actual_type->as_class()) return cast_type_value;

			if (!types_equal(cast_type, actual_type)) {
				//TODO Specify which types are used.
				throw ns::raise_error(m_pos, "Cannot cast incompatible types");
			}
			return cast_type_value;
		}
	};

	void print_type(ebnf::NonterminalDeclaration* nt) {
		MPtr<const types::Type> type = nt->get_extension()->get_concrete_type();
		std::cout << "type " << nt->get_name() << " ";
		type->print(std::cout);
		std::cout << "\n";
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//EBNF_Builder
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ns::EBNF_Builder::calculate_types() {
	assert(m_calculate_general_types_completed);
	assert(!m_calculate_types_completed);

	//Define concrete types for nonterminals where explicit types are specified, or the nonterminal
	//has a class type.
	for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) {
		define_explicit_concrete_type(*this, nt);
	}

	//Define expected types for all expressions.
	for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) define_expected_types_for_nt(nt);

	//Calculate explicit types for all nonterminals and expressions.
	TypeExpressionPropertyAccessor property_accessor(this);
	TypeExpressionPropertyCalculator calculator(m_grammar, this);
	calculate_expression_property(this, m_grammar->get_nonterminals(), &property_accessor, &calculator);

	if (m_verbose_output) {
		std::cout << "*** TYPES ***\n";
		std::cout << '\n';
		for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) print_type(nt);
		std::cout << '\n';
	}
	
	m_calculate_types_completed = true;
}
