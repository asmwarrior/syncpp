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

//Conversion classes implementation.

#include <cassert>
#include <memory>
#include <vector>
#include <utility>

#include "action_factory.h"
#include "commons.h"
#include "conversion.h"
#include "converter.h"
#include "descriptor_type.h"
#include "ebnf__def.h"
#include "util_mptr.h"
#include "util_string.h"

namespace ns = synbin;
namespace conv = ns::conv;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

using std::unique_ptr;

using util::MPtr;
using util::String;

namespace {
	typedef unique_ptr<const conv::ComplexConversionType> ComplexConversionTypeAutoPtr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Misc.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
	
	MPtr<const ns::TypeDescriptor> convert_expr_type(
		synbin::ConverterFacade* converter,
		const ebnf::SyntaxExpression* expr)
	{
		MPtr<const types::Type> concrete_type = expr->get_extension()->get_concrete_type();
		assert(!!concrete_type);
		MPtr<const ns::TypeDescriptor> type = converter->convert_type(concrete_type.get());
		return type;
	}
	
	MPtr<const ns::ClassTypeDescriptor> convert_expr_class_type(
		synbin::ConverterFacade* converter,
		const ebnf::SyntaxExpression* expr)
	{
		MPtr<const types::Type> concrete_type = expr->get_extension()->get_concrete_type();
		assert(!!concrete_type);
		const types::ClassType* class_concrete_type = concrete_type->as_class();
		assert(class_concrete_type);
		MPtr<const ns::ClassTypeDescriptor> class_type = converter->convert_class_type(class_concrete_type);
		return class_type;
	}
	
	MPtr<const ns::PartClassTypeDescriptor> convert_and_part_class_type(
		synbin::ConverterFacade* converter,
		const ebnf::SyntaxAndExpression* and_expr,
		const ns::PartClassTag& part_class_tag)
	{
		MPtr<const ns::ClassTypeDescriptor> class_type = convert_expr_class_type(converter, and_expr);
		MPtr<const ns::PartClassTypeDescriptor> part_class_type =
			converter->convert_part_class_type(class_type, part_class_tag);
		return part_class_type;
	}

	MPtr<const ns::PrimitiveTypeDescriptor> convert_const_expr_type(
		ns::ConverterFacade* converter,
		const ebnf::ConstSyntaxExpression* expr)
	{
		//TODO Ensure that a constant always has a concrete type.
		MPtr<const types::Type> concrete_type = expr->get_extension()->get_concrete_type();
		assert(!!concrete_type);
		const types::PrimitiveType* primitive_type = concrete_type->as_primitive();
		assert(primitive_type);
		MPtr<const ns::PrimitiveTypeDescriptor> type = converter->convert_primitive_type(primitive_type);
		return type;
	}

	MPtr<const ns::ListTypeDescriptor> convert_expr_list_type(
		ns::ConverterFacade* converter,
		const ebnf::LoopSyntaxExpression* loop_expr)
	{
		ebnf::SyntaxExpression* sub_expression = loop_expr->get_body()->get_expression();
		MPtr<conv::ConvSym> conv_sub_expression = converter->convert_expression_to_symbol(sub_expression);
		MPtr<const ns::TypeDescriptor> element_type = converter->get_symbol_type(conv_sub_expression);
		assert(!element_type->is_void());
		MPtr<const ns::ListTypeDescriptor> list_type = converter->create_list_type(element_type);
		return list_type;
	}

	MPtr<const types::Type> get_concrete_type(const ebnf::SyntaxExpression* expr) {
		MPtr<const types::Type> concrete_type = expr->get_extension()->get_concrete_type();
		assert(!!concrete_type);
		return concrete_type;
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Conversion Types
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
//TopComplexConversionType
//

MPtr<const ns::TypeDescriptor> conv::TopComplexConversionType::get_result_type(
	ns::ConverterFacade* converter,
	ebnf::SyntaxExpression* expr) const
{
	return convert_expr_type(converter, expr);
}

//
//DeadComplexConversionType
//

MPtr<const ns::TypeDescriptor> conv::DeadComplexConversionType::get_result_type(
	ns::ConverterFacade* converter,
	ebnf::SyntaxExpression* expr) const
{
	return converter->get_void_type();
}

//
//ThisAndComplexConversionType
//

conv::ThisAndComplexConversionType::ThisAndComplexConversionType(const ebnf::SyntaxAndExpression* main_and_expr)
	: m_main_and_expr(main_and_expr)
{}

MPtr<const ns::TypeDescriptor> conv::ThisAndComplexConversionType::get_result_type(
	ns::ConverterFacade* converter,
	ebnf::SyntaxExpression* expr) const
{
	MPtr<const TypeDescriptor> type = convert_expr_type(converter, m_main_and_expr);
	assert(!type->is_void());
	return type;
}

//
//AttrAndComplexConversionType
//

conv::AttrAndComplexConversionType::AttrAndComplexConversionType(const ebnf::NameSyntaxElement* attr_expr)
	: m_attr_expr(attr_expr)
{}

MPtr<const ns::TypeDescriptor> conv::AttrAndComplexConversionType::get_result_type(
	ns::ConverterFacade* converter,
	ebnf::SyntaxExpression* expr) const
{
	const ebnf::SyntaxExpression* sub_expr = m_attr_expr->get_expression();
	MPtr<const TypeDescriptor> type = convert_expr_type(converter, sub_expr);
	assert(!type->is_void());
	return type;
}

//
//PartClassAndComplexConversionType
//

conv::PartClassAndComplexConversionType::PartClassAndComplexConversionType(
	const ebnf::SyntaxAndExpression* main_and_expr,
	const ns::PartClassTag& part_class_tag)
	: m_main_and_expr(main_and_expr),
	m_part_class_tag(part_class_tag)
{}

MPtr<const ns::TypeDescriptor> conv::PartClassAndComplexConversionType::get_result_type(
	ns::ConverterFacade* converter,
	ebnf::SyntaxExpression* expr) const
{
	return convert_and_part_class_type(converter, m_main_and_expr, m_part_class_tag);
}

//
//ClassAndComplexConversionType
//

conv::ClassAndComplexConversionType::ClassAndComplexConversionType(const ebnf::SyntaxAndExpression* main_and_expr)
	: m_main_and_expr(main_and_expr)
{}

MPtr<const ns::TypeDescriptor> conv::ClassAndComplexConversionType::get_result_type(
	ns::ConverterFacade* converter,
	ebnf::SyntaxExpression* expr) const
{
	return convert_expr_class_type(converter, m_main_and_expr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Conversions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
//Conversion
//

conv::Conversion::Conversion(){}

void conv::Conversion::delegate_nt_to_pr(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const {
	converter->convert_expression_to_production(conv_nt, get_expr());
}

void conv::Conversion::delegate_pr_to_sym(
	ns::ConverterFacade* converter,
	ns::ConvPrBuilderFacade* conv_pr_builder,
	bool is_dead) const
{
	MPtr<conv::ConvSym> conv_sym = convert_sym(converter);
	conv_pr_builder->add_element(conv_sym);
	
	MPtr<const TypeDescriptor> type = converter->get_symbol_type(conv_sym);
	assert(!!type);
	if (is_dead || type->is_void()) {
		VoidActionFactory action_factory;
		conv_pr_builder->set_action_factory(action_factory);
	} else {
		CopyActionFactory action_factory;
		conv_pr_builder->set_action_factory(action_factory);
	}
}

MPtr<conv::ConvSym> conv::Conversion::delegate_sym_to_nt(
	synbin::ConverterFacade* converter,
	MPtr<const ns::TypeDescriptor> nt_type) const
{
	MPtr<conv::ConvSym> conv_sym = converter->convert_expression_to_nonterminal(get_expr(), nt_type);
	return conv_sym;
}

//
//EmptyConversion
//

conv::EmptyConversion::EmptyConversion(ebnf::EmptySyntaxExpression* expr)
	: m_expr(expr)
{}

ebnf::SyntaxExpression* conv::EmptyConversion::get_expr() const {
	return m_expr;
}

void conv::EmptyConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	//TODO Use a shared void nonterminal.
	delegate_nt_to_pr(converter, conv_nt);
}

void conv::EmptyConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	VoidActionFactory action_factory;
	conv_pr_builder->set_action_factory(action_factory);//TODO Share void actions, if possible.
}

MPtr<conv::ConvSym> conv::EmptyConversion::convert_sym(ns::ConverterFacade* converter) const {
	return delegate_sym_to_nt(converter, converter->get_void_type());
}

//
//ConstConversion
//

conv::ConstConversion::ConstConversion(ebnf::ConstSyntaxExpression* expr)
	: m_expr(expr)
{}

ebnf::SyntaxExpression* conv::ConstConversion::get_expr() const {
	return m_expr;
}

void conv::ConstConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	delegate_nt_to_pr(converter, conv_nt);
}

void conv::ConstConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	MPtr<const PrimitiveTypeDescriptor> type = convert_const_expr_type(converter, m_expr);
	ConstActionFactory action_factory(type, m_expr->get_expression());
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<conv::ConvSym> conv::ConstConversion::convert_sym(ns::ConverterFacade* converter) const {
	MPtr<const PrimitiveTypeDescriptor> type = convert_const_expr_type(converter, m_expr);
	return delegate_sym_to_nt(converter, type);
}

//
//CastConversion
//

conv::CastConversion::CastConversion(ebnf::CastSyntaxExpression* expr)
	: m_expr(expr)
{}

ebnf::SyntaxExpression* conv::CastConversion::get_expr() const {
	return m_expr;
}

void conv::CastConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	delegate_nt_to_pr(converter, conv_nt);
}

void conv::CastConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	ebnf::SyntaxExpression* sub_expr = m_expr->get_expression();
	MPtr<const types::Type> actual_type = sub_expr->get_extension()->get_concrete_type();
	assert(!actual_type->is_void());
	MPtr<const types::Type> cast_type = m_expr->get_type();
	
	if (cast_type->equals(actual_type.get())) {
		const Conversion* sub_conv = sub_expr->get_extension()->get_conversion();
		sub_conv->convert_pr(converter, conv_pr_builder);
	} else {
		//Both types must be class types (already checked in EBNF Type Builder).
		const types::ClassType* actual_class_type = actual_type->as_class();
		const types::ClassType* cast_class_type = cast_type->as_class();
		assert(actual_class_type);
		assert(cast_class_type);

		MPtr<conv::ConvSym> conv_sym = converter->convert_expression_to_symbol(sub_expr);
		conv_pr_builder->add_element(conv_sym);
		
		MPtr<const ClassTypeDescriptor> cast_class_type_descriptor = converter->convert_class_type(cast_class_type);
		CastActionFactory action_factory(cast_class_type_descriptor);
		conv_pr_builder->set_action_factory(action_factory);
	}
}

MPtr<conv::ConvSym> conv::CastConversion::convert_sym(ns::ConverterFacade* converter) const {
	ebnf::SyntaxExpression* sub_expr = m_expr->get_expression();
	MPtr<const types::Type> actual_type = sub_expr->get_extension()->get_concrete_type();
	MPtr<const types::Type> cast_type = m_expr->get_type();
	
	if (cast_type->equals(actual_type.get())) {
		//The cast type is the same as the actual type. Action is not needed.
		MPtr<conv::ConvSym> conv_sym = converter->convert_expression_to_symbol(sub_expr);
		return conv_sym;
	} else {
		//Conversion is needed.
		const types::ClassType* cast_class_type = cast_type->as_class();
		assert(cast_class_type);
		MPtr<const TypeDescriptor> cast_class_type_desc = converter->convert_class_type(cast_class_type);
		return delegate_sym_to_nt(converter, cast_class_type_desc);
	}
}

//
//ThisConversion
//

conv::ThisConversion::ThisConversion(ebnf::ThisSyntaxElement* expr)
	: m_expr(expr)
{}

ebnf::SyntaxExpression* conv::ThisConversion::get_expr() const {
	return m_expr;
}

void conv::ThisConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	const ebnf::SyntaxExpression* sub_expr = m_expr->get_expression();
	assert(!get_concrete_type(sub_expr)->is_void());
	const Conversion* sub_conv = sub_expr->get_extension()->get_conversion();
	sub_conv->convert_nt(converter, conv_nt);
}

void conv::ThisConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	const ebnf::SyntaxExpression* sub_expr = m_expr->get_expression();
	assert(!get_concrete_type(sub_expr)->is_void());
	const Conversion* sub_conv = sub_expr->get_extension()->get_conversion();
	sub_conv->convert_pr(converter, conv_pr_builder);
}

MPtr<conv::ConvSym> conv::ThisConversion::convert_sym(ns::ConverterFacade* converter) const {
	ebnf::SyntaxExpression* sub_expr = m_expr->get_expression();
	assert(!get_concrete_type(sub_expr)->is_void());
	const Conversion* sub_conv = sub_expr->get_extension()->get_conversion();
	return sub_conv->convert_sym(converter);
}

//
//SimpleConversion
//
conv::SimpleConversion::SimpleConversion(SimpleConversionType conversion_type)
	: m_conversion_type(conversion_type)
{}

void conv::SimpleConversion::delegate_pr_to_sym0(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	delegate_pr_to_sym(converter, conv_pr_builder, SIMPLE_DEAD == m_conversion_type);
}

//
//NameConversion
//

conv::NameConversion::NameConversion(ebnf::NameSyntaxExpression* expr, SimpleConversionType conversion_type)
	: SimpleConversion(conversion_type), m_expr(expr)
{}

ebnf::SyntaxExpression* conv::NameConversion::get_expr() const {
	return m_expr;
}

void conv::NameConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	delegate_nt_to_pr(converter, conv_nt);
}

void conv::NameConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	delegate_pr_to_sym0(converter, conv_pr_builder);
}

MPtr<conv::ConvSym> conv::NameConversion::convert_sym(ns::ConverterFacade* converter) const {
	ebnf::SymbolDeclaration* sym = m_expr->get_sym();
	MPtr<conv::ConvSym> conv_sym = converter->convert_symbol_to_symbol(sym);
	return conv_sym;
}

//
//StringConversion
//

conv::StringConversion::StringConversion(ebnf::StringSyntaxExpression* expr, SimpleConversionType conversion_type)
	: SimpleConversion(conversion_type), m_expr(expr)
{}

ebnf::SyntaxExpression* conv::StringConversion::get_expr() const {
	return m_expr;
}

void conv::StringConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	delegate_nt_to_pr(converter, conv_nt);
}

void conv::StringConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	delegate_pr_to_sym0(converter, conv_pr_builder);
}

MPtr<conv::ConvSym> conv::StringConversion::convert_sym(ns::ConverterFacade* converter) const {
	const ns::syntax_string& str = m_expr->get_string();
	MPtr<conv::ConvSym> conv_sym = converter->convert_string_to_symbol(str);
	return conv_sym;
}

//
//is_loop_void()
//

namespace {
	bool is_loop_void(conv::SimpleConversionType conversion_type, ebnf::LoopSyntaxExpression* expr) {
		if (conv::SIMPLE_TOP == conversion_type) {
			MPtr<const types::Type> type = expr->get_extension()->get_concrete_type();
			return type->is_void();
		} else {
			assert(conv::SIMPLE_DEAD == conversion_type);
		}
		return true;
	}
}

//
//convert_nt_one_many0()
//

namespace {
	void convert_nt_one_many0(
		synbin::ConverterFacade* converter,
		util::MPtr<conv::ConvNt> conv_nt,
		ns::ActionFactory& many_action_factory,
		ns::ActionFactory& one_action_factory,
		ebnf::LoopSyntaxExpression* expr)
	{
		//"Many" production.
		std::vector<MPtr<conv::ConvSym>> many_conv_elements;
		many_conv_elements.push_back(converter->cast_nt_to_sym(conv_nt));
		ebnf::SyntaxExpression* separator = expr->get_body()->get_separator();
		if (separator) {
			MPtr<conv::ConvSym> conv_separator = converter->convert_expression_to_symbol(separator);
			many_conv_elements.push_back(conv_separator);
		}
		ebnf::SyntaxExpression* sub_expression = expr->get_body()->get_expression();
		MPtr<conv::ConvSym> conv_sub_expression = converter->convert_expression_to_symbol(sub_expression);
		many_conv_elements.push_back(conv_sub_expression);
		converter->create_production(conv_nt, many_conv_elements, many_action_factory);

		//"One" production.
		std::vector<MPtr<conv::ConvSym>> one_conv_elements;
		one_conv_elements.push_back(conv_sub_expression);
		converter->create_production(conv_nt, one_conv_elements, one_action_factory);
	}
}

//
//convert_nt_one_many()
//

namespace {
	void convert_nt_one_many(
		synbin::ConverterFacade* converter,
		util::MPtr<conv::ConvNt> conv_nt,
		conv::SimpleConversionType conversion_type,
		ebnf::LoopSyntaxExpression* expr)
	{
		if (is_loop_void(conversion_type, expr)) {
			ns::VoidActionFactory many_action_factory;
			ns::VoidActionFactory one_action_factory;
			convert_nt_one_many0(converter, conv_nt, many_action_factory, one_action_factory, expr);
		} else {
			MPtr<const ns::ListTypeDescriptor> list_type = convert_expr_list_type(converter, expr);
			bool separator = !!expr->get_body()->get_separator();
			ns::NextListActionFactory many_action_factory(list_type, separator);
			ns::FirstListActionFactory one_action_factory(list_type);
			convert_nt_one_many0(converter, conv_nt, many_action_factory, one_action_factory, expr);
		}
	}
}

//
//LoopConversion
//

conv::LoopConversion::LoopConversion(ebnf::LoopSyntaxExpression* expr, SimpleConversionType conversion_type)
	: SimpleConversion(conversion_type), m_expr(expr)
{}

ebnf::SyntaxExpression* conv::LoopConversion::get_expr() const {
	return m_expr;
}

void conv::LoopConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	delegate_pr_to_sym0(converter, conv_pr_builder);
}

MPtr<conv::ConvSym> conv::LoopConversion::convert_sym(ns::ConverterFacade* converter) const {
	MPtr<const TypeDescriptor> type;
	if (is_loop_void(m_conversion_type, m_expr)) {
		type = converter->get_void_type();
	} else {
		type = convert_expr_list_type(converter, m_expr);
	}
	return delegate_sym_to_nt(converter, type);
}

//
//convert_nt_zero_many0
//

namespace {
	void convert_nt_zero_many(
		ns::ConverterFacade* converter,
		MPtr<conv::ConvNt> conv_nt,
		MPtr<const ns::TypeDescriptor> list_type,
		ns::ActionFactory& one_many_action_factory,
		conv::SimpleConversionType conversion_type,
		ebnf::LoopSyntaxExpression* expr)
	{
		//One-Many production.
		MPtr<conv::ConvNt> one_many_conv_nt = converter->create_auto_nonterminal(list_type);
		convert_nt_one_many(converter, one_many_conv_nt, conversion_type, expr);
		std::vector<MPtr<conv::ConvSym>> one_many_conv_elements;
		one_many_conv_elements.push_back(converter->cast_nt_to_sym(one_many_conv_nt));
		converter->create_production(conv_nt, one_many_conv_elements, one_many_action_factory);

		//Void production.
		std::vector<MPtr<conv::ConvSym>> void_conv_elements;
		ns::VoidActionFactory zero_action_factory;
		converter->create_production(conv_nt, void_conv_elements, zero_action_factory);
	}
}

//
//ZeroManyConversion
//

conv::ZeroManyConversion::ZeroManyConversion(ebnf::ZeroManySyntaxExpression* expr, SimpleConversionType conversion_type)
	: LoopConversion(expr, conversion_type)
{}

void conv::ZeroManyConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	if (is_loop_void(m_conversion_type, m_expr)) {
		MPtr<const TypeDescriptor> void_type = converter->get_void_type();
		VoidActionFactory one_many_action_factory;
		convert_nt_zero_many(converter, conv_nt, void_type, one_many_action_factory, m_conversion_type, m_expr);
	} else {
		MPtr<const ListTypeDescriptor> list_type = convert_expr_list_type(converter, m_expr);
		CopyActionFactory one_many_action_factory;
		convert_nt_zero_many(converter, conv_nt, list_type, one_many_action_factory, m_conversion_type, m_expr);
	}
}

//
//OneManyConversion
//

conv::OneManyConversion::OneManyConversion(ebnf::OneManySyntaxExpression* expr, SimpleConversionType conversion_type)
	: LoopConversion(expr, conversion_type)
{}

void conv::OneManyConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	convert_nt_one_many(converter, conv_nt, m_conversion_type, m_expr);
}

//
//ComplexConversion
//

conv::ComplexConversion::ComplexConversion(
	ComplexConversionTypeAutoPtr conversion_type,
	const ebnf::SyntaxAndExpression* and_expr)
	: m_conversion_type(std::move(conversion_type)),
	m_and_expr(and_expr)
{
	assert(m_conversion_type->is_and_conversion() || !m_and_expr);
	assert(!m_conversion_type->is_and_conversion() || m_and_expr);
}

MPtr<const ns::TypeDescriptor> conv::ComplexConversion::get_result_type(ns::ConverterFacade* converter) const {
	ebnf::SyntaxExpression* expr = get_expr();
	MPtr<const ns::TypeDescriptor> type = m_conversion_type->get_result_type(converter, expr);
	return type;
}

void conv::ComplexConversion::delegate_pr_to_sym0(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	bool is_dead = m_conversion_type->is_dead();
	delegate_pr_to_sym(converter, conv_pr_builder, is_dead);
}

//
//OrConversion
//

conv::OrConversion::OrConversion(
	ComplexConversionTypeAutoPtr conversion_type,
	const ebnf::SyntaxAndExpression* and_expr,
	ebnf::SyntaxOrExpression* expr)
	: ComplexConversion(std::move(conversion_type), and_expr),
	m_expr(expr)
{}

ebnf::SyntaxExpression* conv::OrConversion::get_expr() const {
	return m_expr;
}

void conv::OrConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	for (MPtr<ebnf::SyntaxExpression> sub_expr : m_expr->get_sub_expressions()) {
		converter->convert_expression_to_production(conv_nt, sub_expr.get());
	}
}

void conv::OrConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	delegate_pr_to_sym0(converter, conv_pr_builder);
}

MPtr<conv::ConvSym> conv::OrConversion::convert_sym(ns::ConverterFacade* converter) const {
	MPtr<const TypeDescriptor> type = get_result_type(converter);
	return delegate_sym_to_nt(converter, type);
}

//
//ZeroOneConversion
//

conv::ZeroOneConversion::ZeroOneConversion(
	ComplexConversionTypeAutoPtr conversion_type,
	const ebnf::SyntaxAndExpression* and_expr,
	ebnf::ZeroOneSyntaxExpression* expr)
	: ComplexConversion(std::move(conversion_type), and_expr),
	m_expr(expr)
{}

ebnf::SyntaxExpression* conv::ZeroOneConversion::get_expr() const {
	return m_expr;
}

void conv::ZeroOneConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	//Value production.
	converter->convert_expression_to_production(conv_nt, m_expr->get_sub_expression());

	//Void production.
	std::vector<MPtr<conv::ConvSym>> conv_void_elements;
	VoidActionFactory action_factory;
	converter->create_production(conv_nt, conv_void_elements, action_factory);
}

void conv::ZeroOneConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	delegate_pr_to_sym0(converter, conv_pr_builder);
}

MPtr<conv::ConvSym> conv::ZeroOneConversion::convert_sym(ns::ConverterFacade* converter) const {
	MPtr<const TypeDescriptor> type = get_result_type(converter);
	return delegate_sym_to_nt(converter, type);
}

//
//AttributeConversion
//

conv::AttributeConversion::AttributeConversion(ebnf::NameSyntaxElement* expr)
	: m_expr(expr)
{}

ebnf::SyntaxExpression* conv::AttributeConversion::get_expr() const {
	return m_expr;
}

void conv::AttributeConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	delegate_nt_to_pr(converter, conv_nt);
}

void conv::AttributeConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	MPtr<conv::ConvSym> conv_sym = converter->convert_expression_to_symbol(m_expr->get_expression());
	conv_pr_builder->add_element(conv_sym);
	define_action_factory(converter, conv_pr_builder);
}

MPtr<conv::ConvSym> conv::AttributeConversion::convert_sym(ns::ConverterFacade* converter) const {
	MPtr<const TypeDescriptor> type = get_result_type(converter);
	return delegate_sym_to_nt(converter, type);
}

//
//TopAttributeConversion
//

conv::TopAttributeConversion::TopAttributeConversion(ebnf::NameSyntaxElement* expr)
	: AttributeConversion(expr)
{}

void conv::TopAttributeConversion::define_action_factory(
	ns::ConverterFacade* converter,
	ns::ConvPrBuilderFacade* conv_pr_builder) const
{
	MPtr<const ClassTypeDescriptor> class_type = convert_expr_class_type(converter, m_expr);

	unique_ptr<ActionDefs::AttributeVector> attributes = make_unique1<ActionDefs::AttributeVector>();
	attributes->push_back(ActionDefs::AttributeField(0, m_expr->get_name().get_string()));

	ClassActionFactory action_factory(class_type, std::move(attributes), nullptr, nullptr);
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::TopAttributeConversion::get_result_type(ns::ConverterFacade* converter) const {
	MPtr<const ClassTypeDescriptor> type = convert_expr_class_type(converter, m_expr);
	return type;
}

//
//AttrAndAttributeConversion
//

conv::AttrAndAttributeConversion::AttrAndAttributeConversion(ebnf::NameSyntaxElement* expr)
	: AttributeConversion(expr)
{}

MPtr<conv::ConvSym> conv::AttrAndAttributeConversion::convert_sym(ns::ConverterFacade* converter) const {
	const ebnf::SyntaxExpression* sub_expr = m_expr->get_expression();
	MPtr<conv::ConvSym> conv_sym = sub_expr->get_extension()->get_conversion()->convert_sym(converter);
	return conv_sym;
}

void conv::AttrAndAttributeConversion::define_action_factory(
	ns::ConverterFacade* converter,
	ns::ConvPrBuilderFacade* conv_pr_builder) const
{
	CopyActionFactory action_factory;
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::AttrAndAttributeConversion::get_result_type(ns::ConverterFacade* converter) const {
	ebnf::SyntaxExpression* sub_expr = m_expr->get_expression();
	MPtr<const TypeDescriptor> type = convert_expr_type(converter, sub_expr);
	assert(!type->is_void());
	return type;
}

//
//PartClassAndAttributeConversion
//

conv::PartClassAndAttributeConversion::PartClassAndAttributeConversion(
	ebnf::NameSyntaxElement* expr,
	const ebnf::SyntaxAndExpression* main_and_expr,
	const ns::PartClassTag& part_class_tag)
	: AttributeConversion(expr),
	m_main_and_expr(main_and_expr),
	m_part_class_tag(part_class_tag)
{}

void conv::PartClassAndAttributeConversion::define_action_factory(
	ns::ConverterFacade* converter,
	ns::ConvPrBuilderFacade* conv_pr_builder) const
{
	unique_ptr<ActionDefs::AttributeVector> attributes_var = make_unique1<ActionDefs::AttributeVector>();
	attributes_var->push_back(ActionDefs::AttributeField(0, m_expr->get_name().get_string()));

	unique_ptr<const ActionDefs::AttributeVector> attributes = std::move(attributes_var);
	unique_ptr<const ActionDefs::PartClassVector> part_classes;
	unique_ptr<const ActionDefs::ClassVector> classes;

	MPtr<const PartClassTypeDescriptor> part_class_type = convert_and_part_class_type(
		converter,
		m_main_and_expr,
		m_part_class_tag);

	PartClassActionFactory action_factory(
		std::move(attributes), std::move(part_classes), std::move(classes), part_class_type);

	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::PartClassAndAttributeConversion::get_result_type(
	ns::ConverterFacade* converter) const
{
	return convert_and_part_class_type(converter, m_main_and_expr, m_part_class_tag);
}

//
//ClassAndAttributeConversion
//

conv::ClassAndAttributeConversion::ClassAndAttributeConversion(
	ebnf::NameSyntaxElement* expr,
	const ebnf::SyntaxAndExpression* main_and_expr)
	: AttributeConversion(expr),
	m_main_and_expr(main_and_expr)
{}

void conv::ClassAndAttributeConversion::define_action_factory(
	ns::ConverterFacade* converter,
	ns::ConvPrBuilderFacade* conv_pr_builder) const
{
	MPtr<const ClassTypeDescriptor> class_type = convert_expr_class_type(converter, m_main_and_expr);

	unique_ptr<ActionDefs::AttributeVector> attributes_var = make_unique1<ActionDefs::AttributeVector>();
	attributes_var->push_back(ActionDefs::AttributeField(0, m_expr->get_name().get_string()));

	unique_ptr<const ActionDefs::AttributeVector> attributes = std::move(attributes_var);
	unique_ptr<const ActionDefs::PartClassVector> part_classes;
	unique_ptr<const ActionDefs::ClassVector> classes;

	ClassActionFactory action_factory(class_type, std::move(attributes), std::move(part_classes), std::move(classes));
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::ClassAndAttributeConversion::get_result_type(ns::ConverterFacade* converter) const {
	MPtr<const ClassTypeDescriptor> class_type = convert_expr_class_type(converter, m_main_and_expr);
	return class_type;
}

//
//AndConversion
//

conv::AndConversion::AndConversion(ebnf::SyntaxAndExpression* expr)
	: m_expr(expr)
{}

ebnf::SyntaxExpression* conv::AndConversion::get_expr() const {
	return m_expr;
}

void conv::AndConversion::convert_nt(ns::ConverterFacade* converter, MPtr<ConvNt> conv_nt) const {
	converter->convert_expression_to_production(conv_nt, m_expr);
}

void conv::AndConversion::convert_pr(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	for (MPtr<ebnf::SyntaxExpression> sub_expr : m_expr->get_sub_expressions()) {
		MPtr<conv::ConvSym> conv_sub_sym = converter->convert_expression_to_symbol(sub_expr.get());
		conv_pr_builder->add_element(conv_sub_sym);
	}
	define_action(converter, conv_pr_builder);
}

MPtr<conv::ConvSym> conv::AndConversion::convert_sym(ns::ConverterFacade* converter) const {
	MPtr<const TypeDescriptor> type = get_result_type(converter);
	return delegate_sym_to_nt(converter, type);
}

//
//VoidAndConversion
//

conv::VoidAndConversion::VoidAndConversion(ebnf::SyntaxAndExpression* expr)
	: AndConversion(expr)
{}

void conv::VoidAndConversion::define_action(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	VoidActionFactory action_factory;
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::VoidAndConversion::get_result_type(ns::ConverterFacade* converter) const {
	return converter->get_void_type();
}

//
//ThisAndConversion
//

conv::ThisAndConversion::ThisAndConversion(
	ebnf::SyntaxAndExpression* expr,
	std::size_t result_index,
	const ebnf::SyntaxAndExpression* main_and_expr)
	: AndConversion(expr),
	m_result_index(result_index),
	m_main_and_expr(main_and_expr)
{
	assert(result_index < expr->get_sub_expressions().size());
}

void conv::ThisAndConversion::define_action(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	ResultAndActionFactory action_factory(m_result_index);
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::ThisAndConversion::get_result_type(ns::ConverterFacade* converter) const {
	MPtr<const TypeDescriptor> type = convert_expr_type(converter, m_main_and_expr);
	assert(!type->is_void());
	return type;
}

//
//AttributeAndConversion
//

conv::AttributeAndConversion::AttributeAndConversion(
	ebnf::SyntaxAndExpression* expr,
	std::size_t attribute_index,
	const ebnf::NameSyntaxElement* attr_expr)
	: AndConversion(expr),
	m_attribute_index(attribute_index),
	m_attr_expr(attr_expr)
{}

void conv::AttributeAndConversion::define_action(ns::ConverterFacade* converter, ns::ConvPrBuilderFacade* conv_pr_builder) const {
	ResultAndActionFactory action_factory(m_attribute_index);
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::AttributeAndConversion::get_result_type(ns::ConverterFacade* converter) const {
	const ebnf::SyntaxExpression* sub_expr = m_attr_expr->get_expression();
	MPtr<const TypeDescriptor> type = convert_expr_type(converter, sub_expr);
	assert(!type->is_void());
	return type;
}

//
//vector_unique_ptr_empty()
//

template<class T>
bool vector_unique_ptr_empty(unique_ptr<const std::vector<T>>& vector) {
	return !vector.get() || vector->empty();
}


//
//AbstractClassAndConversion
//

conv::AbstractClassAndConversion::AbstractClassAndConversion(
	ebnf::SyntaxAndExpression* expr,
	const ebnf::SyntaxAndExpression* main_and_expr,
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes)
	: AndConversion(expr),
	m_main_and_expr(main_and_expr),
	m_attributes(std::move(attributes)),
	m_part_classes(std::move(part_classes)),
	m_classes(std::move(classes))
{
	assert(m_main_and_expr);
	assert(vector_unique_ptr_empty(m_classes)
		|| (vector_unique_ptr_empty(m_attributes) && vector_unique_ptr_empty(m_part_classes)));
	assert(vector_unique_ptr_empty(m_classes) || m_classes->size() == 1);
}

const ebnf::SyntaxAndExpression* conv::AbstractClassAndConversion::get_main_and_expr() const {
	return m_main_and_expr;
}

namespace {

	template<class A, class C, class F>
	unique_ptr<const std::vector<A>> convert_fields(const std::vector<C>* conv_fields, const F& convert_fn) {
		if (!conv_fields) return nullptr;

		unique_ptr<std::vector<A>> action_fields = ns::make_unique1<std::vector<A>>();
		action_fields->reserve(conv_fields->size());
		for (std::size_t i = 0, n = conv_fields->size(); i < n; ++i) {
			A action_field = convert_fn((*conv_fields)[i]);
			action_fields->push_back(action_field);
		}

		return util::const_vector_ptr(std::move(action_fields));
	}

	ns::ActionDefs::AttributeField convert_attribute_field(
		const conv::AbstractClassAndConversion::AttributeField& conv_field)
	{
		return ns::ActionDefs::AttributeField(conv_field.m_index, conv_field.m_name);
	}

	ns::ActionDefs::ClassField convert_class_field(
		const conv::AbstractClassAndConversion::ClassField& conv_field)
	{
		return ns::ActionDefs::ClassField(conv_field.m_index);
	}

	class PartClassFieldConverter {
		ns::ConverterFacade* const m_converter;
		const ebnf::SyntaxAndExpression* const m_main_and_expr;

	public:
		PartClassFieldConverter(ns::ConverterFacade* converter, const ebnf::SyntaxAndExpression* main_and_expr)
			: m_converter(converter),
			m_main_and_expr(main_and_expr)
		{}

		ns::ActionDefs::PartClassField operator()(const conv::AbstractClassAndConversion::PartClassField& conv_field)
			const
		{
			MPtr<const ns::PartClassTypeDescriptor> part_class_type = convert_and_part_class_type(
				m_converter,
				m_main_and_expr,
				conv_field.m_part_class_tag);
		
			return ns::ActionDefs::PartClassField(conv_field.m_index, part_class_type);
		}
	};

}

void conv::AbstractClassAndConversion::define_action(
	ns::ConverterFacade* converter,
	ns::ConvPrBuilderFacade* conv_pr_builder) const
{
	unique_ptr<const ActionDefs::AttributeVector> action_attributes = convert_fields<ActionDefs::AttributeField>(
		m_attributes.get(),
		std::ptr_fun(&convert_attribute_field));
	unique_ptr<const ActionDefs::PartClassVector> action_part_classes = convert_fields<ActionDefs::PartClassField>(
		m_part_classes.get(),
		PartClassFieldConverter(converter, m_main_and_expr));
	unique_ptr<const ActionDefs::ClassVector> action_classes = convert_fields<ActionDefs::ClassField>(
		m_classes.get(),
		std::ptr_fun(&convert_class_field));

	define_action0(
		converter,
		conv_pr_builder,
		std::move(action_attributes),
		std::move(action_part_classes),
		std::move(action_classes));
}

//
//PartClassAndConversion
//

conv::PartClassAndConversion::PartClassAndConversion(
	ebnf::SyntaxAndExpression* expr,
	const ebnf::SyntaxAndExpression* main_and_expr,
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes,
	const ns::PartClassTag& part_class_tag)
	: AbstractClassAndConversion(expr, main_and_expr, std::move(attributes), std::move(part_classes), std::move(classes)),
	m_part_class_tag(part_class_tag)
{}

void conv::PartClassAndConversion::define_action0(
	ns::ConverterFacade* converter,
	ns::ConvPrBuilderFacade* conv_pr_builder,
	unique_ptr<const ns::ActionDefs::AttributeVector> attributes,
	unique_ptr<const ns::ActionDefs::PartClassVector> part_classes,
	unique_ptr<const ns::ActionDefs::ClassVector> classes) const
{
	MPtr<const ns::PartClassTypeDescriptor> part_class_type = convert_and_part_class_type(
		converter, get_main_and_expr(), m_part_class_tag);
	PartClassActionFactory action_factory(std::move(attributes), std::move(part_classes), std::move(classes), part_class_type);
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::PartClassAndConversion::get_result_type(ns::ConverterFacade* converter) const {
	return convert_and_part_class_type(converter, get_main_and_expr(), m_part_class_tag);
}

//
//ClassAndConversion
//

conv::ClassAndConversion::ClassAndConversion(
	ebnf::SyntaxAndExpression* expr,
	const ebnf::SyntaxAndExpression* main_and_expr,
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes)
	: AbstractClassAndConversion(
		expr,
		main_and_expr,
		std::move(attributes),
		std::move(part_classes),
		std::move(classes))
{}

void conv::ClassAndConversion::define_action0(
	ns::ConverterFacade* converter,
	ns::ConvPrBuilderFacade* conv_pr_builder,
	unique_ptr<const ns::ActionDefs::AttributeVector> attributes,
	unique_ptr<const ns::ActionDefs::PartClassVector> part_classes,
	unique_ptr<const ns::ActionDefs::ClassVector> classes) const
{
	MPtr<const ClassTypeDescriptor> type = convert_expr_class_type(converter, get_main_and_expr());
	ClassActionFactory action_factory(type, std::move(attributes), std::move(part_classes), std::move(classes));
	conv_pr_builder->set_action_factory(action_factory);
}

MPtr<const ns::TypeDescriptor> conv::ClassAndConversion::get_result_type(ns::ConverterFacade* converter) const {
	MPtr<const ClassTypeDescriptor> type = convert_expr_class_type(converter, get_main_and_expr());
	return type;
}
