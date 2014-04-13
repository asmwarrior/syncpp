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

//Template framework for calculation of arbitrary properties of syntax expressions.
//Example of a property is a type of a syntax expression.

#ifndef SYN_CORE_EBNF_BLD_PROPERTY_H_INCLUDED
#define SYN_CORE_EBNF_BLD_PROPERTY_H_INCLUDED

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
#include "ebnf_builder.h"
#include "ebnf_extension__imp.h"
#include "ebnf_visitor__imp.h"
#include "noncopyable.h"
#include "primitives.h"
#include "types.h"
#include "util_mptr.h"

namespace synbin {

	//
	//ExpressionPropertyAccessor
	//

	//Interface for property access. Allows getting/setting the property to/from an expression or a nonterminal.
	template<class T>
	class ExpressionPropertyAccessor {
		NONCOPYABLE(ExpressionPropertyAccessor);

	public:
		ExpressionPropertyAccessor(){}
		virtual ~ExpressionPropertyAccessor(){}

		//Reads the value of the property from the specified nonterminal. If the property is already defined for the
		//nonterminal, a pair (true, value) is returned. Otherwise, if the property has to be calculated, a pair
		//(false, T()) is returned.
		virtual std::pair<bool, T> get_property_nt(ebnf::NonterminalDeclaration* nt) = 0;

		//Sets the value of the property of a nonterminal.
		virtual void set_property_nt(ebnf::NonterminalDeclaration* nt, T value) = 0;

		//Sets the value of the property of an expression.
		virtual void set_property_expr(ebnf::SyntaxExpression* expr, T value) = 0;
	};

	//
	//AbstractExpressionPropertyCalculator
	//

	class AbstractExpressionPropertyCalculator {
		NONCOPYABLE(AbstractExpressionPropertyCalculator);

	protected:
		//The file position of the syntax expression being processed. Used for error reporting.
		FilePos m_pos;

	protected:
		AbstractExpressionPropertyCalculator(){}

	public:
		void set_pos(const FilePos& pos) { m_pos = pos; }
	};

	//
	//ExpressionPropertyCalculator
	//

	//Property calculator interface. Used to calculate properties for particular syntax expression nodes.
	template<class T>
	class ExpressionPropertyCalculator : public AbstractExpressionPropertyCalculator {
		NONCOPYABLE(ExpressionPropertyCalculator);

	public:
		ExpressionPropertyCalculator(){}
		virtual ~ExpressionPropertyCalculator(){}

		virtual T calculate_Recursion() = 0;
		virtual T calculate_Or(ebnf::SyntaxExpression* expr, T value1, T value2) = 0;

		virtual T calculate_PrimitiveType(const types::PrimitiveType* type) = 0;
		virtual T calculate_NameClassType(const types::NameClassType* type) = 0;
		virtual T calculate_NonterminalClassType(const types::NonterminalClassType* type, T sub_value) = 0;
		virtual T calculate_VoidType() = 0;

		virtual T calculate_NameSyntaxElement(ebnf::NameSyntaxElement* expr, T sub_value) = 0;
		virtual T calculate_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr, T sub_value) = 0;
		virtual T calculate_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) = 0;
		virtual T calculate_SyntaxAndExpression_Class(ebnf::SyntaxAndExpression* expr) = 0;
		virtual T calculate_SyntaxAndExpression_Type(ebnf::SyntaxAndExpression* expr, T type_value) = 0;

		virtual T calculate_Cast(util::MPtr<const types::Type> cast_type, T cast_type_value, T actual_value) = 0;
	};

	//
	//PropertyCalculationContext
	//

	template<class T>
	class PropertyCalculationContext {
		NONCOPYABLE(PropertyCalculationContext);

		virtual void abstract_method() = 0;

	protected:
		virtual T visit_sym(ebnf::SymbolDeclaration* sym, const FilePos& pos) = 0;
		virtual T visit_expr(ebnf::SyntaxExpression* expr, const FilePos& pos) = 0;

	private:
		//
		//PropTypeVisitor
		//

		class PropTypeVisitor : public synbin::TypeVisitor<T> {
			NONCOPYABLE(PropTypeVisitor);

			PropertyCalculationContext<T>* const m_context;
			const FilePos m_pos;

		public:
			PropTypeVisitor(PropertyCalculationContext<T>* context, const FilePos& pos)
				: m_context(context), m_pos(pos)
			{}

			T visit_Type(const types::Type* type) override {
				throw err_illegal_state();
			}
			
			T visit_PrimitiveType(const types::PrimitiveType* type) override {
				T result = m_context->m_calculator->calculate_PrimitiveType(type);
				return result;
			}
			
			T visit_NonterminalClassType(const types::NonterminalClassType* type) override {
				ebnf::NonterminalDeclaration* nt = type->get_nt();
				T result = m_context->visit_sym(nt, m_pos);
				result = m_context->m_calculator->calculate_NonterminalClassType(type, result);
				return result;
			}
			
			T visit_NameClassType(const types::NameClassType* type) override {
				T result = m_context->m_calculator->calculate_NameClassType(type);
				return result;
			}
			
			T visit_VoidType(const types::VoidType* type) override {
				T result = m_context->m_calculator->calculate_VoidType();
				return result;
			}
		};

		//Calculates the value of the property for a set of alternative syntax expressions.
		template<class P>
		T calculate_or(
			ebnf::SyntaxExpression* expr,
			const std::vector<util::MPtr<P>>& expressions,
			const FilePos& pos)
		{
			T result = m_calculator->calculate_VoidType();
			for (util::MPtr<P> expr : expressions) {
				T value = visit_expr(expr.get(), pos);
				result = m_calculator->calculate_Or(expr.get(), result, value);
			}
			return result;
		}

		EBNF_Builder* const m_builder;
		ExpressionPropertyAccessor<T>* const m_property_accessor;
		ExpressionPropertyCalculator<T>* const m_calculator;

	protected:
		T process_cast(util::MPtr<const types::Type> type, ebnf::SyntaxExpression* sub_expr, const FilePos& pos) {
			PropTypeVisitor type_visitor(this, pos);
			T type_value = type->visit(&type_visitor);
			T sub_value = visit_expr(sub_expr, pos);
			T result = m_calculator->calculate_Cast(type, type_value, sub_value);
			return result;
		}

		//
		//SymVisitor
		//

		class SymVisitor : public SymbolDeclarationVisitor<T> {
			NONCOPYABLE(SymVisitor);

			virtual void abstract_method() = 0;
			
			PropertyCalculationContext<T>* const m_context;
			FilePos const m_pos;

		public:
			SymVisitor(PropertyCalculationContext<T>* context, const FilePos& pos)
				: m_context(context), m_pos(pos)
			{}

			T visit_TerminalDeclaration(ebnf::TerminalDeclaration* tr) override {
				T result;
				
				const types::Type* type = tr->get_type();
				if (type) {
					PropTypeVisitor type_visitor(m_context, m_pos);
					result = type->visit(&type_visitor);
				} else {
					result = m_context->m_calculator->calculate_VoidType();
				}

				return result;
			}
		};

		//
		//ExprVisitor
		//

		class ExprVisitor : public SyntaxExpressionVisitor<T> {
			NONCOPYABLE(ExprVisitor);

			PropertyCalculationContext<T>* const m_context;
			FilePos const m_pos;

		public:
			ExprVisitor(PropertyCalculationContext<T>* context, const FilePos& pos)
				: m_context(context), m_pos(pos)
			{}

			const FilePos& pos() const { return m_pos; }

			T visit_EmptySyntaxExpression(ebnf::EmptySyntaxExpression* expr) override {
				T result = m_context->m_calculator->calculate_VoidType();
				return result;
			}

			T visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) override {
				T result = m_context->calculate_or(expr, expr->get_sub_expressions(), m_pos);
				return result;
			}
			
			T visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override {
				class MeaningVisitor : public AndExpressionMeaningVisitor<T> {
					PropertyCalculationContext<T>* const m_context;
					FilePos const m_pos;
					ebnf::SyntaxAndExpression* const m_expr;

				public:
					MeaningVisitor(
						PropertyCalculationContext<T>* context,
						const FilePos& pos,
						ebnf::SyntaxAndExpression* expr)
						: m_context(context),
						m_pos(pos),
						m_expr(expr)
					{}

					T visit_VoidAndExpressionMeaning(VoidAndExpressionMeaning* meaning) override {
						T result = m_context->m_calculator->calculate_VoidType();
						return result;
					}

					T visit_ThisAndExpressionMeaning(ThisAndExpressionMeaning* meaning) override {
						T result = m_context->calculate_or(m_expr, meaning->get_result_elements(), m_pos);
						return result;
					}

					T visit_ClassAndExpressionMeaning(ClassAndExpressionMeaning* meaning) override {
						T result;

						const types::Type* type = m_expr->get_type();
						if (type) {
							PropTypeVisitor type_visitor(m_context, m_pos);
							T type_value = type->visit(&type_visitor);
							result = m_context->m_calculator->calculate_SyntaxAndExpression_Type(m_expr, type_value);
						} else {
							result = m_context->m_calculator->calculate_SyntaxAndExpression_Class(m_expr);
						}
						
						return result;
					}
				};

				AndExpressionMeaning* meaning = expr->get_and_extension()->get_meaning();

				MeaningVisitor meaning_visitor(m_context, m_pos, expr);
				T result = meaning->visit(&meaning_visitor);
				return result;
			}
			
			T visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) override {
				ebnf::SyntaxExpression* sub_expr = expr->get_expression();
				const FilePos& pos = expr->get_name().pos();
				T sub_value = m_context->visit_expr(sub_expr, pos);
				T result = m_context->m_calculator->calculate_NameSyntaxElement(expr, sub_value);
				return result;
			}

			T visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) override {
				T result = m_context->visit_expr(expr->get_expression(), expr->get_pos());
				return result;
			}

			T visit_NameSyntaxExpression(ebnf::NameSyntaxExpression* expr) override {
				ebnf::SymbolDeclaration* sym = expr->get_sym();
				const FilePos& pos = expr->get_name().pos();
				T result = m_context->visit_sym(sym, pos);
				return result;
			}

			T visit_StringSyntaxExpression(ebnf::StringSyntaxExpression* expr) override {
				util::MPtr<const types::Type> type = m_context->m_builder->get_string_literal_type();
				const FilePos& pos = expr->get_string().pos();
				PropTypeVisitor type_visitor(m_context, pos);
				T result = type->visit(&type_visitor);
				return result;
			}
			
			T visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) override {
				util::MPtr<const types::Type> type = expr->get_type();
				ebnf::SyntaxExpression* sub_expr = expr->get_expression();
				const FilePos& pos = expr->get_raw_type()->get_name().pos();
				T result = m_context->process_cast(type, sub_expr, pos);
				return result;
			}
			
			T visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override {
				T result = m_context->visit_expr(expr->get_sub_expression(), m_pos);
				return result;
			}

			T visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) override {
				ebnf::SyntaxExpression* sub_expr = expr->get_body()->get_expression();
				T sub_value = m_context->visit_expr(sub_expr, m_pos);
				T result = m_context->m_calculator->calculate_LoopSyntaxExpression(expr, sub_value);
				return result;
			}

			T visit_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) override {
				T result = m_context->m_calculator->calculate_ConstSyntaxExpression(expr);
				return result;
			}
		};

		PropertyCalculationContext(
			EBNF_Builder* builder,
			ExpressionPropertyAccessor<T>* property_accessor,
			ExpressionPropertyCalculator<T>* calculator)
			: m_builder(builder),
			m_property_accessor(property_accessor),
			m_calculator(calculator)
		{}

	public:
		ExpressionPropertyAccessor<T>* get_property_accessor() const {
			return m_property_accessor;
		}

		ExpressionPropertyCalculator<T>* get_calculator() const {
			return m_calculator;
		}
	};

	//
	//RootPropertyCalculationContext
	//

	template<class T>
	class RootPropertyCalculationContext : public PropertyCalculationContext<T> {
		NONCOPYABLE(RootPropertyCalculationContext);

		void abstract_method(){}

		//
		//RootSymVisitor
		//

		class RootSymVisitor : public PropertyCalculationContext<T>::SymVisitor {
			NONCOPYABLE(RootSymVisitor);

			void abstract_method() override{}

			RootPropertyCalculationContext<T>* const m_root_context;

		public:
			RootSymVisitor(RootPropertyCalculationContext<T>* context, const FilePos& pos)
				: PropertyCalculationContext<T>::SymVisitor(context, pos),
				m_root_context(context)
			{}

			T visit_NonterminalDeclaration(ebnf::NonterminalDeclaration* nt) override {
				//If the value of the property has already been calculated, return that value.
				std::pair<bool, T> pair = m_root_context->get_property_accessor()->get_property_nt(nt);
				if (pair.first) return pair.second;

				NonterminalDeclarationExtension* ext = nt->get_extension();
				if (ext->set_visiting(true)) return m_root_context->get_calculator()->calculate_Recursion();

				util::MPtr<const types::Type> type = nt->get_explicit_type();
				ebnf::SyntaxExpression* expr = nt->get_expression();
				const FilePos& pos = nt->get_name().pos();

				T result;
				if (type.get()) {
					//Explicit type has been specified for the nonterminal. Process it like a type cast.
					result = m_root_context->process_cast(type, expr, pos);
				} else {
					//Calculate the property of the underlying syntax expression.
					result = m_root_context->visit_expr(expr, pos);
				}

				ext->set_visiting(false);
				return result;
			}
		};

	public:
		RootPropertyCalculationContext(
			EBNF_Builder* builder,
			ExpressionPropertyAccessor<T>* property_accessor,
			ExpressionPropertyCalculator<T>* calculator)
			: PropertyCalculationContext<T>(builder, property_accessor, calculator)
		{}

		T calculate(ebnf::NonterminalDeclaration* nt) {
			//If the property has already been calculated for the nonterminal - use the known value.
			std::pair<bool, T> pair = this->get_property_accessor()->get_property_nt(nt);
			if (pair.first) return pair.second;

			const FilePos& pos = nt->get_name().pos();
			this->get_calculator()->set_pos(pos);
			
			RootSymVisitor root_sym_visitor(this, pos);
			T result = root_sym_visitor.visit_NonterminalDeclaration(nt);
			this->get_property_accessor()->set_property_nt(nt, result);
			return result;
		}

	protected:
		T visit_sym(ebnf::SymbolDeclaration* sym, const FilePos& pos) override {
			RootSymVisitor root_sym_visitor(this, pos);
			return sym->visit(&root_sym_visitor);
		}

		T visit_expr(ebnf::SyntaxExpression* expr, const FilePos& pos) override {
			typename RootPropertyCalculationContext::ExprVisitor root_expr_visitor(this, pos);
			return expr->visit(&root_expr_visitor);
		}
	};

	//
	//DeepPropertyCalculationContext
	//

	template<class T>
	class DeepPropertyCalculationContext : public PropertyCalculationContext<T> {
		NONCOPYABLE(DeepPropertyCalculationContext);

		void abstract_method() override{}

		//
		//DeepSymVisitor
		//

		class DeepSymVisitor : public PropertyCalculationContext<T>::SymVisitor {
			NONCOPYABLE(DeepSymVisitor);

			void abstract_method() override{}

			DeepPropertyCalculationContext<T>* const m_deep_context;

		public:
			DeepSymVisitor(DeepPropertyCalculationContext<T>* context, const FilePos& pos)
				: PropertyCalculationContext<T>::SymVisitor(context, pos),
				m_deep_context(context)
			{}

			T visit_NonterminalDeclaration(ebnf::NonterminalDeclaration* nt) override {
				std::pair<bool, T> pair = m_deep_context->get_property_accessor()->get_property_nt(nt);
				assert(pair.first);
				return pair.second;
			}
		};

		//
		//DeepExprVisitor
		//

		class DeepExprVisitor : public PropertyCalculationContext<T>::ExprVisitor {
			NONCOPYABLE(DeepExprVisitor);

			DeepPropertyCalculationContext<T>* const m_deep_context;

		public:
			DeepExprVisitor(DeepPropertyCalculationContext<T>* context, const FilePos& pos)
				: PropertyCalculationContext<T>::ExprVisitor(context, pos),
				m_deep_context(context)
			{}
			
			T visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override {
				AndExpressionMeaning* meaning = expr->get_and_extension()->get_meaning();
				for (util::MPtr<ebnf::SyntaxExpression> expr : meaning->get_non_result_sub_expressions()) {
					m_deep_context->visit_expr(expr.get(), this->pos());
				}
				T result = PropertyCalculationContext<T>::ExprVisitor::visit_SyntaxAndExpression(expr);
				return result;
			}

			T visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) override {
				const ebnf::LoopBody* body = expr->get_body();
				ebnf::SyntaxExpression* separator = body->get_separator();
				if (separator) m_deep_context->visit_expr(separator, body->get_separator_pos());
				T result = PropertyCalculationContext<T>::ExprVisitor::visit_LoopSyntaxExpression(expr);
				return result;
			}
		};

		//
		//SetterExprVisitor
		//

		class SetterExprVisitor : public SyntaxExpressionVisitor<T> {
			NONCOPYABLE(SetterExprVisitor);

			DeepPropertyCalculationContext<T>* const m_deep_context;
			SyntaxExpressionVisitor<T>* const m_effective_visitor;

		public:
			SetterExprVisitor(
				DeepPropertyCalculationContext<T>* context,
				SyntaxExpressionVisitor<T>* visitor)
				: m_deep_context(context), m_effective_visitor(visitor)
			{
				assert(context);
				assert(visitor);
			}

			T visit_SyntaxExpression(ebnf::SyntaxExpression* expr) override {
				T result = expr->visit(m_effective_visitor);
				m_deep_context->get_property_accessor()->set_property_expr(expr, result);
				return result;
			}
		};

	public:
		DeepPropertyCalculationContext(
			EBNF_Builder* builder,
			ExpressionPropertyAccessor<T>* property_accessor,
			ExpressionPropertyCalculator<T>* calculator)
			: PropertyCalculationContext<T>(builder, property_accessor, calculator)
		{}

		void calculate(ebnf::NonterminalDeclaration* nt) {
			util::MPtr<const types::Type> type = nt->get_explicit_type();
			ebnf::SyntaxExpression* expr = nt->get_expression();

			const FilePos& pos = nt->get_name().pos();
			this->get_calculator()->set_pos(pos);

			if (type.get()) {
				this->process_cast(type, expr, pos);
			} else {
				this->visit_expr(expr, pos);
			}
		}

	protected:
		T visit_sym(ebnf::SymbolDeclaration* sym, const FilePos& pos) override {
			DeepSymVisitor deep_sym_visitor(this, pos);
			return sym->visit(&deep_sym_visitor);
		}
		
		T visit_expr(ebnf::SyntaxExpression* expr, const FilePos& pos) override {
			DeepExprVisitor deep_visitor(this, pos);
			SetterExprVisitor setter_expr_visitor(this, &deep_visitor);
			return expr->visit(&setter_expr_visitor);
		}
	};

	//
	//calculate_expression_property()
	//

	template<class T>
	void calculate_expression_property(
		EBNF_Builder* builder,
		const std::vector<ebnf::NonterminalDeclaration*>& nts,
		ExpressionPropertyAccessor<T>* property_accessor,
		ExpressionPropertyCalculator<T>* calculator)
	{
		//Phase 1: root calculation. The value of the property is calculated for all nonterminals and for
		//root expressions.
		RootPropertyCalculationContext<T> root_context(builder, property_accessor, calculator);
		for (ebnf::NonterminalDeclaration* nt : nts) root_context.calculate(nt);

		//Phase 2: deep calculation. The value of the property is calculated for the rest of expressions.
		DeepPropertyCalculationContext<T> deep_context(builder, property_accessor, calculator);
		for (ebnf::NonterminalDeclaration* nt : nts) deep_context.calculate(nt);
	}

}

#endif//SYN_CORE_EBNF_BLD_PROPERTY_H_INCLUDED
