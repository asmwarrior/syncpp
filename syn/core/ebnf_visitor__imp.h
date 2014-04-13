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

//EBNF grammar visitor templates implementations.

#ifndef SYN_CORE_EBNF_VISITOR_IMP_H_INCLUDED
#define SYN_CORE_EBNF_VISITOR_IMP_H_INCLUDED

#include "ebnf_visitor__def.h"

//
//SymbolDeclarationVisitor
//

template<class T>
T synbin::SymbolDeclarationVisitor<T>::visit_SymbolDeclaration(ebnf::SymbolDeclaration* sym) {
	return T();
}

template<class T>
T synbin::SymbolDeclarationVisitor<T>::visit_TerminalDeclaration(ebnf::TerminalDeclaration* tr) {
	return visit_SymbolDeclaration(tr);
}

template<class T>
T synbin::SymbolDeclarationVisitor<T>::visit_NonterminalDeclaration(ebnf::NonterminalDeclaration* nt) {
	return visit_SymbolDeclaration(nt);
}

//
//DeclarationVisitor
//

template<class T>
T synbin::DeclarationVisitor<T>::visit_Declaration(ebnf::Declaration* declaration) {
	return T();
}

template<class T>
T synbin::DeclarationVisitor<T>::visit_TypeDeclaration(ebnf::TypeDeclaration* declaration) {
	return visit_Declaration(declaration);
}

template<class T>
T synbin::DeclarationVisitor<T>::visit_CustomTerminalTypeDeclaration(ebnf::CustomTerminalTypeDeclaration* declaration) {
	return visit_Declaration(declaration);
}

template<class T>
T synbin::DeclarationVisitor<T>::visit_SymbolDeclaration(ebnf::SymbolDeclaration* declaration) {
	return visit_Declaration(declaration);
}

//
//SyntaxExpressionVisitor
//

template<class T>
synbin::SyntaxExpressionVisitor<T> synbin::SyntaxExpressionVisitor<T>::NullValue;

template<class T>
synbin::SyntaxExpressionVisitor<T>* const synbin::SyntaxExpressionVisitor<T>::Null = &NullValue;

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_SyntaxExpression(ebnf::SyntaxExpression* expr) {
	return T();
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_EmptySyntaxExpression(ebnf::EmptySyntaxExpression* expr) {
	return visit_SyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_CompoundSyntaxExpression(ebnf::CompoundSyntaxExpression* expr) {
	return visit_SyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) {
	return visit_CompoundSyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) {
	return visit_CompoundSyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_SyntaxElement(ebnf::SyntaxElement* expr) {
	return visit_SyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) {
	return visit_SyntaxElement(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) {
	return visit_SyntaxElement(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_NameSyntaxExpression(ebnf::NameSyntaxExpression* expr) {
	return visit_SyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_StringSyntaxExpression(ebnf::StringSyntaxExpression* expr) {
	return visit_SyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) {
	return visit_SyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) {
	return visit_SyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) {
	return visit_SyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_ZeroManySyntaxExpression(ebnf::ZeroManySyntaxExpression* expr) {
	return visit_LoopSyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_OneManySyntaxExpression(ebnf::OneManySyntaxExpression* expr) {
	return visit_LoopSyntaxExpression(expr);
}

template<class T>
T synbin::SyntaxExpressionVisitor<T>::visit_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) {
	return visit_SyntaxExpression(expr);
}

//
//ConstExpressionVisitor
//

template<class T>
T synbin::ConstExpressionVisitor<T>::visit_ConstExpression(const ebnf::ConstExpression* expr) {
	return T();
}

template<class T>
T synbin::ConstExpressionVisitor<T>::visit_IntegerConstExpression(const ebnf::IntegerConstExpression* expr) {
	return visit_ConstExpression(expr);
}

template<class T>
T synbin::ConstExpressionVisitor<T>::visit_StringConstExpression(const ebnf::StringConstExpression* expr) {
	return visit_ConstExpression(expr);
}

template<class T>
T synbin::ConstExpressionVisitor<T>::visit_BooleanConstExpression(const ebnf::BooleanConstExpression* expr) {
	return visit_ConstExpression(expr);
}

template<class T>
T synbin::ConstExpressionVisitor<T>::visit_NativeConstExpression(const ebnf::NativeConstExpression* expr) {
	return visit_ConstExpression(expr);
}

//
//AndExpressionMeaningVisitor
//

template<class T>
T synbin::AndExpressionMeaningVisitor<T>::visit_AndExpressionMeaning(AndExpressionMeaning *meaning) {
	return T();
}

template<class T>
T synbin::AndExpressionMeaningVisitor<T>::visit_VoidAndExpressionMeaning(VoidAndExpressionMeaning *meaning) {
	return visit_AndExpressionMeaning(meaning);
}

template<class T>
T synbin::AndExpressionMeaningVisitor<T>::visit_ThisAndExpressionMeaning(ThisAndExpressionMeaning *meaning) {
	return visit_AndExpressionMeaning(meaning);
}

template<class T>
T synbin::AndExpressionMeaningVisitor<T>::visit_ClassAndExpressionMeaning(ClassAndExpressionMeaning *meaning) {
	return visit_AndExpressionMeaning(meaning);
}

#endif//SYN_CORE_EBNF_VISITOR_IMP_H_INCLUDED
