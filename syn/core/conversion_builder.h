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

//Classes used to build conversion objects for AND syntax expressions.

#ifndef SYN_CORE_CONVERSION_BUILDER_H_INCLUDED
#define SYN_CORE_CONVERSION_BUILDER_H_INCLUDED

#include <cstddef>
#include <memory>

#include "action__dec.h"
#include "conversion.h"
#include "ebnf__dec.h"
#include "noncopyable.h"

namespace synbin {
	namespace conv {
	
		class AndConversionBuilder;
		class VoidAndConversionBuilder;
		class ThisAndConversionBuilder;
		class AttributeAndConversionBuilder;
		class AbstractClassAndConversionBuilder;
		class PartClassAndConversionBuilder;
		class ClassAndConversionBuilder;

		class AndBNFResult;
		class VoidAndBNFResult;
		class ThisAndBNFResult;
		class AttributeAndBNFResult;
		class AbstractClassAndBNFResult;
		class PartClassAndBNFResult;
		class ClassAndBNFResult;
	
	}
}

//
//AndConversionBuilder
//

//Base class for AND conversion builders. Callers specify elements of an AND expression, and this class decides
//which AND conversion has to be used.
class synbin::conv::AndConversionBuilder {
	NONCOPYABLE(AndConversionBuilder);

public:
	AndConversionBuilder(){}
	virtual ~AndConversionBuilder(){}

	void add_sub_element(const AndBNFResult* element_result, std::size_t element_index);

	virtual void add_sub_element_this(const ThisAndBNFResult* element_result, std::size_t element_index);
	virtual void add_sub_element_attribute(const AttributeAndBNFResult* element_result, std::size_t element_index);
	virtual void add_sub_element_part_class(const PartClassAndBNFResult* element_result, std::size_t element_index);
	virtual void add_sub_element_class(const ClassAndBNFResult* element_result, std::size_t element_index);

	//Creates a conversion object.
	virtual std::unique_ptr<const Conversion> create_conversion(ebnf::SyntaxAndExpression* expr) = 0;
};

//
//VoidAndConversionBuilder
//

class synbin::conv::VoidAndConversionBuilder : public AndConversionBuilder {
	NONCOPYABLE(VoidAndConversionBuilder);

public:
	VoidAndConversionBuilder(){}

	std::unique_ptr<const Conversion> create_conversion(ebnf::SyntaxAndExpression* expr) override;
};

//
//ThisAndConversionBuilder
//

class synbin::conv::ThisAndConversionBuilder : public AndConversionBuilder {
	NONCOPYABLE(ThisAndConversionBuilder);

	bool m_result_element_set;
	std::size_t m_result_element_index;
	const ebnf::SyntaxAndExpression* const m_main_and_expr;

public:
	ThisAndConversionBuilder(const ebnf::SyntaxAndExpression* main_and_expr);

	void add_sub_element_this(const ThisAndBNFResult* element_result, std::size_t element_index) override;
	std::unique_ptr<const Conversion> create_conversion(ebnf::SyntaxAndExpression* expr) override;
};

//
//AttributeAndConversionBuilder
//

class synbin::conv::AttributeAndConversionBuilder : public AndConversionBuilder {
	NONCOPYABLE(AttributeAndConversionBuilder);

	bool m_attribute_set;
	std::size_t m_attribute_index;
	const ebnf::NameSyntaxElement* const m_attr_expr;

public:
	AttributeAndConversionBuilder(const ebnf::NameSyntaxElement* attr_expr);

	void add_sub_element_attribute(const AttributeAndBNFResult* element_result, std::size_t element_index) override;
	std::unique_ptr<const Conversion> create_conversion(ebnf::SyntaxAndExpression* expr) override;
};

//
//AbstractClassAndConversionBuilder
//

class synbin::conv::AbstractClassAndConversionBuilder : public AndConversionBuilder {
	NONCOPYABLE(AbstractClassAndConversionBuilder);

protected:
	typedef AbstractClassAndConversion::AttributeField AttributeField;
	typedef AbstractClassAndConversion::PartClassField PartClassField;
	typedef AbstractClassAndConversion::ClassField ClassField;
	typedef AbstractClassAndConversion::AttributeVector AttributeVector;
	typedef AbstractClassAndConversion::PartClassVector PartClassVector;
	typedef AbstractClassAndConversion::ClassVector ClassVector;

private:
	std::unique_ptr<AttributeVector> m_attributes;
	std::unique_ptr<PartClassVector> m_part_classes;
	std::unique_ptr<ClassVector> m_classes;
	bool m_used;

public:
	AbstractClassAndConversionBuilder();

	void add_sub_element_attribute(const AttributeAndBNFResult* element_result, std::size_t element_index);
	void add_sub_element_part_class(const PartClassAndBNFResult* element_result, std::size_t element_index);
	void add_sub_element_class(const ClassAndBNFResult* element_result, std::size_t element_index);

private:
	std::unique_ptr<const Conversion> create_conversion(ebnf::SyntaxAndExpression* expr) override;

	virtual std::unique_ptr<const Conversion> create_conversion0(
		ebnf::SyntaxAndExpression* expr,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes) = 0;
};

//
//PartClassAndConversionBuilder
//

class synbin::conv::PartClassAndConversionBuilder : public AbstractClassAndConversionBuilder {
	NONCOPYABLE(PartClassAndConversionBuilder);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;
	const PartClassTag m_part_class_tag;

public:
	PartClassAndConversionBuilder(const ebnf::SyntaxAndExpression* main_and_expr, const PartClassTag& part_class_tag);

private:
	std::unique_ptr<const Conversion> create_conversion0(
		ebnf::SyntaxAndExpression* expr,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes) override;
};

//
//ClassAndConversionBuilder
//

class synbin::conv::ClassAndConversionBuilder : public AbstractClassAndConversionBuilder {
	NONCOPYABLE(ClassAndConversionBuilder);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;

public:
	ClassAndConversionBuilder(const ebnf::SyntaxAndExpression* main_and_expr);

private:
	std::unique_ptr<const Conversion> create_conversion0(
		ebnf::SyntaxAndExpression* expr,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes) override;
};

//
//AndBNFResult
//

//Describes what kind of result does an AND syntax expression produce.
class synbin::conv::AndBNFResult {
	NONCOPYABLE(AndBNFResult);

public:
	AndBNFResult(){}
	virtual ~AndBNFResult(){}

	virtual bool is_class_type() const { return false; }
	virtual std::unique_ptr<AndBNFResult> clone() const = 0;

	//TODO Do not create multiple instances of classes which might be made singletons (like VoidAndBNFConversion).
	virtual std::unique_ptr<AndConversionBuilder> create_and_conversion_builder() const = 0;
	virtual void add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const = 0;

	virtual std::unique_ptr<const ComplexConversionType> get_complex_conversion_type() const = 0;

	virtual std::unique_ptr<const AttributeConversion> create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const = 0;
};

//
//VoidAndBNFResult
//

//The result is nothing.
class synbin::conv::VoidAndBNFResult : public AndBNFResult {
	NONCOPYABLE(VoidAndBNFResult);

public:
	VoidAndBNFResult(){}

	std::unique_ptr<AndBNFResult> clone() const override;

	std::unique_ptr<AndConversionBuilder> create_and_conversion_builder() const override;
	void add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const override;
	
	std::unique_ptr<const ComplexConversionType> get_complex_conversion_type() const override;
	
	std::unique_ptr<const AttributeConversion> create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const override;
};

//
//ThisAndBNFResult
//

//The result of the AND expression is defined by '%this=' clause.
class synbin::conv::ThisAndBNFResult : public AndBNFResult {
	NONCOPYABLE(ThisAndBNFResult);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;

public:
	ThisAndBNFResult(const ebnf::SyntaxAndExpression* main_and_expr);

	std::unique_ptr<AndBNFResult> clone() const override;

	std::unique_ptr<AndConversionBuilder> create_and_conversion_builder() const override;
	void add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const override;
	
	std::unique_ptr<const ComplexConversionType> get_complex_conversion_type() const override;

	std::unique_ptr<const AttributeConversion> create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const override;
};

//
//AttributeAndBNFResult
//

//The AND expression contains a single attribute element, and produces a class.
class synbin::conv::AttributeAndBNFResult : public AndBNFResult {
	NONCOPYABLE(AttributeAndBNFResult);

	const ebnf::NameSyntaxElement* const m_attr_expr;

public:
	AttributeAndBNFResult(const ebnf::NameSyntaxElement* attr_expr);

	const ebnf::NameSyntaxElement* get_attr_expr() const;

	std::unique_ptr<AndBNFResult> clone() const override;

	std::unique_ptr<AndConversionBuilder> create_and_conversion_builder() const override;
	void add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const override;
	
	std::unique_ptr<const ComplexConversionType> get_complex_conversion_type() const override;
	
	std::unique_ptr<const AttributeConversion> create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const override;
};

//
//AbstractClassAndBNFResult
//

//The AND expression contains more than one attribute element, and produces a class.
class synbin::conv::AbstractClassAndBNFResult : public AndBNFResult {
	NONCOPYABLE(AbstractClassAndBNFResult);

public:
	AbstractClassAndBNFResult(){}

	bool is_class_type() const override { return true; }
};

//
//PartClassAndBNFResult
//

class synbin::conv::PartClassAndBNFResult : public AbstractClassAndBNFResult {
	NONCOPYABLE(PartClassAndBNFResult);

	const ebnf::SyntaxAndExpression* const m_main_and_expr;
	const PartClassTag m_part_class_tag;

public:
	PartClassAndBNFResult(const ebnf::SyntaxAndExpression* main_and_expr, const PartClassTag& part_class_tag);

	const PartClassTag& get_part_class_tag() const;

	std::unique_ptr<AndBNFResult> clone() const override;

	std::unique_ptr<AndConversionBuilder> create_and_conversion_builder() const override;
	void add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const override;
	
	std::unique_ptr<const ComplexConversionType> get_complex_conversion_type() const override;
	
	std::unique_ptr<const AttributeConversion> create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const override;
};

//
//ClassAndBNFResult
//

class synbin::conv::ClassAndBNFResult : public AbstractClassAndBNFResult {
	NONCOPYABLE(ClassAndBNFResult);
	
	const ebnf::SyntaxAndExpression* const m_main_and_expr;

public:
	ClassAndBNFResult(const ebnf::SyntaxAndExpression* main_and_expr);

	std::unique_ptr<AndBNFResult> clone() const override;

	std::unique_ptr<AndConversionBuilder> create_and_conversion_builder() const override;
	void add_to_builder(AndConversionBuilder* conversion_builder, std::size_t element_index) const override;
	
	std::unique_ptr<const ComplexConversionType> get_complex_conversion_type() const override;
	
	std::unique_ptr<const AttributeConversion> create_attribute_conversion(ebnf::NameSyntaxElement* attr_expr) const override;
};

#endif//SYN_CORE_CONVERSION_BUILDER_H_INCLUDED
