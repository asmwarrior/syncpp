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

//Hepler classes used by the EBNF Builder.

#ifndef SYN_CORE_EBNF_BLD_COMMON_H_INCLUDED
#define SYN_CORE_EBNF_BLD_COMMON_H_INCLUDED

#include "ebnf__imp.h"
#include "ebnf_extension__imp.h"
#include "ebnf_visitor__imp.h"
#include "noncopyable.h"
#include "util_mptr.h"

namespace synbin {

	//
	//SubSyntaxExpressionVisitor
	//

	//Expression visitor which enumerates direct sub-expressions.
	class SubSyntaxExpressionVisitor : public SyntaxExpressionVisitor<void> {
		NONCOPYABLE(SubSyntaxExpressionVisitor);

		SyntaxExpressionVisitor<void>* const m_effective_visitor;

	public:
		explicit SubSyntaxExpressionVisitor(SyntaxExpressionVisitor<void>* effective_visitor)
			: m_effective_visitor(effective_visitor ? effective_visitor : SyntaxExpressionVisitor<void>::Null)
		{}

		void visit_CompoundSyntaxExpression(ebnf::CompoundSyntaxExpression* expr) override {
			const std::vector<util::MPtr<ebnf::SyntaxExpression>>& sub_expressions = expr->get_sub_expressions();
			ebnf::SyntaxExpression::visit_all(sub_expressions, m_effective_visitor);
		}
		
		void visit_SyntaxElement(ebnf::SyntaxElement* expr) override {
			expr->get_expression()->visit(m_effective_visitor);
		}
		
		void visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) override {
			expr->get_expression()->visit(m_effective_visitor);
		}
		
		void visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override {
			expr->get_sub_expression()->visit(m_effective_visitor);
		}
		
		void visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) override {
			const ebnf::LoopBody* body = expr->get_body();
			body->get_expression()->visit(m_effective_visitor);
			if (body->get_separator()) {
				body->get_separator()->visit(m_effective_visitor);
			}
		}
	};

	//
	//RecursiveSyntaxExpressionVisitor
	//

	//Expression visitor that enumerates all sub-expressions (direct and indirect).
	class RecursiveSyntaxExpressionVisitor : public SyntaxExpressionVisitor<void> {
		NONCOPYABLE(RecursiveSyntaxExpressionVisitor);

		SubSyntaxExpressionVisitor m_sub_visitor;
		SyntaxExpressionVisitor<void>* const m_effective_visitor;

	public:
		explicit RecursiveSyntaxExpressionVisitor(SyntaxExpressionVisitor<void>* effective_visitor)
			: m_sub_visitor(this),
			m_effective_visitor(effective_visitor)
		{}

		void visit_SyntaxExpression(ebnf::SyntaxExpression* expr) override {
			expr->visit(m_effective_visitor);
			expr->visit(&m_sub_visitor);
		}
	};

}

#endif//SYN_CORE_EBNF_BLD_COMMON_H_INCLUDED
