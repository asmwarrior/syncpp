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

//Part of EBNF Grammar Builder responsible for recursion detection.

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
#include "util.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace util = ns::util;

using util::MPtr;

namespace {

	//
	//NtIndexFn
	//

	class NtIndexFn : public std::unary_function<ebnf::NonterminalDeclaration*, std::size_t> {
	public:
		std::size_t operator()(const ebnf::NonterminalDeclaration* nt) const {
			return nt->nt_index();
		}
	};

	//
	//RecursionVerificationVisitor
	//

	typedef util::IndexedSet<ebnf::NonterminalDeclaration*, NtIndexFn> NtIndexedSet;

	class RecursionVerificationVisitor : public ns::SyntaxExpressionVisitor<void> {
		NONCOPYABLE(RecursionVerificationVisitor);

		NtIndexedSet m_nt_path;
		NtIndexedSet::const_iterator m_loop_pos;
		ns::SubSyntaxExpressionVisitor m_sub_visitor;

	public:
		RecursionVerificationVisitor(MPtr<const ebnf::Grammar> grammar)
			: m_nt_path(grammar->get_nonterminals().size()),
			m_sub_visitor(this)
		{
			m_loop_pos = m_nt_path.begin();
		}

		void visit2_Nt(ebnf::NonterminalDeclaration* nt) {
			if (m_nt_path.add(nt)) {
				ebnf::SyntaxExpression* expr = nt->get_expression();
				expr->visit(this);
				m_nt_path.remove(nt);
			} else {
				NtIndexedSet::const_iterator pos = m_nt_path.find(nt);
				assert(pos < m_nt_path.end());
				if (pos < m_loop_pos) {
					std::string msg = "Recursion through loop:";
					for (ebnf::NonterminalDeclaration* cur_nt : m_nt_path) msg += " " + cur_nt->get_name().str();
					throw ns::raise_error(nt->get_name(), msg);
				}
			}
		}

		void visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) override {
			expr->visit(&m_sub_visitor);
		}
		
		void visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override {
			class MeaningVisitor : public ns::AndExpressionMeaningVisitor<void> {
				RecursionVerificationVisitor* const m_main_visitor;
				ebnf::SyntaxAndExpression* const m_expr;
			public:
				MeaningVisitor(
					RecursionVerificationVisitor* main_visitor,
					ebnf::SyntaxAndExpression* expr)
					: m_main_visitor(main_visitor),
					m_expr(expr)
				{}

				void visit_ThisAndExpressionMeaning(ns::ThisAndExpressionMeaning* meaning) {
					ebnf::SyntaxExpression::visit_all(meaning->get_result_elements(), m_main_visitor);
				}
			};

			MeaningVisitor meaning_visitor(this, expr);
			expr->get_and_extension()->get_meaning()->visit(&meaning_visitor);
		}
		
		void visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) override {
			expr->visit(&m_sub_visitor);
		}
		
		void visit_NameSyntaxExpression(ebnf::NameSyntaxExpression* expr) override {
			ebnf::SymbolDeclaration* sym = expr->get_sym();
			ebnf::NonterminalDeclaration* nt = sym->as_nt();
			if (nt) visit2_Nt(nt);
		}
		
		void visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override {
			expr->visit(&m_sub_visitor);
		}
		
		void visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) override {
			if (!expr->get_extension()->is_void()) {
				//All nonterminals in the current path are behind the loop bounds.
				//If one of those nonterminals appears in the path again,
				//recursion-through-loop is detected.
				NtIndexedSet::const_iterator old_loop_pos = m_loop_pos;
				m_loop_pos = m_nt_path.end();
				ebnf::SyntaxExpression* sub_expr = expr->get_body()->get_expression();
				sub_expr->visit(this);
				m_loop_pos = old_loop_pos;
			}
		}
	};

}

//
//EBNF_Builder
//

void ns::EBNF_Builder::verify_recursion() {
	assert(m_calculate_is_void_completed);
	assert(!m_verify_recursion_completed);

	RecursionVerificationVisitor visitor(m_grammar);
	for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) visitor.visit2_Nt(nt);

	m_verify_recursion_completed = true;
}
