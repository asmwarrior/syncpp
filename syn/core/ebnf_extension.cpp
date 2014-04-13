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

//EBNF extension classes implementation.

#include <cassert>
#include <vector>

#include "ebnf__def.h"
#include "ebnf_extension__def.h"
#include "ebnf_visitor__def.h"
#include "types.h"
#include "util_mptr.h"
#include "util_string.h"

namespace ns = synbin;
namespace conv = ns::conv;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

using util::MPtr;
using util::String;

//
//AbstractExtension
//

void ns::AbstractExtension::set_is_void(bool is_void) {
	m_is_void.set(is_void);
}

bool ns::AbstractExtension::is_void_defined() const {
	return m_is_void.is_defined();
}

bool ns::AbstractExtension::is_void() const {
	return m_is_void.get();
}

void ns::AbstractExtension::set_general_type(ns::GeneralType general_type) {
	bool is_void = m_is_void.get();
	if (is_void) {
		assert(ns::GENERAL_TYPE_VOID == general_type);
	} else {
		assert(ns::GENERAL_TYPE_VOID != general_type);
	}
	m_general_type.set(general_type);
}

bool ns::AbstractExtension::general_type_defined() const {
	return m_general_type.is_defined();
}

ns::GeneralType ns::AbstractExtension::get_general_type() const {
	return m_general_type.get();
}

void ns::AbstractExtension::set_concrete_type(util::MPtr<const types::Type> concrete_type) {
	m_concrete_type.set(concrete_type);
}

bool ns::AbstractExtension::concrete_type_defined() const {
	return m_concrete_type.is_defined();
}

util::MPtr<const types::Type> ns::AbstractExtension::get_concrete_type() const {
	return m_concrete_type.get();
}

bool ns::AbstractExtension::is_concrete_type_set() const {
	return m_concrete_type.is_defined();
}

//
//NonterminalDeclarationExtension
//

ns::NonterminalDeclarationExtension::NonterminalDeclarationExtension()
	: m_visiting(false)
{}

bool ns::NonterminalDeclarationExtension::set_visiting(bool visiting) {
	bool old_visiting = m_visiting;
	m_visiting = visiting;
	return old_visiting;
}

void ns::NonterminalDeclarationExtension::set_class_type(MPtr<const types::ClassType> class_type) {
	m_class_type.set(class_type);
}

MPtr<const types::ClassType> ns::NonterminalDeclarationExtension::get_class_type() const {
	return m_class_type.get();
}

MPtr<const types::ClassType> ns::NonterminalDeclarationExtension::get_class_type_opt() const {
	return m_class_type.is_defined() ? m_class_type.get() : MPtr<const types::ClassType>();
}

//
//SyntaxExpressionExtension
//

void ns::SyntaxExpressionExtension::set_expected_type(MPtr<const types::Type> type) {
	m_expected_type.set(type);
}

MPtr<const types::Type> ns::SyntaxExpressionExtension::get_expected_type() const {
	return m_expected_type.get();
}

void ns::SyntaxExpressionExtension::add_and_attribute(const ebnf::NameSyntaxElement* attribute) {
	//Duplicated attributes are allowed (and expected) in this vector.
	m_and_attributes.push_back(attribute);
}

void ns::SyntaxExpressionExtension::add_and_attributes(const std::vector<const ebnf::NameSyntaxElement*>& attributes) {
	for (const ebnf::NameSyntaxElement* attribute : attributes) m_and_attributes.push_back(attribute);
}

void ns::SyntaxExpressionExtension::clear_and_attributes() {
	//Clear the attributes vector.
	std::vector<const ebnf::NameSyntaxElement*>().swap(m_and_attributes);
}

const std::vector<const ebnf::NameSyntaxElement*>& ns::SyntaxExpressionExtension::get_and_attributes() const {
	return m_and_attributes;
}

void ns::SyntaxExpressionExtension::set_and_result(bool and_result) {
	m_and_result.set(and_result);
}

bool ns::SyntaxExpressionExtension::is_and_result() const {
	return m_and_result.get();
}

void ns::SyntaxExpressionExtension::set_conversion(std::unique_ptr<const conv::Conversion> conversion) {
	assert(conversion.get());
	assert(!m_conversion.get());
	m_conversion = std::move(conversion);
}

const conv::Conversion* ns::SyntaxExpressionExtension::get_conversion() const {
	assert(m_conversion.get());
	return m_conversion.get();
}

//
//SyntaxAndExpressionExtension
//

void ns::SyntaxAndExpressionExtension::set_meaning(std::unique_ptr<ns::AndExpressionMeaning> meaning) {
	assert(meaning.get());
	assert(!m_meaning.get());
	m_meaning = std::move(meaning);
}

ns::AndExpressionMeaning* ns::SyntaxAndExpressionExtension::get_meaning() const {
	assert(m_meaning.get());
	return m_meaning.get();
}

//
//AndExpressionMeaning
//

ns::AndExpressionMeaning::AndExpressionMeaning(
	const std::vector<MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions)
	: m_non_result_sub_expressions(non_result_sub_expressions)
{}

ns::AndExpressionMeaning::~AndExpressionMeaning()
{}

void ns::AndExpressionMeaning::visit(AndExpressionMeaningVisitor<void>* visitor) {
	visit0(visitor);
}

const std::vector<MPtr<ebnf::SyntaxExpression>>& ns::AndExpressionMeaning::get_non_result_sub_expressions() const {
	return m_non_result_sub_expressions;
}

//
//VoidAndExpressionMeaning
//

ns::VoidAndExpressionMeaning::VoidAndExpressionMeaning(
	const std::vector<MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions)
	: AndExpressionMeaning(non_result_sub_expressions)
{}

void ns::VoidAndExpressionMeaning::visit0(ns::AndExpressionMeaningVisitor<void>* visitor) {
	visitor->visit_VoidAndExpressionMeaning(this);
}

//
//ThisAndExpressionMeaning
//

ns::ThisAndExpressionMeaning::ThisAndExpressionMeaning(
	const std::vector<MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions,
	const std::vector<MPtr<ebnf::ThisSyntaxElement>>& result_elements)
	: AndExpressionMeaning(non_result_sub_expressions),
	m_result_elements(result_elements)
{}

void ns::ThisAndExpressionMeaning::visit0(ns::AndExpressionMeaningVisitor<void>* visitor) {
	visitor->visit_ThisAndExpressionMeaning(this);
}

const std::vector<MPtr<ebnf::ThisSyntaxElement>>& ns::ThisAndExpressionMeaning::get_result_elements() const {
	return m_result_elements;
}

//
//ClassAndExpressionMeaning
//

ns::ClassAndExpressionMeaning::ClassAndExpressionMeaning(
	const std::vector<MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions,
	bool has_attributes)
	: AndExpressionMeaning(non_result_sub_expressions),
	m_has_attributes(has_attributes)
{}

void ns::ClassAndExpressionMeaning::visit0(ns::AndExpressionMeaningVisitor<void>* visitor) {
	visitor->visit_ClassAndExpressionMeaning(this);
}

bool ns::ClassAndExpressionMeaning::has_attributes() const {
	return m_has_attributes;
}
