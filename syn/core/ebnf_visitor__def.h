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

//EBNF grammar visitor classes definitions.

#ifndef SYN_CORE_EBNF_VISITOR_DEF_H_INCLUDED
#define SYN_CORE_EBNF_VISITOR_DEF_H_INCLUDED

#include "ebnf__dec.h"
#include "ebnf_extension__dec.h"
#include "ebnf_visitor__dec.h"
#include "noncopyable.h"

namespace synbin {

	//
	//SymbolDeclarationVisitor
	//

	template<class T>
	class SymbolDeclarationVisitor {
		NONCOPYABLE(SymbolDeclarationVisitor);

	public:
		SymbolDeclarationVisitor(){}
		virtual ~SymbolDeclarationVisitor(){}

		virtual T visit_SymbolDeclaration(ebnf::SymbolDeclaration* sym);
		virtual T visit_TerminalDeclaration(ebnf::TerminalDeclaration* tr);
		virtual T visit_NonterminalDeclaration(ebnf::NonterminalDeclaration* nt);
	};

	//
	//DeclarationVisitor
	//

	template<class T>
	class DeclarationVisitor : public SymbolDeclarationVisitor<T> {
		NONCOPYABLE(DeclarationVisitor);

	public:
		DeclarationVisitor(){}

		virtual T visit_Declaration(ebnf::Declaration* declaration);
		virtual T visit_TypeDeclaration(ebnf::TypeDeclaration* declaration);
		virtual T visit_CustomTerminalTypeDeclaration(ebnf::CustomTerminalTypeDeclaration* declaration);
		T visit_SymbolDeclaration(ebnf::SymbolDeclaration* declaration);
	};

	//
	//SyntaxExpressionVisitor
	//

	template<class T>
	class SyntaxExpressionVisitor {
		NONCOPYABLE(SyntaxExpressionVisitor);

		static SyntaxExpressionVisitor<T> NullValue;

	public:
		static SyntaxExpressionVisitor<T>* const Null;

		SyntaxExpressionVisitor(){}
		virtual ~SyntaxExpressionVisitor(){}

		virtual T visit_SyntaxExpression(ebnf::SyntaxExpression* expr);
		virtual T visit_EmptySyntaxExpression(ebnf::EmptySyntaxExpression* expr);
		virtual T visit_CompoundSyntaxExpression(ebnf::CompoundSyntaxExpression* expr);
		virtual T visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr);
		virtual T visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr);
		virtual T visit_SyntaxElement(ebnf::SyntaxElement* expr);
		virtual T visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr);
		virtual T visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr);
		virtual T visit_NameSyntaxExpression(ebnf::NameSyntaxExpression* expr);
		virtual T visit_StringSyntaxExpression(ebnf::StringSyntaxExpression* expr);
		virtual T visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr);
		virtual T visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr);
		virtual T visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr);
		virtual T visit_ZeroManySyntaxExpression(ebnf::ZeroManySyntaxExpression* expr);
		virtual T visit_OneManySyntaxExpression(ebnf::OneManySyntaxExpression* expr);
		virtual T visit_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr);
	};

	//
	//ConstExpressionVisitor
	//

	template<class T>
	class ConstExpressionVisitor {
		NONCOPYABLE(ConstExpressionVisitor);

	public:
		ConstExpressionVisitor(){}
		virtual ~ConstExpressionVisitor(){}

		virtual T visit_ConstExpression(const ebnf::ConstExpression* expr);
		virtual T visit_IntegerConstExpression(const ebnf::IntegerConstExpression* expr);
		virtual T visit_StringConstExpression(const ebnf::StringConstExpression* expr);
		virtual T visit_BooleanConstExpression(const ebnf::BooleanConstExpression* expr);
		virtual T visit_NativeConstExpression(const ebnf::NativeConstExpression* expr);
	};

	//
	//AndExpressionMeaningVisitor
	//

	template<class T>
	class AndExpressionMeaningVisitor {
		NONCOPYABLE(AndExpressionMeaningVisitor);

	public:
		AndExpressionMeaningVisitor(){}
		virtual ~AndExpressionMeaningVisitor(){}

		virtual T visit_AndExpressionMeaning(AndExpressionMeaning* meaning);
		virtual T visit_VoidAndExpressionMeaning(VoidAndExpressionMeaning* meaning);
		virtual T visit_ThisAndExpressionMeaning(ThisAndExpressionMeaning* meaning);
		virtual T visit_ClassAndExpressionMeaning(ClassAndExpressionMeaning* meaning);
	};

}

#endif//SYN_CORE_EBNF_VISITOR_DEF_H_INCLUDED
