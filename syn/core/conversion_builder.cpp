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

//Conversion builder classes implementation.

#include <memory>
#include <stdexcept>

#include "commons.h"
#include "conversion.h"
#include "conversion_builder.h"
#include "descriptor_type.h"
#include "ebnf__def.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace conv = ns::conv;
namespace ebnf = ns::ebnf;
namespace util = ns::util;

namespace {
	using std::unique_ptr;

	using util::MPtr;

	typedef unique_ptr<const conv::Conversion> ConversionAutoPtr;
	typedef unique_ptr<conv::AndBNFResult> AndBNFResultAutoPtr;
	typedef unique_ptr<conv::AndConversionBuilder> AndConversionBuilderPtr;
	typedef unique_ptr<const conv::ComplexConversionType> ComplexConversionTypeAutoPtr;
	typedef unique_ptr<const conv::AttributeConversion> AttributeConversionAutoPtr;
}

//
//AndConversionBuilder
//

void conv::AndConversionBuilder::add_sub_element(const AndBNFResult* element_result, std::size_t element_index) {
	element_result->add_to_builder(this, element_index);
}

void conv::AndConversionBuilder::add_sub_element_this(const ThisAndBNFResult* element_result, std::size_t element_index) {
	throw err_illegal_state();
}

void conv::AndConversionBuilder::add_sub_element_attribute(
	const AttributeAndBNFResult* element_result,
	std::size_t element_index)
{
	throw err_illegal_state();
}

void conv::AndConversionBuilder::add_sub_element_part_class(
	const PartClassAndBNFResult* element_result,
	std::size_t element_index)
{
	throw err_illegal_state();
}

void conv::AndConversionBuilder::add_sub_element_class(const ClassAndBNFResult* element_result, std::size_t element_index) {
	throw err_illegal_state();
}

//
//VoidAndConversionBuilder
//

ConversionAutoPtr conv::VoidAndConversionBuilder::create_conversion(ebnf::SyntaxAndExpression* expr) {
	return ConversionAutoPtr(new VoidAndConversion(expr));
}

//
//ThisAndConversionBuilder
//

conv::ThisAndConversionBuilder::ThisAndConversionBuilder(const ebnf::SyntaxAndExpression* main_and_expr)
	: m_result_element_set(false),
	m_main_and_expr(main_and_expr)
{}

void conv::ThisAndConversionBuilder::add_sub_element_this(const ThisAndBNFResult* element_result, std::size_t element_index) {
	assert(!m_result_element_set);
	m_result_element_set = true;
	m_result_element_index = element_index;
}

ConversionAutoPtr conv::ThisAndConversionBuilder::create_conversion(ebnf::SyntaxAndExpression* expr) {
	assert(m_result_element_set);
	return ConversionAutoPtr(new ThisAndConversion(expr, m_result_element_index, m_main_and_expr));
}

//
//AttributeAndConversionBuilder
//

conv::AttributeAndConversionBuilder::AttributeAndConversionBuilder(const ebnf::NameSyntaxElement* attr_expr)
	: m_attribute_set(false),
	m_attr_expr(attr_expr)
{}

void conv::AttributeAndConversionBuilder::add_sub_element_attribute(
	const AttributeAndBNFResult* element_result,
	std::size_t element_index)
{
	assert(!m_attribute_set);
	m_attribute_index = element_index;
	m_attribute_set = true;
}

ConversionAutoPtr conv::AttributeAndConversionBuilder::create_conversion(ebnf::SyntaxAndExpression* expr) {
	if (m_attribute_set) {
		return ConversionAutoPtr(new AttributeAndConversion(expr, m_attribute_index, m_attr_expr));
	} else {
		//It is possible that the attribute has not been defined in a sub-expression of OR expression, while the OR
		//itself returns single attribute, causing attribute builders to be used for sub-expressions.
		return ConversionAutoPtr(new VoidAndConversion(expr));
	}
}

//
//AbstractClassAndConversionBuilder
//

conv::AbstractClassAndConversionBuilder::AbstractClassAndConversionBuilder()
	: m_used(false)
{}

void conv::AbstractClassAndConversionBuilder::add_sub_element_attribute(
	const AttributeAndBNFResult* element_result,
	std::size_t element_index)
{
	assert(!m_used);
	if (!m_attributes.get()) m_attributes = make_unique1<AttributeVector>();
	assert(!m_classes.get() || m_classes->empty());
	const util::String& attr_name = element_result->get_attr_expr()->get_name().get_string();
	m_attributes->push_back(AttributeField(element_index, attr_name));
}

void conv::AbstractClassAndConversionBuilder::add_sub_element_part_class(
	const PartClassAndBNFResult* element_result,
	std::size_t element_index)
{
	assert(!m_used);
	if (!m_part_classes.get()) m_part_classes = make_unique1<PartClassVector>();
	assert(!m_classes.get() || m_classes->empty());
	m_part_classes->push_back(PartClassField(element_index, element_result->get_part_class_tag()));
}

void conv::AbstractClassAndConversionBuilder::add_sub_element_class(
	const ClassAndBNFResult* element_result,
	std::size_t element_index)
{
	assert(!m_used);
	if (!m_classes.get()) m_classes = make_unique1<ClassVector>();

	//TODO Optimize the structure: there may be no more than one class field - do not use a vector.
	assert(m_classes->empty());
	assert(!m_attributes.get() || m_attributes->empty());
	assert(!m_part_classes.get() || m_part_classes->empty());
	m_classes->push_back(ClassField(element_index));
}

ConversionAutoPtr conv::AbstractClassAndConversionBuilder::create_conversion(ebnf::SyntaxAndExpression* expr) {
	assert(!m_used);
	unique_ptr<const AttributeVector> attributes = std::move(m_attributes);
	unique_ptr<const PartClassVector> part_classes = std::move(m_part_classes);
	unique_ptr<const ClassVector> classes = std::move(m_classes);
	m_used = true;
	return create_conversion0(expr, std::move(attributes), std::move(part_classes), std::move(classes));
}

//
//PartClassAndConversionBuilder
//

conv::PartClassAndConversionBuilder::PartClassAndConversionBuilder(
	const ebnf::SyntaxAndExpression* main_and_expr,
	const ns::PartClassTag& part_class_tag)
	: m_main_and_expr(main_and_expr),
	m_part_class_tag(part_class_tag)
{}

ConversionAutoPtr conv::PartClassAndConversionBuilder::create_conversion0(
	ebnf::SyntaxAndExpression* expr,
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes)
{
	return ConversionAutoPtr(new PartClassAndConversion(
		expr,
		m_main_and_expr,
		std::move(attributes),
		std::move(part_classes),
		std::move(classes),
		m_part_class_tag));
}

//
//ClassAndConversionBuilder
//

conv::ClassAndConversionBuilder::ClassAndConversionBuilder(const ebnf::SyntaxAndExpression* main_and_expr)
	: m_main_and_expr(main_and_expr)
{}

ConversionAutoPtr conv::ClassAndConversionBuilder::create_conversion0(
	ebnf::SyntaxAndExpression* expr,
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes)
{
	return ConversionAutoPtr(new ClassAndConversion(
		expr, m_main_and_expr, std::move(attributes), std::move(part_classes), std::move(classes)));
}

//
//VoidAndBNFResult
//

AndBNFResultAutoPtr conv::VoidAndBNFResult::clone() const {
	return AndBNFResultAutoPtr(new VoidAndBNFResult());
}

AndConversionBuilderPtr conv::VoidAndBNFResult::create_and_conversion_builder() const {
	return AndConversionBuilderPtr(new VoidAndConversionBuilder());
}

void conv::VoidAndBNFResult::add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const {
	//Do nothing.
}

ComplexConversionTypeAutoPtr conv::VoidAndBNFResult::get_complex_conversion_type() const {
	throw err_illegal_state();
}

AttributeConversionAutoPtr conv::VoidAndBNFResult::create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const {
	throw err_illegal_state();
}

//
//ThisAndBNFResult
//

conv::ThisAndBNFResult::ThisAndBNFResult(const ebnf::SyntaxAndExpression* main_and_expr)
	: m_main_and_expr(main_and_expr)
{}

AndBNFResultAutoPtr conv::ThisAndBNFResult::clone() const {
	return AndBNFResultAutoPtr(new ThisAndBNFResult(m_main_and_expr));
}

AndConversionBuilderPtr conv::ThisAndBNFResult::create_and_conversion_builder() const {
	return AndConversionBuilderPtr(new ThisAndConversionBuilder(m_main_and_expr));
}

void conv::ThisAndBNFResult::add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const {
	conversion_builder->add_sub_element_this(this, element_index);
}

ComplexConversionTypeAutoPtr conv::ThisAndBNFResult::get_complex_conversion_type() const {
	return ComplexConversionTypeAutoPtr(new ThisAndComplexConversionType(m_main_and_expr));
}

AttributeConversionAutoPtr conv::ThisAndBNFResult::create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const {
	throw err_illegal_state();
}

//
//AttributeAndBNFResult
//

conv::AttributeAndBNFResult::AttributeAndBNFResult(const ebnf::NameSyntaxElement* attr_expr)
	: m_attr_expr(attr_expr)
{}

const ebnf::NameSyntaxElement* conv::AttributeAndBNFResult::get_attr_expr() const {
	return m_attr_expr;
}

AndBNFResultAutoPtr conv::AttributeAndBNFResult::clone() const {
	return AndBNFResultAutoPtr(new AttributeAndBNFResult(m_attr_expr));
}

AndConversionBuilderPtr conv::AttributeAndBNFResult::create_and_conversion_builder() const {
	return AndConversionBuilderPtr(new AttributeAndConversionBuilder(m_attr_expr));
}

void conv::AttributeAndBNFResult::add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const {
	conversion_builder->add_sub_element_attribute(this, element_index);
}

ComplexConversionTypeAutoPtr conv::AttributeAndBNFResult::get_complex_conversion_type() const {
	return ComplexConversionTypeAutoPtr(new AttrAndComplexConversionType(m_attr_expr));
}

AttributeConversionAutoPtr conv::AttributeAndBNFResult::create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const {
	return AttributeConversionAutoPtr(new AttrAndAttributeConversion(attr_expr));
}

//
//PartClassAndBNFResult
//

conv::PartClassAndBNFResult::PartClassAndBNFResult(
	const ebnf::SyntaxAndExpression* main_and_expr,
	const ns::PartClassTag& part_class_tag)
	: m_main_and_expr(main_and_expr),
	m_part_class_tag(part_class_tag)
{}

const ns::PartClassTag& conv::PartClassAndBNFResult::get_part_class_tag() const {
	return m_part_class_tag;
}

AndBNFResultAutoPtr conv::PartClassAndBNFResult::clone() const {
	return AndBNFResultAutoPtr(new PartClassAndBNFResult(m_main_and_expr, m_part_class_tag));
}

AndConversionBuilderPtr conv::PartClassAndBNFResult::create_and_conversion_builder() const {
	return AndConversionBuilderPtr(new PartClassAndConversionBuilder(m_main_and_expr, m_part_class_tag));
}


void conv::PartClassAndBNFResult::add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const {
	conversion_builder->add_sub_element_part_class(this, element_index);
}

ComplexConversionTypeAutoPtr conv::PartClassAndBNFResult::get_complex_conversion_type() const {
	return ComplexConversionTypeAutoPtr(new PartClassAndComplexConversionType(m_main_and_expr, m_part_class_tag));
}

AttributeConversionAutoPtr conv::PartClassAndBNFResult::create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const {
	return AttributeConversionAutoPtr(new PartClassAndAttributeConversion(attr_expr, m_main_and_expr, m_part_class_tag));
}

//
//ClassAndBNFResult
//

conv::ClassAndBNFResult::ClassAndBNFResult(const ebnf::SyntaxAndExpression* main_and_expr)
	: m_main_and_expr(main_and_expr)
{}

AndBNFResultAutoPtr conv::ClassAndBNFResult::clone() const {
	return AndBNFResultAutoPtr(new ClassAndBNFResult(m_main_and_expr));
}

AndConversionBuilderPtr conv::ClassAndBNFResult::create_and_conversion_builder() const {
	return AndConversionBuilderPtr(new ClassAndConversionBuilder(m_main_and_expr));
}

void conv::ClassAndBNFResult::add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const {
	conversion_builder->add_sub_element_class(this, element_index);
}

ComplexConversionTypeAutoPtr conv::ClassAndBNFResult::get_complex_conversion_type() const {
	return ComplexConversionTypeAutoPtr(new ClassAndComplexConversionType(m_main_and_expr));
}

AttributeConversionAutoPtr conv::ClassAndBNFResult::create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const {
	return AttributeConversionAutoPtr(new ClassAndAttributeConversion(attr_expr, m_main_and_expr));
}
