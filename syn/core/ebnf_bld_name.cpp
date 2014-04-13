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

//Part of the EBNF Grammar Builder responsible for names resolution and verification.

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
#include "ebnf_visitor__imp.h"
#include "types.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

namespace {

	using util::MPtr;

	//
	//NameRegisteringDeclarationVisitor
	//

	class NameRegisteringDeclarationVisitor : public ns::DeclarationVisitor<void> {
		NONCOPYABLE(NameRegisteringDeclarationVisitor);

		ns::EBNF_Builder* const m_builder;

	public:
		NameRegisteringDeclarationVisitor(ns::EBNF_Builder* builder)
			: m_builder(builder)
		{}

		void visit_TypeDeclaration(ebnf::TypeDeclaration* typedecl) override {
			m_builder->register_type_declaration(typedecl);
		}

		void visit_TerminalDeclaration(ebnf::TerminalDeclaration* tr) override {
			m_builder->register_tr_declaration(tr);
		}
		
		void visit_NonterminalDeclaration(ebnf::NonterminalDeclaration* nt) override {
			m_builder->register_nt_declaration(nt);
		}

		void visit_CustomTerminalTypeDeclaration(ebnf::CustomTerminalTypeDeclaration* declaration) override {
			m_builder->register_custom_terminal_type_declaration(declaration);
		}
	};

	//
	//NameResolvingSyntaxExpressionVisitor
	//

	class NameResolvingSyntaxExpressionVisitor : public ns::SyntaxExpressionVisitor<void> {
		NONCOPYABLE(NameResolvingSyntaxExpressionVisitor);

		ns::EBNF_Builder* const m_builder;

	public:
		NameResolvingSyntaxExpressionVisitor(ns::EBNF_Builder* builder)
			: m_builder(builder)
		{}

		void visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override {
			const ebnf::RawType* raw_type = expr->get_raw_type();
			if (!raw_type) return;

			const ns::syntax_string& name = raw_type->get_name();
			MPtr<const types::Type> type = m_builder->resolve_type_name(name);
			const types::ClassType* class_type = type->as_class();
			if (!class_type) {
				throw ns::raise_error(name, "'" + name.str() +
					"' is not a class type, it cannot be used as a production type");
			}
			expr->set_type(class_type);
		}

		void visit_NameSyntaxExpression(ebnf::NameSyntaxExpression* expr) override {
			const ns::syntax_string& name = expr->get_name();
			ebnf::SymbolDeclaration* sym = m_builder->resolve_symbol_name(name);
			expr->set_sym(sym);
		}

		void visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) override {
			const ebnf::RawType* raw_type = expr->get_raw_type();
			const ns::syntax_string& type_name = raw_type->get_name();
			MPtr<const types::Type> type = m_builder->resolve_type_name(type_name);
			expr->set_type(type);
		}
	};

	void resolve_nonterminal_declaration_names(ns::EBNF_Builder* builder, ebnf::NonterminalDeclaration* nt) {
		const ebnf::RawType* raw_type = nt->get_explicit_raw_type();
		if (raw_type) {
			const ns::syntax_string& type_name = raw_type->get_name();
			MPtr<const types::Type> type = builder->resolve_type_name(type_name);
			nt->set_explicit_type(type);
		}

		NameResolvingSyntaxExpressionVisitor effective_visitor(builder);
		ns::RecursiveSyntaxExpressionVisitor recursive_visitor(&effective_visitor);
		ebnf::SyntaxExpression* expression = nt->get_expression();
		expression->visit(&recursive_visitor);
	}

}

//
//EBNF_Builder
//

void ns::EBNF_Builder::register_names() {
	assert(m_install_extensions_completed);
	assert(!m_register_names_completed);

	NameRegisteringDeclarationVisitor visitor(this);
	for (MPtr<ebnf::Declaration> decl : m_grammar->get_declarations()) decl->visit(&visitor);
	
	m_register_names_completed = true;
}

void ns::EBNF_Builder::resolve_name_references() {
	assert(m_register_names_completed);
	assert(!m_resolve_name_references_completed);

	typedef void (ebnf::Declaration::*visit_fn_t)(DeclarationVisitor<void>* visitor);
	visit_fn_t visit_fn = &ebnf::Declaration::visit;

	for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) {
		resolve_nonterminal_declaration_names(this, nt);
	}

	m_resolve_name_references_completed = true;
}
