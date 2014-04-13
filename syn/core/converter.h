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

//Converter. Converts an EBNF grammar into a BNF grammar.

#ifndef SYN_CORE_CONVERTER_H_INCLUDED
#define SYN_CORE_CONVERTER_H_INCLUDED

#include <memory>
#include <string>

#include "action__dec.h"
#include "action_factory.h"
#include "primitives.h"
#include "descriptor_type.h"
#include "conversion__dec.h"
#include "converter__dec.h"
#include "ebnf__dec.h"
#include "noncopyable.h"
#include "types.h"

namespace synbin {
	class Converter;
}

//
//ConverterFacade
//

//Converter interface. The implementation class, Converter, is defined in the corresponding .cpp file, in order to hide
//its member variables and dependencies.
class synbin::ConverterFacade {
	NONCOPYABLE(ConverterFacade);

protected:
	ConverterFacade(){}

public:
	virtual util::MPtr<conv::ConvNt> get_empty_nonterminal() = 0;

	virtual util::MPtr<conv::ConvSym> cast_nt_to_sym(util::MPtr<conv::ConvNt> conv_nt) const = 0;

	virtual util::MPtr<conv::ConvSym> convert_expression_to_nonterminal(
		ebnf::SyntaxExpression* expr,
		util::MPtr<const TypeDescriptor> type) = 0;
	virtual void convert_expression_to_production(util::MPtr<conv::ConvNt> conv_nt, const ebnf::SyntaxExpression* expr) = 0;
	virtual util::MPtr<conv::ConvSym> convert_symbol_to_symbol(ebnf::SymbolDeclaration* sym) = 0;
	virtual util::MPtr<conv::ConvSym> convert_expression_to_symbol(ebnf::SyntaxExpression* expr) = 0;
	virtual util::MPtr<conv::ConvSym> convert_string_to_symbol(const syntax_string& str) = 0;

	virtual util::MPtr<conv::ConvNt> create_auto_nonterminal(util::MPtr<const TypeDescriptor> type) = 0;

	virtual void create_production(
		util::MPtr<conv::ConvNt> conv_nt,
		const std::vector<util::MPtr<conv::ConvSym>>& conv_elements,
		ActionFactory& action_factory) = 0;

	virtual util::MPtr<const TypeDescriptor> convert_type(const types::Type* type) = 0;
	virtual util::MPtr<const ListTypeDescriptor> create_list_type(util::MPtr<const TypeDescriptor> element_type) = 0;
	virtual util::MPtr<const PrimitiveTypeDescriptor> convert_primitive_type(const types::PrimitiveType* primitive_type) = 0;
	virtual util::MPtr<const ClassTypeDescriptor> convert_class_type(const types::ClassType* class_type) = 0;
	
	virtual util::MPtr<const PartClassTypeDescriptor> convert_part_class_type(
		util::MPtr<const ClassTypeDescriptor> class_type,
		const PartClassTag& part_class_tag) = 0;

	virtual util::MPtr<Action> manage_action(std::unique_ptr<Action> action) = 0;
	virtual util::MPtr<const TypeDescriptor> manage_type(TypeDescriptor* type) = 0;

	virtual util::MPtr<const VoidTypeDescriptor> get_void_type() = 0;
	virtual util::MPtr<const TypeDescriptor> get_symbol_type(util::MPtr<conv::ConvSym> conv_sym) const = 0;
};

//
//ConvPrBuilderFacade
//

//Interface of a BNF production builder.
class synbin::ConvPrBuilderFacade {
	NONCOPYABLE(ConvPrBuilderFacade);

protected:
	ConvPrBuilderFacade(){}

public:
	virtual void add_element(util::MPtr<conv::ConvSym> sym) = 0;
	virtual void set_action_factory(ActionFactory& action_factory) = 0;
};

#endif//SYN_CORE_CONVERTER_H_INCLUDED
