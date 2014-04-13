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

//Implementation of EBNF grammar classes.

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "commons.h"
#include "ebnf__imp.h"
#include "ebnf_visitor__imp.h"
#include "ebnf_builder.h"
#include "primitives.h"
#include "types.h"
#include "util.h"
#include "util_mptr.h"

//TODO Check that the expression used as a loop separator is appropriate (e. g. it must not produce objects,
//since this makes no sense).

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

const ns::syntax_string ns::g_empty_syntax_string;

bool ns::raise_error(const FilePos& pos, const std::string& message) {
	throw ns::TextException(message, pos);
}

bool ns::raise_error(const syntax_string& cause, const std::string& message) {
	throw ns::TextException(message, cause.pos());
}

namespace {

	using std::unique_ptr;
	using util::MPtr;

	//
	//ListPrinter
	//

	class ListPrinter {
		std::ostream& m_out;
		const char* const m_second_prefix;
		const char* const m_end;
		const char* m_current_prefix;

	public:
		ListPrinter(std::ostream& out, const char* second_prefix, bool parentheses)
			: m_out(out),
			m_second_prefix(second_prefix),
			m_current_prefix(""),
			m_end(parentheses ? ")" : "")
		{
			m_out << (parentheses ? "(" : "");
		}

		~ListPrinter() {
			m_out << m_end;
		}

		void print_prefix() {
			m_out << m_current_prefix;
			m_current_prefix = m_second_prefix;
		}

		void print(const char* str) {
			print_prefix();
			m_out << str;
		}
	};

	void print_str_literal(std::ostream& out, const std::string& str) {
		for (std::size_t i = 0, n = str.length(); i < n; ++i) {
			char c = str[i];
			if (c == '"' || c == '\'' || c == '\\') {
				out << '\\';
				out << c;
			} else if (c == '\r') {
				out << "\\r";
			} else if (c == '\n') {
				out << "\\n";
			} else if (c == '\t') {
				out << "\\t";
			} else {
				out << c;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////
//Base Classes
/////////////////////////////////////////////////////////////////////

//
//Grammar
//

ebnf::Grammar::Grammar(MPtr<const std::vector<MPtr<Declaration>>> declarations)
	: m_syn_declarations(declarations)
{
	for (MPtr<Declaration> decl : *m_syn_declarations) decl->enumerate(m_terminals, m_nonterminals);
}

void ebnf::Grammar::print(std::ostream& out) const {
	for (MPtr<Declaration> decl : *m_syn_declarations) decl->print(out);
}

//
//RegularDeclaration
//

ebnf::RegularDeclaration::RegularDeclaration(const ns::syntax_string& name)
	: m_syn_name(name)
{}

//
//TypeDeclaration
//

ebnf::TypeDeclaration::TypeDeclaration(const ns::syntax_string& name)
: RegularDeclaration(name)
{}

void ebnf::TypeDeclaration::visit0(ns::DeclarationVisitor<void>* visitor) {
	visitor->visit_TypeDeclaration(this);
}

void ebnf::TypeDeclaration::print(std::ostream& out) const {
	out << "type " << get_name() << ";\n\n";
}

//
//SymbolDeclaration
//

ebnf::SymbolDeclaration::SymbolDeclaration(const ns::syntax_string& name)
: RegularDeclaration(name)
{}

void ebnf::SymbolDeclaration::visit0(ns::DeclarationVisitor<void>* visitor) {
	visit0(static_cast<SymbolDeclarationVisitor<void>*>(visitor));
}

//
//TerminalDeclaration
//

ebnf::TerminalDeclaration::TerminalDeclaration(const ns::syntax_string& name, MPtr<const RawType> raw_type)
	: SymbolDeclaration(name),
	m_syn_raw_type(raw_type),
	m_type(nullptr),
	m_tr_index(std::numeric_limits<std::size_t>::max())
{}

void ebnf::TerminalDeclaration::enumerate(
	std::vector<TerminalDeclaration*>& terminals,
	std::vector<NonterminalDeclaration*>& nonterminals)
{
	m_tr_index = terminals.size();
	terminals.push_back(this);
}

void ebnf::TerminalDeclaration::visit0(ns::SymbolDeclarationVisitor<void>* visitor) {
	visitor->visit_TerminalDeclaration(this);
}

void ebnf::TerminalDeclaration::print(std::ostream& out) const {
	out << "token " << get_name();
	if (m_syn_raw_type.get()) {
		out << " ";
		m_syn_raw_type->print(out);
	}
	out << ";\n\n";
}

//
//NonterminalDeclaration
//

ebnf::NonterminalDeclaration::NonterminalDeclaration(
	bool start,
	const ns::syntax_string& name,
	MPtr<SyntaxExpression> expression,
	MPtr<const RawType> explicit_raw_type)
	: SymbolDeclaration(name),
	m_syn_start(start),
	m_syn_expression(expression),
	m_syn_explicit_raw_type(explicit_raw_type),
	m_explicit_type(),
	m_nt_index(std::numeric_limits<std::size_t>::max())
{}

void ebnf::NonterminalDeclaration::install_extension(std::unique_ptr<ns::NonterminalDeclarationExtension> extension) {
	assert(extension.get());
	assert(!m_extension.get());
	m_extension = std::move(extension);
}

ns::NonterminalDeclarationExtension* ebnf::NonterminalDeclaration::get_extension() const {
	assert(m_extension.get());
	return m_extension.get();
}

void ebnf::NonterminalDeclaration::visit0(ns::SymbolDeclarationVisitor<void>* visitor) {
	visitor->visit_NonterminalDeclaration(this);
}

void ebnf::NonterminalDeclaration::enumerate(
	std::vector<TerminalDeclaration*>& terminals,
	std::vector<NonterminalDeclaration*>& nonterminals)
{
	m_nt_index = nonterminals.size();
	nonterminals.push_back(this);
}

void ebnf::NonterminalDeclaration::print(std::ostream& out) const {
	if (m_syn_start) out << "@";

	out << get_name();
	if (m_syn_explicit_raw_type.get()) {
		out << " ";
		m_syn_explicit_raw_type->print(out);
	}
	out << "\n\t: ";
	m_syn_expression->print(out, ebnf::PRIOR_NT);
	out << "\n\t;\n\n";
}

//
//CustomTerminalTypeDeclaration
//

ebnf::CustomTerminalTypeDeclaration::CustomTerminalTypeDeclaration(MPtr<const RawType> raw_type)
	: m_syn_raw_type(raw_type)
{
	assert(raw_type.get());
}

void ebnf::CustomTerminalTypeDeclaration::visit0(ns::DeclarationVisitor<void>* visitor) {
	visitor->visit_CustomTerminalTypeDeclaration(this);
}

void ebnf::CustomTerminalTypeDeclaration::print(std::ostream& out) const {
	out << "token \"\" ";
	m_syn_raw_type->print(out);
	out << ";\n\n";
}

//
//RawType
//

ebnf::RawType::RawType(const ns::syntax_string& name)
	: m_syn_name(name)
{}

void ebnf::RawType::print(std::ostream& out) const {
	out << "{" << m_syn_name << "}";
}

/////////////////////////////////////////////////////////////////////
//Syntax Expressions
/////////////////////////////////////////////////////////////////////

//
//SyntaxExpression
//

void ebnf::SyntaxExpression::install_extension(std::unique_ptr<ns::SyntaxExpressionExtension> extension) {
	assert(extension.get());
	assert(!m_extension.get());
	m_extension = std::move(extension);
}

ns::SyntaxExpressionExtension* ebnf::SyntaxExpression::get_extension() const {
	assert(m_extension.get());
	return m_extension.get();
}

//
//EmptySyntaxExpression
//

void ebnf::EmptySyntaxExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_EmptySyntaxExpression(this);
}

void ebnf::EmptySyntaxExpression::print(std::ostream& out, SyntaxPriority priority) const {}

//
//CompoundSyntaxExpression
//

ebnf::CompoundSyntaxExpression::CompoundSyntaxExpression(
	util::MPtr<const std::vector<util::MPtr<SyntaxExpression>>> sub_expressions)
	: m_syn_sub_expressions(sub_expressions)
{}

//
//SyntaxOrExpression
//

ebnf::SyntaxOrExpression::SyntaxOrExpression(
	util::MPtr<const std::vector<util::MPtr<SyntaxExpression>>> sub_expressions)
	: CompoundSyntaxExpression(sub_expressions)
{}

void ebnf::SyntaxOrExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_SyntaxOrExpression(this);
}

void ebnf::SyntaxOrExpression::print(std::ostream& out, SyntaxPriority priority) const {
	bool top = PRIOR_NT == priority;
	const char* separator = top ? "\n\t| " : " | ";
	
	ListPrinter list_printer(out, separator, priority > PRIOR_OR);
	for (MPtr<ebnf::SyntaxExpression> expr : get_sub_expressions()) {
		list_printer.print_prefix();
		expr->print(out, PRIOR_OR);
	}
}

//
//SyntaxAndExpression
//

ebnf::SyntaxAndExpression::SyntaxAndExpression(
	util::MPtr<const std::vector<util::MPtr<SyntaxExpression>>> sub_expressions,
	util::MPtr<const RawType> raw_type)
	: CompoundSyntaxExpression(sub_expressions),
	m_syn_raw_type(raw_type),
	m_type(nullptr)
{}

void ebnf::SyntaxAndExpression::install_and_extension(std::unique_ptr<ns::SyntaxAndExpressionExtension> and_extension) {
	assert(and_extension.get());
	assert(!m_and_extension.get());
	m_and_extension = std::move(and_extension);
}

ns::SyntaxAndExpressionExtension* ebnf::SyntaxAndExpression::get_and_extension() const {
	assert(m_and_extension.get());
	return m_and_extension.get();
}

void ebnf::SyntaxAndExpression::set_type(const types::ClassType* type) {
	assert(!m_type);
	assert(type);
	m_type = type;
}

void ebnf::SyntaxAndExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_SyntaxAndExpression(this);
}

void ebnf::SyntaxAndExpression::print(std::ostream& out, SyntaxPriority priority) const {
	ListPrinter list_printer(out, " ", (priority > PRIOR_AND) || (priority == PRIOR_AND && m_syn_raw_type.get()));
	for (MPtr<ebnf::SyntaxExpression> expr : get_sub_expressions()) {
		list_printer.print_prefix();
		expr->print(out, PRIOR_AND);
	}
	if (m_syn_raw_type.get()) {
		list_printer.print_prefix();
		m_syn_raw_type->print(out);
	}
}

//
//SyntaxElement
//

ebnf::SyntaxElement::SyntaxElement(MPtr<SyntaxExpression> expression)
	: m_syn_expression(expression)
{}

//
//NameSyntaxElement
//

ebnf::NameSyntaxElement::NameSyntaxElement(MPtr<SyntaxExpression> expression, const ns::syntax_string& name)
	: SyntaxElement(expression), m_syn_name(name)
{}

void ebnf::NameSyntaxElement::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_NameSyntaxElement(this);
}

void ebnf::NameSyntaxElement::print(std::ostream& out, SyntaxPriority priority) const {
	out << m_syn_name << "=";
	get_expression()->print(out, PRIOR_TERM);
}

//
//ThisSyntaxElement
//

ebnf::ThisSyntaxElement::ThisSyntaxElement(const ns::FilePos& pos, MPtr<SyntaxExpression> expression)
	: SyntaxElement(expression), m_syn_pos(pos)
{}

void ebnf::ThisSyntaxElement::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_ThisSyntaxElement(this);
}

const ns::FilePos& ebnf::ThisSyntaxElement::get_pos() const {
	return m_syn_pos;
}

void ebnf::ThisSyntaxElement::print(std::ostream& out, SyntaxPriority priority) const {
	out << "this=";
	get_expression()->print(out, PRIOR_TERM);
}

//
//NameSyntaxExpression
//

ebnf::NameSyntaxExpression::NameSyntaxExpression(const ns::syntax_string& name)
	: m_syn_name(name), m_sym(nullptr)
{}

void ebnf::NameSyntaxExpression::set_sym(ebnf::SymbolDeclaration* sym) {
	assert(!m_sym);
	assert(sym);
	m_sym = sym;
}

void ebnf::NameSyntaxExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_NameSyntaxExpression(this);
}

void ebnf::NameSyntaxExpression::print(std::ostream& out, SyntaxPriority priority) const {
	out << m_syn_name;
}

//
//StringSyntaxExpression
//

ebnf::StringSyntaxExpression::StringSyntaxExpression(const ns::syntax_string& string)
	: m_syn_string(string)
{}

void ebnf::StringSyntaxExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_StringSyntaxExpression(this);
}

void ebnf::StringSyntaxExpression::print(std::ostream& out, SyntaxPriority priority) const {
	out << '"';
	print_str_literal(out, m_syn_string.str());
	out << '"';
}

//
//CastSyntaxExpression
//

ebnf::CastSyntaxExpression::CastSyntaxExpression(MPtr<const RawType> raw_type, MPtr<SyntaxExpression> expression)
	: m_syn_raw_type(raw_type),
	m_syn_expression(expression),
	m_type()
{}

void ebnf::CastSyntaxExpression::set_type(MPtr<const types::Type> type) {
	assert(!m_type.get());
	assert(type.get());
	m_type = type;
}

void ebnf::CastSyntaxExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_CastSyntaxExpression(this);
}

void ebnf::CastSyntaxExpression::print(std::ostream& out, SyntaxPriority priority) const {
	m_syn_raw_type->print(out);
	out << "(";
	m_syn_expression->print(out, PRIOR_TOP);
	out << ")";
}

//
//ZeroOneSyntaxExpression
//

ebnf::ZeroOneSyntaxExpression::ZeroOneSyntaxExpression(MPtr<SyntaxExpression> sub_expression)
	: m_syn_sub_expression(sub_expression)
{}

void ebnf::ZeroOneSyntaxExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_ZeroOneSyntaxExpression(this);
}

void ebnf::ZeroOneSyntaxExpression::print(std::ostream& out, SyntaxPriority priority) const {
	m_syn_sub_expression->print(out, PRIOR_TERM);
	out << "?";
}

//
//LoopSyntaxExpression
//

ebnf::LoopSyntaxExpression::LoopSyntaxExpression(MPtr<LoopBody> body)
	: m_syn_body(body)
{}

//
//ZeroManySyntaxExpression
//

ebnf::ZeroManySyntaxExpression::ZeroManySyntaxExpression(MPtr<LoopBody> body)
	: LoopSyntaxExpression(body)
{}

void ebnf::ZeroManySyntaxExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_ZeroManySyntaxExpression(this);
}

void ebnf::ZeroManySyntaxExpression::print(std::ostream& out, SyntaxPriority priority) const {
	get_body()->print(out);
	out << "*";
}

//
//OneManySyntaxExpression
//

ebnf::OneManySyntaxExpression::OneManySyntaxExpression(MPtr<LoopBody> body)
	: LoopSyntaxExpression(body)
{}

void ebnf::OneManySyntaxExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_OneManySyntaxExpression(this);
}

void ebnf::OneManySyntaxExpression::print(std::ostream& out, SyntaxPriority priority) const {
	get_body()->print(out);
	out << "+";
}

//
//LoopBody
//

ebnf::LoopBody::LoopBody(
	MPtr<SyntaxExpression> expression,
	MPtr<SyntaxExpression> separator,
	const ns::FilePos& separator_pos)
	: m_syn_expression(expression),
	m_syn_separator(separator),
	m_syn_separator_pos(separator_pos)
{
	assert(m_syn_expression.get());
}

void ebnf::LoopBody::print(std::ostream& out) const {
	out << "(";
	m_syn_expression->print(out, PRIOR_TOP);
	if (m_syn_separator.get()) {
		out << " : ";
		m_syn_separator->print(out, PRIOR_TOP);
	}
	out << ")";
}

//
//ConstSyntaxExpression
//

ebnf::ConstSyntaxExpression::ConstSyntaxExpression(MPtr<const ConstExpression> expression)
	: m_syn_expression(expression)
{}

void ebnf::ConstSyntaxExpression::visit0(ns::SyntaxExpressionVisitor<void>* visitor) {
	visitor->visit_ConstSyntaxExpression(this);
}

void ebnf::ConstSyntaxExpression::print(std::ostream& out, SyntaxPriority priority) const {
	out << "<";
	m_syn_expression->print(out);
	out << ">";
}

/////////////////////////////////////////////////////////////////////
//Constant Expressions
/////////////////////////////////////////////////////////////////////

//
//IntegerConstExpression
//

ebnf::IntegerConstExpression::IntegerConstExpression(ns::syntax_number value)
	: m_syn_value(value)
{}

void ebnf::IntegerConstExpression::visit0(ns::ConstExpressionVisitor<void>* visitor) const {
	visitor->visit_IntegerConstExpression(this);
}

void ebnf::IntegerConstExpression::print(std::ostream& out) const {
	out << m_syn_value;
}

//
//StringConstExpression
//

ebnf::StringConstExpression::StringConstExpression(const ns::syntax_string& value)
	: m_syn_value(value)
{}

void ebnf::StringConstExpression::visit0(ns::ConstExpressionVisitor<void>* visitor) const {
	visitor->visit_StringConstExpression(this);
}

void ebnf::StringConstExpression::print(std::ostream& out) const {
	out << '"';
	print_str_literal(out, m_syn_value.str());
	out << '"';
}

//
//BooleanConstExpression
//

ebnf::BooleanConstExpression::BooleanConstExpression(bool value)
	: m_syn_value(value)
{}

void ebnf::BooleanConstExpression::visit0(ns::ConstExpressionVisitor<void>* visitor) const {
	visitor->visit_BooleanConstExpression(this);
}

void ebnf::BooleanConstExpression::print(std::ostream& out) const {
	out << (m_syn_value ? "true" : "false");
}

//
//NativeConstExpression
//

ebnf::NativeConstExpression::NativeConstExpression(
	MPtr<const std::vector<ns::syntax_string>> qualifiers,
	MPtr<const NativeName> name,
	MPtr<const std::vector<MPtr<const NativeReference>>> references)
	: m_syn_qualifiers(qualifiers),
	m_syn_name(name),
	m_syn_references(references)
{}

void ebnf::NativeConstExpression::visit0(ns::ConstExpressionVisitor<void>* visitor) const {
	visitor->visit_NativeConstExpression(this);
}

void ebnf::NativeConstExpression::print(std::ostream& out) const {
	for (const ns::syntax_string& q : *m_syn_qualifiers) out << q << "::";
	m_syn_name->print(out);
	for (MPtr<const NativeReference> ref : *m_syn_references) ref->print(out);
}

//
//NativeName
//

ebnf::NativeName::NativeName(const ns::syntax_string& name)
	: m_syn_name(name)
{}

//
//NativeVariableName
//

ebnf::NativeVariableName::NativeVariableName(const ns::syntax_string& name)
	: NativeName(name)
{}

void ebnf::NativeVariableName::print(std::ostream& out) const {
	out << get_name();
}

//
//NativeFunctionName
//

ebnf::NativeFunctionName::NativeFunctionName(
	const ns::syntax_string& name,
	util::MPtr<const std::vector<util::MPtr<const ConstExpression>>> arguments)
	: NativeName(name), m_syn_arguments(arguments)
{}

void ebnf::NativeFunctionName::print(std::ostream& out) const {
	out << get_name() << "(";
	ListPrinter list_printer(out, ", ", false);
	for (MPtr<const ConstExpression> expr : *m_syn_arguments) {
		list_printer.print_prefix();
		expr->print(out);
	}
	out << ")";
}

//
//NativeReference
//

ebnf::NativeReference::NativeReference(MPtr<const NativeName> name)
	: m_syn_name(name)
{}

//
//NativePointerReference
//

ebnf::NativePointerReference::NativePointerReference(MPtr<const NativeName> name)
	: NativeReference(name)
{}

void ebnf::NativePointerReference::print(std::ostream& out) const {
	out << "->";
	get_native_name()->print(out);
}

//
//NativeReferenceReference
//

ebnf::NativeReferenceReference::NativeReferenceReference(MPtr<const NativeName> name)
	: NativeReference(name)
{}

void ebnf::NativeReferenceReference::print(std::ostream& out) const {
	out << ".";
	get_native_name()->print(out);
}
