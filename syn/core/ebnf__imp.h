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

//Implementation of EBNF grammar-related template functions.

#ifndef SYN_CORE_EBNF_IMP_H_INCLUDED
#define SYN_CORE_EBNF_IMP_H_INCLUDED

#include "ebnf__def.h"
#include "ebnf_visitor__def.h"

//
//Declaration
//

template<class T>
T synbin::ebnf::Declaration::visit(DeclarationVisitor<T>* visitor) {
	struct AdaptingVisitor : public DeclarationVisitor<void> {
		DeclarationVisitor<T>* m_effective_visitor;
		T m_value;
	
		void visit_Declaration(ebnf::Declaration* declaration) override {
			m_value = m_effective_visitor->visit_Declaration(declaration);
		}
		void visit_TypeDeclaration(ebnf::TypeDeclaration* declaration) override {
			m_value = m_effective_visitor->visit_TypeDeclaration(declaration);
		}
		void visit_CustomTerminalTypeDeclaration(ebnf::CustomTerminalTypeDeclaration* declaration) override {
			m_value = m_effective_visitor->visit_CustomTerminalTypeDeclaration(declaration);
		}
		void visit_SymbolDeclaration(ebnf::SymbolDeclaration* sym) override {
			m_value = m_effective_visitor->visit_SymbolDeclaration(sym);
		}
		void visit_TerminalDeclaration(ebnf::TerminalDeclaration* tr) override {
			m_value = m_effective_visitor->visit_TerminalDeclaration(tr);
		}
		void visit_NonterminalDeclaration(ebnf::NonterminalDeclaration* nt) override {
			m_value = m_effective_visitor->visit_NonterminalDeclaration(nt);
		}
	};
	
	AdaptingVisitor adapting_visitor;
	adapting_visitor.m_effective_visitor = visitor;
	visit0(&adapting_visitor);
	
	return adapting_visitor.m_value;
}

//
//SymbolDeclaration
//

template<class T>
T synbin::ebnf::SymbolDeclaration::visit(SymbolDeclarationVisitor<T>* visitor) {
	struct AdaptingVisitor : public SymbolDeclarationVisitor<void> {
		SymbolDeclarationVisitor<T>* m_effective_visitor;
		T m_value;
	
		void visit_SymbolDeclaration(ebnf::SymbolDeclaration* sym) override {
			m_value = m_effective_visitor->visit_SymbolDeclaration(sym);
		}
		void visit_TerminalDeclaration(ebnf::TerminalDeclaration* tr) override {
			m_value = m_effective_visitor->visit_TerminalDeclaration(tr);
		}
		void visit_NonterminalDeclaration(ebnf::NonterminalDeclaration* nt) override {
			m_value = m_effective_visitor->visit_NonterminalDeclaration(nt);
		}
	};
	
	AdaptingVisitor adapting_visitor;
	adapting_visitor.m_effective_visitor = visitor;
	visit0(&adapting_visitor);
	
	return adapting_visitor.m_value;
}

//
//SyntaxExpression
//

template<class T>
T synbin::ebnf::SyntaxExpression::visit(SyntaxExpressionVisitor<T>* visitor) {
	struct AdaptingVisitor : public SyntaxExpressionVisitor<void> {
		SyntaxExpressionVisitor<T>* m_effective_visitor;
		T m_value;

		void visit_SyntaxExpression(ebnf::SyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_SyntaxExpression(expr);
		}
		void visit_EmptySyntaxExpression(ebnf::EmptySyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_EmptySyntaxExpression(expr);
		}
		void visit_CompoundSyntaxExpression(ebnf::CompoundSyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_CompoundSyntaxExpression(expr);
		}
		void visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) override {
			m_value = m_effective_visitor->visit_SyntaxOrExpression(expr);
		}
		void visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override {
			m_value = m_effective_visitor->visit_SyntaxAndExpression(expr);
		}
		void visit_SyntaxElement(ebnf::SyntaxElement* expr) override {
			m_value = m_effective_visitor->visit_SyntaxElement(expr);
		}
		void visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) override {
			m_value = m_effective_visitor->visit_NameSyntaxElement(expr);
		}
		void visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) override {
			m_value = m_effective_visitor->visit_ThisSyntaxElement(expr);
		}
		void visit_NameSyntaxExpression(ebnf::NameSyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_NameSyntaxExpression(expr);
		}
		void visit_StringSyntaxExpression(ebnf::StringSyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_StringSyntaxExpression(expr);
		}
		void visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_CastSyntaxExpression(expr);
		}
		void visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_ZeroOneSyntaxExpression(expr);
		}
		void visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_LoopSyntaxExpression(expr);
		}
		void visit_ZeroManySyntaxExpression(ebnf::ZeroManySyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_ZeroManySyntaxExpression(expr);
		}
		void visit_OneManySyntaxExpression(ebnf::OneManySyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_OneManySyntaxExpression(expr);
		}
		void visit_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) override {
			m_value = m_effective_visitor->visit_ConstSyntaxExpression(expr);
		}
	};
	
	AdaptingVisitor adapting_visitor;
	adapting_visitor.m_effective_visitor = visitor;
	visit0(&adapting_visitor);
	
	return adapting_visitor.m_value;
}

//
//ConstExpression
//

template<class T>
T synbin::ebnf::ConstExpression::visit(ConstExpressionVisitor<T>* visitor) const {
	struct AdaptingVisitor : public ConstExpressionVisitor<void> {
		ConstExpressionVisitor<T>* m_effective_visitor;
		T m_value;

		void visit_ConstExpression(const ebnf::ConstExpression* expr) override {
			m_value = m_effective_visitor->visit_ConstExpression(expr);
		}
		void visit_IntegerConstExpression(const ebnf::IntegerConstExpression* expr) override {
			m_value = m_effective_visitor->visit_IntegerConstExpression(expr);
		}
		void visit_StringConstExpression(const ebnf::StringConstExpression* expr) override {
			m_value = m_effective_visitor->visit_StringConstExpression(expr);
		}
		void visit_BooleanConstExpression(const ebnf::BooleanConstExpression* expr) override {
			m_value = m_effective_visitor->visit_BooleanConstExpression(expr);
		}
		void visit_NativeConstExpression(const ebnf::NativeConstExpression* expr) override {
			m_value = m_effective_visitor->visit_NativeConstExpression(expr);
		}
	};
	
	AdaptingVisitor adapting_visitor;
	adapting_visitor.m_effective_visitor = visitor;
	visit0(&adapting_visitor);
	
	return adapting_visitor.m_value;
}

#endif//SYN_CORE_EBNF_IMP_H_INCLUDED
