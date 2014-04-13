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

//Conversions - classes describing how to convert a particular EBNF expression into a BNF grammar symbol.

#ifndef SYN_CORE_CONVERSION_H_INCLUDED
#define SYN_CORE_CONVERSION_H_INCLUDED

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "action__dec.h"
#include "action_factory.h"
#include "conversion__dec.h"
#include "converter__dec.h"
#include "descriptor_type.h"
#include "ebnf__dec.h"
#include "noncopyable.h"
#include "util_mptr.h"

//
//PartClassTag
//

namespace synbin {
	//Used to distinguish one part class from another.
	class PartClassTag {
		std::size_t m_index;

	public:
		explicit PartClassTag(std::size_t index) : m_index(index){}
		std::size_t get_index() const { return m_index; }
	};
}

//
//ComplexConversionType
//

//Context of a complex conversion. See comments in conversion__dec.h.
class synbin::conv::ComplexConversionType {
	NONCOPYABLE(ComplexConversionType);

protected:
	ComplexConversionType(){}
	
public:
	virtual ~ComplexConversionType(){}

	//Returns true, if the conversion is used in a "dead" context, i. e. the value produced by the syntax expression
	//is not used.
	virtual bool is_dead() const { return false; }

	//Returns true, if the context of the conversion is AndConversion or its subclass.
	virtual bool is_and_conversion() const { return false; }

	//Returns the type descriptor of the type produced by the syntax expression.
	virtual util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter, ebnf::SyntaxExpression* expr) const = 0;
};

//
//TopComplexConversionType
//

//The syntax expression is a top-level expression (N: E).
class synbin::conv::TopComplexConversionType : public ComplexConversionType {
	NONCOPYABLE(TopComplexConversionType);

public:
	TopComplexConversionType(){}

	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter, ebnf::SyntaxExpression* expr) const override;
};

//
//DeadComplexConversionType
//

//Dead syntax expression - the produced value is not used (N: a=A b=B (C) ===> 'C' is dead).
class synbin::conv::DeadComplexConversionType : public ComplexConversionType {
	NONCOPYABLE(DeadComplexConversionType);

public:
	DeadComplexConversionType(){}

	bool is_dead() const override { return true; }
	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter, ebnf::SyntaxExpression* expr) const override;
};

//
//AndComplexConversionType
//

//The syntax expression is an element of an AND expression (N: A B C).
class synbin::conv::AndComplexConversionType : public ComplexConversionType {
	NONCOPYABLE(AndComplexConversionType);

protected:
	AndComplexConversionType(){}

public:
	bool is_and_conversion() const override { return true; }
};

//
//ThisAndComplexConversionType
//

//The syntax expression is an element of an AND expression which contains '%this=' element (N: A %this=B C).
class synbin::conv::ThisAndComplexConversionType : public AndComplexConversionType {
	NONCOPYABLE(ThisAndComplexConversionType);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;

public:
	ThisAndComplexConversionType(const ebnf::SyntaxAndExpression* main_and_expr);

	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter, ebnf::SyntaxExpression* expr) const override;
};

//
//AttrAndComplexConversionType
//

//The syntax expression is an element of an AND expression which contains attribute elements (N: a=A b=B).
class synbin::conv::AttrAndComplexConversionType : public AndComplexConversionType {
	NONCOPYABLE(AttrAndComplexConversionType);

	const ebnf::NameSyntaxElement* const m_attr_expr;

public:
	AttrAndComplexConversionType(const ebnf::NameSyntaxElement* attr_expr);

	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter, ebnf::SyntaxExpression* expr) const override;
};

//
//PartClassAndComplexConversionType
//

//The syntax expression is an element of an AND expression, and it must produce a part-class (N: (a=A b=B) (c=C d=D) ===>
//the type of '(c=C d=D)' is a part-class-and.
class synbin::conv::PartClassAndComplexConversionType : public AndComplexConversionType {
	NONCOPYABLE(PartClassAndComplexConversionType);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;
	const PartClassTag m_part_class_tag;

public:
	PartClassAndComplexConversionType(const ebnf::SyntaxAndExpression* main_and_expr, const PartClassTag& part_class_tag);

	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter, ebnf::SyntaxExpression* expr) const override;
};

//
//ClassAndComplexConversionType
//

//The syntax expression is an element of an AND expression, and it must produce a class object (N: X (a=A b=B) Y ===>
//the type of '(a=A b=B)' is a class-and.
class synbin::conv::ClassAndComplexConversionType : public AndComplexConversionType {
	NONCOPYABLE(ClassAndComplexConversionType);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;

public:
	ClassAndComplexConversionType(const ebnf::SyntaxAndExpression* main_and_expr);

	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter, ebnf::SyntaxExpression* expr) const override;
};

//
//Conversion
//

//Describes how to convert a particular syntax expression into a BNF symbol. See comments in conversion__dec.h.
class synbin::conv::Conversion {
	NONCOPYABLE(Conversion);

public:
	Conversion();
	virtual ~Conversion(){}

	//Returns the associated syntax expression. Each subclass holds its own reference to the expression of the
	//corresponding type.
	virtual ebnf::SyntaxExpression* get_expr() const = 0;
	
	//Converts the associated EBNF syntax expression into a BNF nonterminal symbol. The BNF nonterminal symbol is
	//provided by the caller. The function adds productions to that nonterminal.
	virtual void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const = 0;

	//Converts the associated EBNF syntax expression into a BNF production.
	virtual void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const = 0;

	//Converts the associated EBNF syntax expression into a BNF symbol.
	virtual util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const = 0;

protected:
	//Delegates convert_nt() to convert_pr(), i. e. the syntax expression is converted into a single BNF production,
	//and that production is added to the passed BNF nonterminal symbol.
	void delegate_nt_to_pr(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const;

	//Delegates convert_pr() to convert_sym() by converting the associated EBNF expression into a single BNF symbol,
	//and creating a BNF production out of that single symbol.
	void delegate_pr_to_sym(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder, bool is_dead) const;

	//Delegates convert_sym() to convert_nt() by creating a new BNF nonterminal symbol.
	util::MPtr<ConvSym> delegate_sym_to_nt(synbin::ConverterFacade* converter, util::MPtr<const TypeDescriptor> nt_type) const;
};

//
//EmptyConversion
//

class synbin::conv::EmptyConversion : public Conversion {
	NONCOPYABLE(EmptyConversion);

	ebnf::EmptySyntaxExpression* const m_expr;

public:
	EmptyConversion(ebnf::EmptySyntaxExpression* expr);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//ConstConversion
//

class synbin::conv::ConstConversion : public Conversion {
	NONCOPYABLE(ConstConversion);

	ebnf::ConstSyntaxExpression* const m_expr;

public:
	ConstConversion(ebnf::ConstSyntaxExpression* expr);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//CastConversion
//

class synbin::conv::CastConversion : public Conversion {
	NONCOPYABLE(CastConversion);

	ebnf::CastSyntaxExpression* const m_expr;

public:
	CastConversion(ebnf::CastSyntaxExpression* expr);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//ThisConversion
//

class synbin::conv::ThisConversion : public Conversion {
	NONCOPYABLE(ThisConversion);

	ebnf::ThisSyntaxElement* const m_expr;

public:
	ThisConversion(ebnf::ThisSyntaxElement* expr);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//SimpleConversion
//

class synbin::conv::SimpleConversion : public Conversion {
	NONCOPYABLE(SimpleConversion);

protected:
	const SimpleConversionType m_conversion_type;

protected:
	SimpleConversion(SimpleConversionType conversion_type);

	void delegate_pr_to_sym0(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const;
};

//
//NameConversion
//

class synbin::conv::NameConversion : public SimpleConversion {
	NONCOPYABLE(NameConversion);

	ebnf::NameSyntaxExpression* const m_expr;

public:
	NameConversion(ebnf::NameSyntaxExpression* expr, SimpleConversionType conversion_type);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//StringConversion
//

class synbin::conv::StringConversion : public SimpleConversion {
	NONCOPYABLE(StringConversion);

	ebnf::StringSyntaxExpression* const m_expr;

public:
	StringConversion(ebnf::StringSyntaxExpression* expr, SimpleConversionType conversion_type);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//LoopConversion
//

class synbin::conv::LoopConversion : public SimpleConversion {
	NONCOPYABLE(LoopConversion);

protected:
	ebnf::LoopSyntaxExpression* const m_expr;

public:
	LoopConversion(ebnf::LoopSyntaxExpression* expr, SimpleConversionType conversion_type);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//ZeroManyConversion
//

class synbin::conv::ZeroManyConversion : public LoopConversion {
	NONCOPYABLE(ZeroManyConversion);

public:
	ZeroManyConversion(ebnf::ZeroManySyntaxExpression* expr, SimpleConversionType conversion_type);

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
};

//
//OneManyConversion
//

class synbin::conv::OneManyConversion : public LoopConversion {
	NONCOPYABLE(OneManyConversion);

public:
	OneManyConversion(ebnf::OneManySyntaxExpression* expr, SimpleConversionType conversion_type);

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
};

//
//ComplexConversion
//

//Complex conversion is associated with a syntax expression that belongs to (contained by) an AND expression,
//and attributes and 'this' elements of the inner syntax expression belong to the outer AND expression
//(N: (a=A | b=B) (c=C)? d=D ===> all four attribute expressions belong to the top-level AND expression).
class synbin::conv::ComplexConversion : public Conversion {
	NONCOPYABLE(ComplexConversion);

	const std::unique_ptr<const ComplexConversionType> m_conversion_type;
	const ebnf::SyntaxAndExpression* const m_and_expr;//can be 0.

protected:
	ComplexConversion(
		std::unique_ptr<const ComplexConversionType> conversion_type,
		const ebnf::SyntaxAndExpression* and_expr);

	void delegate_pr_to_sym0(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const;

	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter) const;
};

//
//OrConversion
//

class synbin::conv::OrConversion : public ComplexConversion {
	NONCOPYABLE(OrConversion);

	ebnf::SyntaxOrExpression* const m_expr;

public:
	OrConversion(
		std::unique_ptr<const ComplexConversionType> conversion_type,
		const ebnf::SyntaxAndExpression* and_expr,
		ebnf::SyntaxOrExpression* expr);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//ZeroOneConversion
//

class synbin::conv::ZeroOneConversion : public ComplexConversion {
	NONCOPYABLE(ZeroOneConversion);

	ebnf::ZeroOneSyntaxExpression* const m_expr;

public:
	ZeroOneConversion(
		std::unique_ptr<const ComplexConversionType> conversion_type,
		const ebnf::SyntaxAndExpression* and_expr,
		ebnf::ZeroOneSyntaxExpression* expr);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;
};

//
//AttributeConversion
//

class synbin::conv::AttributeConversion : public Conversion {
	NONCOPYABLE(AttributeConversion);

protected:
	ebnf::NameSyntaxElement* const m_expr;

protected:
	AttributeConversion(ebnf::NameSyntaxElement* expr);

	ebnf::SyntaxExpression* get_expr() const override;

	void convert_nt(ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;

private:
	virtual void define_action_factory(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const = 0;
	virtual util::MPtr<const TypeDescriptor> get_result_type(synbin::ConverterFacade* converter) const = 0;
};

//
//TopAttributeConversion
//

class synbin::conv::TopAttributeConversion : public AttributeConversion {
	NONCOPYABLE(TopAttributeConversion);

public:
	TopAttributeConversion(ebnf::NameSyntaxElement* expr);

private:
	void define_action_factory(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<const TypeDescriptor> get_result_type(synbin::ConverterFacade* converter) const override;
};

//
//AttrAndAttributeConversion
//

class synbin::conv::AttrAndAttributeConversion : public AttributeConversion {
	NONCOPYABLE(AttrAndAttributeConversion);

public:
	AttrAndAttributeConversion(ebnf::NameSyntaxElement* expr);

	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const;

private:
	void define_action_factory(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<const TypeDescriptor> get_result_type(synbin::ConverterFacade* converter) const override;
};

//
//PartClassAndAttributeConversion
//

class synbin::conv::PartClassAndAttributeConversion : public AttributeConversion {
	NONCOPYABLE(PartClassAndAttributeConversion);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;
	const PartClassTag m_part_class_tag;

public:
	PartClassAndAttributeConversion(
		ebnf::NameSyntaxElement* expr,
		const ebnf::SyntaxAndExpression* main_and_expr,
		const PartClassTag& part_class_tag);

private:
	void define_action_factory(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<const TypeDescriptor> get_result_type(synbin::ConverterFacade* converter) const override;
};

//
//ClassAndAttributeConversion
//

class synbin::conv::ClassAndAttributeConversion : public AttributeConversion {
	NONCOPYABLE(ClassAndAttributeConversion);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;

public:
	ClassAndAttributeConversion(ebnf::NameSyntaxElement* expr, const ebnf::SyntaxAndExpression* main_and_expr);

private:
	void define_action_factory(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<const TypeDescriptor> get_result_type(synbin::ConverterFacade* converter) const override;
};

//
//AndConversion
//

//Base class for conversions associated with an AND syntax expression.
class synbin::conv::AndConversion : public Conversion {
	NONCOPYABLE(AndConversion);

protected:
	ebnf::SyntaxAndExpression* const m_expr;

public:
	AndConversion(ebnf::SyntaxAndExpression* expr);

	ebnf::SyntaxExpression* get_expr() const override final;

	void convert_nt(synbin::ConverterFacade* converter, util::MPtr<ConvNt> conv_nt) const override;
	void convert_pr(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<ConvSym> convert_sym(synbin::ConverterFacade* converter) const override;

private:
	virtual void define_action(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const = 0;
	virtual util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter) const = 0;
};

//
//VoidAndConversion
//

//This conversion is associated with an AND expression that produces nothing at run-time.
class synbin::conv::VoidAndConversion : public AndConversion {
	NONCOPYABLE(VoidAndConversion);

public:
	VoidAndConversion(ebnf::SyntaxAndExpression* expr);

private:
	void define_action(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter) const override;
};

//
//ThisAndConversion
//

//Associated with an AND expression procuding a result specified by a '%this=' element.
class synbin::conv::ThisAndConversion : public AndConversion {
	NONCOPYABLE(ThisAndConversion);

	const std::size_t m_result_index;
	const ebnf::SyntaxAndExpression* const m_main_and_expr;

public:
	ThisAndConversion(ebnf::SyntaxAndExpression* expr, std::size_t result_index, const ebnf::SyntaxAndExpression* main_and_expr);

private:
	void define_action(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter) const override;
};

//
//AttributeAndConversion
//

//Associated with an AND expression containing a single attribute element. The result BNF symbol produces an object of a class.
class synbin::conv::AttributeAndConversion : public AndConversion {
	NONCOPYABLE(AttributeAndConversion);

	const std::size_t m_attribute_index;
	const ebnf::NameSyntaxElement* const m_attr_expr;

public:
	AttributeAndConversion(
		ebnf::SyntaxAndExpression* expr,
		std::size_t attribute_index,
		const ebnf::NameSyntaxElement* attr_expr);

private:
	void define_action(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter) const override;
};

//
//AbstractClassAndConversion
//

//Associated with an AND expression producing a class object.
class synbin::conv::AbstractClassAndConversion : public AndConversion {
	NONCOPYABLE(AbstractClassAndConversion);

public:
	struct Field {
		const std::size_t m_index;

		Field(std::size_t index) : m_index(index){}
	};

	struct AttributeField : public Field {
		const util::String m_name;

		AttributeField(std::size_t index, const util::String& name) : Field(index), m_name(name){}
	};

	struct PartClassField : public Field {
		const PartClassTag m_part_class_tag;

		PartClassField(std::size_t index, const PartClassTag& part_class_tag)
			: Field(index), m_part_class_tag(part_class_tag){}
	};

	struct ClassField : public Field {
		ClassField(std::size_t index) : Field(index){}
	};

	typedef std::vector<AttributeField> AttributeVector;
	typedef std::vector<PartClassField> PartClassVector;
	typedef std::vector<ClassField> ClassVector;

private:
	const ebnf::SyntaxAndExpression* const m_main_and_expr;
	std::unique_ptr<const AttributeVector> m_attributes;
	std::unique_ptr<const PartClassVector> m_part_classes;
	std::unique_ptr<const ClassVector> m_classes;

protected:
	AbstractClassAndConversion(
		ebnf::SyntaxAndExpression* expr,
		const ebnf::SyntaxAndExpression* main_and_expr,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes);

	const ebnf::SyntaxAndExpression* get_main_and_expr() const;

private:
	void define_action(synbin::ConverterFacade* converter, ConvPrBuilderFacade* conv_pr_builder) const override;
	
	virtual void define_action0(
		synbin::ConverterFacade* converter,
		ConvPrBuilderFacade* conv_pr_builder,
		std::unique_ptr<const ActionDefs::AttributeVector> attributes,
		std::unique_ptr<const ActionDefs::PartClassVector> part_classes,
		std::unique_ptr<const ActionDefs::ClassVector> classes) const = 0;
};

//
//PartClassAndConversion
//

class synbin::conv::PartClassAndConversion : public AbstractClassAndConversion {
	NONCOPYABLE(PartClassAndConversion);

	const PartClassTag m_part_class_tag;

public:
	PartClassAndConversion(
		ebnf::SyntaxAndExpression* expr,
		const ebnf::SyntaxAndExpression* main_and_expr,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes,
		const PartClassTag& part_class_tag);

private:
	void define_action0(
		synbin::ConverterFacade* converter,
		ConvPrBuilderFacade* conv_pr_builder,
		std::unique_ptr<const ActionDefs::AttributeVector> attributes,
		std::unique_ptr<const ActionDefs::PartClassVector> part_classes,
		std::unique_ptr<const ActionDefs::ClassVector> classes) const override;

	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter) const override;
};

//
//ClassAndConversion
//

class synbin::conv::ClassAndConversion : public AbstractClassAndConversion {
	NONCOPYABLE(ClassAndConversion);

public:
	ClassAndConversion(
		ebnf::SyntaxAndExpression* expr,
		const ebnf::SyntaxAndExpression* main_and_expr,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes);

private:
	void define_action0(
		synbin::ConverterFacade* converter,
		ConvPrBuilderFacade* conv_pr_builder,
		std::unique_ptr<const ActionDefs::AttributeVector> attributes,
		std::unique_ptr<const ActionDefs::PartClassVector> part_classes,
		std::unique_ptr<const ActionDefs::ClassVector> classes) const override;

	util::MPtr<const TypeDescriptor> get_result_type(ConverterFacade* converter) const override;
};

#endif//SYN_CORE_CONVERSION_H_INCLUDED
