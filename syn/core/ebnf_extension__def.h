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

//EBNF extension classes definitions.

#ifndef SYN_CORE_EBNF_EXTENSION_DEF_H_INCLUDED
#define SYN_CORE_EBNF_EXTENSION_DEF_H_INCLUDED

#include <memory>
#include <vector>

#include "conversion.h"
#include "ebnf_extension__dec.h"
#include "ebnf_visitor__dec.h"
#include "noncopyable.h"
#include "types.h"
#include "util.h"
#include "util_mptr.h"

namespace synbin {

	//General data type.
	enum GeneralType {
		GENERAL_TYPE_VOID,
		GENERAL_TYPE_PRIMITIVE,
		GENERAL_TYPE_ARRAY,
		GENERAL_TYPE_CLASS
	};

}

//
//AbstractExtension
//

//Base class for EBNF extensions - for both nonterminal declaration extension and syntax expression extension.
class synbin::AbstractExtension {
	NONCOPYABLE(AbstractExtension);

	virtual void abstract_method() = 0;

	util::AssignOnce<bool> m_is_void;
	util::AssignOnce<GeneralType> m_general_type;
	util::AssignOnce<util::MPtr<const types::Type>> m_concrete_type;

public:
	AbstractExtension(){}

	void set_is_void(bool is_void);
	bool is_void_defined() const;
	bool is_void() const;
	
	void set_general_type(GeneralType general_type);
	bool general_type_defined() const;
	GeneralType get_general_type() const;
	
	void set_concrete_type(util::MPtr<const types::Type> concrete_type);
	bool concrete_type_defined() const;
	util::MPtr<const types::Type> get_concrete_type() const;
	bool is_concrete_type_set() const;
};

//
//NonterminalDeclarationExtension
//

//Extension attached to a nonterminal declaration.
class synbin::NonterminalDeclarationExtension : public synbin::AbstractExtension {
	NONCOPYABLE(NonterminalDeclarationExtension);

	void abstract_method() override{}

	bool m_visiting;
	util::AssignOnce<util::MPtr<const types::ClassType>> m_class_type;
		
public:
	NonterminalDeclarationExtension();

	bool set_visiting(bool visiting);
	void set_class_type(util::MPtr<const types::ClassType> class_type);
	util::MPtr<const types::ClassType> get_class_type() const;
	util::MPtr<const types::ClassType> get_class_type_opt() const;
};

//
//SyntaxExpressionExtension
//

//Extension attached to a syntax expression.
class synbin::SyntaxExpressionExtension : public synbin::AbstractExtension {
	NONCOPYABLE(SyntaxExpressionExtension);

	void abstract_method() override{}

	util::AssignOnce<util::MPtr<const types::Type>> m_expected_type;

	//true, if there is a 'this=' element related to this expression.
	util::AssignOnce<bool> m_and_result;

	//List of all semantic attributes related to this expression.
	//For example, for a syntax expression 'a=A (b=B | (c=C)?)', all three attributes are related to it.
	std::vector<const ebnf::NameSyntaxElement*> m_and_attributes;

	std::unique_ptr<const conv::Conversion> m_conversion;

public:
	SyntaxExpressionExtension(){}

	void set_expected_type(util::MPtr<const types::Type> type);
	util::MPtr<const types::Type> get_expected_type() const;
	void add_and_attribute(const ebnf::NameSyntaxElement* attribute);
	void add_and_attributes(const std::vector<const ebnf::NameSyntaxElement*>& attributes);
	void clear_and_attributes();
	const std::vector<const ebnf::NameSyntaxElement*>& get_and_attributes() const;
	void set_and_result(bool and_result);
	bool is_and_result() const;
	void set_conversion(std::unique_ptr<const conv::Conversion> conversion);
	const conv::Conversion* get_conversion() const;
};

//
//SyntaxAndExpressionExtension
//

//Extension attached to a syntax AND expression.
class synbin::SyntaxAndExpressionExtension {
	NONCOPYABLE(SyntaxAndExpressionExtension);

	std::unique_ptr<AndExpressionMeaning> m_meaning;

public:
	SyntaxAndExpressionExtension(){}

	void set_meaning(std::unique_ptr<AndExpressionMeaning> meaning);
	AndExpressionMeaning* get_meaning() const;
};

//
//AndExpressionMeaning
//

//Meaning of an AND syntax expression. Defines what kind of result does an AND expression produce.
class synbin::AndExpressionMeaning {
	NONCOPYABLE(AndExpressionMeaning);

	const std::vector<util::MPtr<ebnf::SyntaxExpression>> m_non_result_sub_expressions;

	virtual void visit0(AndExpressionMeaningVisitor<void>* visitor) = 0;

public:
	explicit AndExpressionMeaning(
		const std::vector<util::MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions);
	virtual ~AndExpressionMeaning();

	template<class T>
	T visit(AndExpressionMeaningVisitor<T>* visitor);
			
	void visit(AndExpressionMeaningVisitor<void>* visitor);

	const std::vector<util::MPtr<ebnf::SyntaxExpression>>& get_non_result_sub_expressions() const;
};

//
//VoidAndExpressionMeaning
//

//Void meaning. The expression does not produce any result.
class synbin::VoidAndExpressionMeaning : public synbin::AndExpressionMeaning {
	NONCOPYABLE(VoidAndExpressionMeaning);

	void visit0(AndExpressionMeaningVisitor<void>* visitor) override;

public:
	explicit VoidAndExpressionMeaning(
		const std::vector<util::MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions);
};

//
//ThisAndExpressionMeaning
//

//This meaning. The expression contains '%this=' element, which defines its result.
class synbin::ThisAndExpressionMeaning : public synbin::AndExpressionMeaning {
	NONCOPYABLE(ThisAndExpressionMeaning);

	const std::vector<util::MPtr<ebnf::ThisSyntaxElement>> m_result_elements;

	void visit0(AndExpressionMeaningVisitor<void>* visitor) override;

public:
	ThisAndExpressionMeaning(
		const std::vector<util::MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions,
		const std::vector<util::MPtr<ebnf::ThisSyntaxElement>>& result_elements
	);

	const std::vector<util::MPtr<ebnf::ThisSyntaxElement>>& get_result_elements() const;
};

//
//ClassAndExpressionMeaning
//

//Class meaning. The expression contains attribute elements, and its result is a class.
class synbin::ClassAndExpressionMeaning : public synbin::AndExpressionMeaning {
	NONCOPYABLE(ClassAndExpressionMeaning);

	const bool m_has_attributes;

	void visit0(AndExpressionMeaningVisitor<void>* visitor) override;

public:
	ClassAndExpressionMeaning(
		const std::vector<util::MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions,
		bool has_attributes
	);

	bool has_attributes() const;
};

#endif//SYN_CORE_EBNF_EXTENSION_DEF_H_INCLUDED
