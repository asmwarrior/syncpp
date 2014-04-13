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

//Part of EBNF Grammar Builder responsible for the processing of semantic attributes.

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "commons.h"
#include "conversion.h"
#include "conversion_builder.h"
#include "ebnf__imp.h"
#include "ebnf_bld_common.h"
#include "ebnf_builder.h"
#include "ebnf_visitor__imp.h"
#include "util.h"
#include "util_mptr.h"
#include "util_string.h"

namespace ns = synbin;
namespace conv = ns::conv;
namespace ebnf = ns::ebnf;
namespace util = ns::util;

using std::unique_ptr;
using util::MPtr;
using util::MContainer;
using util::String;

namespace {

	typedef std::vector<String> StringVector;
	typedef std::vector<const ebnf::NameSyntaxElement*> AttrExprVector;

	typedef util::MPtr<const ns::PartClassTypeDescriptor> PartClassTypeMPtr;

	typedef unique_ptr<const conv::ComplexConversionType> ComplexConversionTypeAutoPtr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//AttributeScope
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	//
	//RootScope
	//

	class RootScope {
		NONCOPYABLE(RootScope);

		bool m_has_result;
		bool m_has_attribute;

	public:
		RootScope()
			: m_has_result(false), m_has_attribute(false)
		{}

		bool has_result() const { return m_has_result; }
		bool has_attribute() const { return m_has_attribute; }

		void set_has_result() { m_has_result = true; }
		void set_has_attribute() { m_has_attribute = true; }
	};

	//
	//AttributeScope
	//

	class AttributeScope {
		NONCOPYABLE(AttributeScope);

		RootScope* const m_root_scope;
		AttributeScope* const m_parent_scope;
		StringVector m_attributes;
		
		//'this=' elements specified in this scope and nested scopes.
		std::vector<util::MPtr<ebnf::ThisSyntaxElement>> m_result_elements;
		
		std::vector<util::MPtr<ebnf::SyntaxExpression>> m_non_result_sub_expressions;

	public:
		explicit AttributeScope(RootScope* root_scope)
			: m_root_scope(root_scope),
			m_parent_scope(nullptr)
		{
			assert(root_scope);
		}

		explicit AttributeScope(AttributeScope* parent_scope)
			: m_root_scope(parent_scope ? parent_scope->m_root_scope : nullptr),
			m_parent_scope(parent_scope)
		{}

		const StringVector& get_attributes() {
			return m_attributes;
		}
		
		const std::vector<util::MPtr<ebnf::ThisSyntaxElement>>& get_result_elements() {
			return m_result_elements;
		}
		
		const std::vector<util::MPtr<ebnf::SyntaxExpression>>& get_non_result_sub_expressions() {
			return m_non_result_sub_expressions;
		}

		void add_attribute(const ns::syntax_string& name) {
			const String str = name.get_string();

			assert(!m_root_scope->has_result());
			m_root_scope->set_has_attribute();

			//Check name conflict.
			AttributeScope* scope = this;
			while (scope) {
				for (std::size_t i = 0, size = scope->m_attributes.size(); i < size; ++i) {
					if (str == scope->m_attributes[i]) {
						throw ns::raise_error(name, "Attribute name conflict: '" + str.str() + "'");
					}
				}
				scope = scope->m_parent_scope;
			}

			//Add the attribute.
			m_attributes.push_back(name.get_string());
		}

		void set_result_element(ebnf::ThisSyntaxElement* result_element) {
			assert(!m_root_scope->has_attribute());
			m_root_scope->set_has_result();

			//Check this element conflict.
			AttributeScope* scope = this;
			while (scope) {
				if (!scope->m_result_elements.empty()) {
					throw ns::raise_error(result_element->get_pos(), "Result element conflict: '%this'");
				}
				scope = scope->m_parent_scope;
			}

			//Add the element.
			m_result_elements.push_back(MPtr<ebnf::ThisSyntaxElement>::unsafe_cast(result_element));
		}

		void add_non_result_sub_expression(ebnf::SyntaxExpression* expression) {
			m_non_result_sub_expressions.push_back(MPtr<ebnf::SyntaxExpression>::unsafe_cast(expression));
		}

		void add_scope(AttributeScope* scope) {
			m_attributes.insert(
				m_attributes.end(),
				scope->m_attributes.begin(),
				scope->m_attributes.end()
			);
			m_result_elements.insert(
				m_result_elements.end(),
				scope->m_result_elements.begin(),
				scope->m_result_elements.end()
			);
			m_non_result_sub_expressions.insert(
				m_non_result_sub_expressions.end(),
				scope->m_non_result_sub_expressions.begin(),
				scope->m_non_result_sub_expressions.end()
			);
		}
	};

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//init_subtree_attributes, clear_subtree_attributes
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	//Recursively goes through all the sub-expressions of the given expression (inclusively), and
	//initializes the list of attributes for each expression.
	void init_subtree_attributes(
		MContainer<const StringVector>* managed_attr_vectors,
		ebnf::SyntaxAndExpression* top_expr)
	{
		struct AttributesFindingVisitor : public ns::SyntaxExpressionVisitor<void> {
			bool m_has_attributes;
			bool m_has_this;

			AttributesFindingVisitor() : m_has_attributes(false), m_has_this(false){}

			void visit_SyntaxExpression(ebnf::SyntaxExpression* expr) override {
				ns::SyntaxExpressionExtension* ext = expr->get_extension();
				ext->set_and_result(false);
			}

			void visit_CompoundSyntaxExpression(ebnf::CompoundSyntaxExpression* expr) override {
				ns::SyntaxExpressionExtension* ext = expr->get_extension();
				bool and_result = false;
				for (MPtr<ebnf::SyntaxExpression> sub_expr : expr->get_sub_expressions()) {
					sub_expr->visit(this);
					ns::SyntaxExpressionExtension* sub_ext = sub_expr->get_extension();
					ext->add_and_attributes(sub_ext->get_and_attributes());
					and_result = and_result || sub_ext->is_and_result();
				}
				ext->set_and_result(and_result);
			}
		
			void visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) override {
				if (m_has_this) {
					const ns::syntax_string& name = expr->get_name();
					throw ns::raise_error(name, "Attribute and '%this' conflict: '" + name.str() + "'");
				}

				m_has_attributes = true;
				ns::SyntaxExpressionExtension* ext = expr->get_extension();
				ext->add_and_attribute(expr);
				ext->set_and_result(false);
			}
		
			void visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) override {
				if (m_has_attributes) throw ns::raise_error(expr->get_pos(), "Attribute and '%this' conflict");
				m_has_this = true;
				ns::SyntaxExpressionExtension* ext = expr->get_extension();
				ext->set_and_result(true);
			}
		
			void visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override {
				ebnf::SyntaxExpression* sub_expr = expr->get_sub_expression();
				sub_expr->visit(this);
				ns::SyntaxExpressionExtension* ext = expr->get_extension();
				ns::SyntaxExpressionExtension* sub_ext = sub_expr->get_extension();
				ext->add_and_attributes(sub_ext->get_and_attributes());
				ext->set_and_result(sub_ext->is_and_result());
			}
		};

		AttributesFindingVisitor visitor;
		top_expr->visit(&visitor);
	}

	//Recursively clears vectors of attributes for the given syntax expression and its sub-expressions
	//(in order to not occupy memory when it is not needed).
	void clear_subtree_attributes(ebnf::SyntaxAndExpression* top_expr) {
		class AttributesClearingVisitor : public ns::SyntaxExpressionVisitor<void> {
		public:
			void visit_CompoundSyntaxExpression(ebnf::CompoundSyntaxExpression* expr) override {
				for (MPtr<ebnf::SyntaxExpression> sub_expr : expr->get_sub_expressions()) {
					sub_expr->visit(this);
				}
				expr->get_extension()->clear_and_attributes();
			}
		
			void visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) override {
				expr->get_extension()->clear_and_attributes();
			}
		
			void visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override {
				expr->get_sub_expression()->visit(this);
				expr->get_extension()->clear_and_attributes();
			}
		};

		AttributesClearingVisitor visitor;
		top_expr->visit(&visitor);
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Commons
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	typedef unique_ptr<const conv::Conversion> ConversionAutoPtr;
	typedef unique_ptr<conv::AndBNFResult> AndBNFResultAutoPtr;
	typedef unique_ptr<conv::AndConversionBuilder> AndConversionBuilderAutoPtr;

	void set_conversion(ebnf::SyntaxExpression* expr, conv::Conversion* conversion) {
		expr->get_extension()->set_conversion(ConversionAutoPtr(conversion));
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//AttributeVerificationContext
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	class AttributeVerificationContext {
		NONCOPYABLE(AttributeVerificationContext);

		std::vector<ns::PartClassTag>* const m_part_class_tags;
		const MPtr<MContainer<ns::PartClassTypeDescriptor>> m_managed_part_class_types;
		ns::FilePos m_pos;

	public:
		AttributeVerificationContext(
			std::vector<ns::PartClassTag>* part_class_tags,
			MPtr<MContainer<ns::PartClassTypeDescriptor>> managed_part_class_types)
			: m_part_class_tags(part_class_tags),
			m_managed_part_class_types(managed_part_class_types)
		{}
	
		void verify_attributes_top(ebnf::SyntaxExpression* expr);
		void verify_attributes_top_nt(ebnf::NonterminalDeclaration* nt);
	
		void verify_attributes_and(
			const ebnf::SyntaxAndExpression* main_and_expr,
			ebnf::SyntaxExpression* expr,
			AttributeScope* scope,
			conv::AndBNFResult* and_bnf_result);
	
		void verify_attributes_dead(ebnf::SyntaxExpression* expr);

		AndBNFResultAutoPtr process_and_sub_expression(
			const ebnf::SyntaxAndExpression* main_and_expr,
			AttributeScope* scope,
			conv::AndBNFResult* and_bnf_result,
			ebnf::SyntaxExpression* sub_expr,
			std::size_t n_sub_exprs_with_attr);

		void process_and_expression(
			const ebnf::SyntaxAndExpression* main_and_expr,
			AttributeScope* scope,
			conv::AndBNFResult* and_bnf_result,
			ebnf::SyntaxAndExpression* expr);
	};

	unique_ptr<const std::vector<String>> attributes_to_names(const AttrExprVector& attrs) {
		unique_ptr<std::vector<String>> names = ns::make_unique1<std::vector<String>>();
		for (std::size_t i = 0, n = attrs.size(); i < n; ++i) {
			const String& name = attrs[i]->get_name().get_string();
			if (std::find(names->begin(), names->end(), name) == names->end()) {
				names->push_back(name);
			}
		}
		return util::const_vector_ptr(std::move(names));
	}

}

AndBNFResultAutoPtr AttributeVerificationContext::process_and_sub_expression(
	const ebnf::SyntaxAndExpression* main_and_expr,
	AttributeScope* scope,
	conv::AndBNFResult* and_bnf_result,
	ebnf::SyntaxExpression* sub_expr,
	std::size_t n_sub_exprs_with_attr)
{
	const ns::SyntaxExpressionExtension* sub_ext = sub_expr->get_extension();
	if (sub_ext->is_and_result()) {
		AndBNFResultAutoPtr sub_bnf_result = ns::make_unique1<conv::ThisAndBNFResult>(main_and_expr);
		verify_attributes_and(main_and_expr, sub_expr, scope, sub_bnf_result.get());
		return sub_bnf_result;
	}

	const AttrExprVector& sub_and_attributes = sub_ext->get_and_attributes();
	if (sub_and_attributes.empty()) {
		AndBNFResultAutoPtr sub_bnf_result = ns::make_unique1<conv::VoidAndBNFResult>();
		verify_attributes_dead(sub_expr);
		return sub_bnf_result;
	}

	AndBNFResultAutoPtr sub_bnf_result;

	if (1 == sub_and_attributes.size()) {
		const ebnf::NameSyntaxElement* attr_expr = sub_and_attributes[0];
		sub_bnf_result = ns::make_unique1<conv::AttributeAndBNFResult>(attr_expr);
	} else if (1 == n_sub_exprs_with_attr) {
		//This is the only sub-expression with attributes, therefore it must return the same result as its
		//parent expression.
		assert(and_bnf_result->is_class_type());

		//TODO Do not clone - use the same object instead. But unique_ptr must not be used then.
		//Use a shared pointer or MPtr,	etc.
		sub_bnf_result = and_bnf_result->clone();
	} else {
		unique_ptr<const std::vector<String>> names = attributes_to_names(sub_and_attributes);
		ns::PartClassTag part_class_tag(m_part_class_tags->size());
		m_part_class_tags->push_back(part_class_tag);
		sub_bnf_result = ns::make_unique1<conv::PartClassAndBNFResult>(main_and_expr, part_class_tag);
	}

	verify_attributes_and(main_and_expr, sub_expr, scope, sub_bnf_result.get());

	return sub_bnf_result;
}

void AttributeVerificationContext::process_and_expression(
	const ebnf::SyntaxAndExpression* main_and_expr,
	AttributeScope* scope,
	conv::AndBNFResult* and_bnf_result,
	ebnf::SyntaxAndExpression* expr)
{
	const std::vector<util::MPtr<ebnf::SyntaxExpression>>& sub_expressions = expr->get_sub_expressions();

	//Calculate how many sub-expressions with attributes are there.
	std::size_t n_sub_exprs_with_attr = 0;
	for (MPtr<ebnf::SyntaxExpression> sub_expr : sub_expressions) {
		if (!sub_expr->get_extension()->get_and_attributes().empty()) ++n_sub_exprs_with_attr;
	}

	//Process sub-expressions.
	AndConversionBuilderAutoPtr conversion_builder = and_bnf_result->create_and_conversion_builder();
	
	std::size_t idx = 0;
	for (MPtr<ebnf::SyntaxExpression> sub_expr : sub_expressions) {
		//Verify the sub-expression.
		AndBNFResultAutoPtr sub_bnf_result = process_and_sub_expression(
			main_and_expr,
			scope,
			and_bnf_result,
			sub_expr.get(),
			n_sub_exprs_with_attr);
			
		//Add the result of the sub-expression into the result of this expression.
		conversion_builder->add_sub_element(sub_bnf_result.get(), idx);
		++idx;
	}

	ConversionAutoPtr conversion = conversion_builder->create_conversion(expr);
	expr->get_extension()->set_conversion(std::move(conversion));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//AttributesVerificationVisitor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	class AttributesVerificationVisitor : public ns::SyntaxExpressionVisitor<void> {
		NONCOPYABLE(AttributesVerificationVisitor);

	protected:
		AttributeVerificationContext* const m_verification_context;
		const ns::FilePos m_pos;

	public:
		AttributesVerificationVisitor(AttributeVerificationContext* verification_context, const ns::FilePos& pos);
	};

}

AttributesVerificationVisitor::AttributesVerificationVisitor(
	AttributeVerificationContext* verification_context,
	const ns::FilePos& pos)
: m_verification_context(verification_context),
m_pos(pos)
{}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//BasicAttributesVerificationVisitor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	class BasicAttributesVerificationVisitor : public AttributesVerificationVisitor {
		NONCOPYABLE(BasicAttributesVerificationVisitor);

	public:
		BasicAttributesVerificationVisitor(AttributeVerificationContext* verification_context, const ns::FilePos& pos);

		void visit_SyntaxExpression(ebnf::SyntaxExpression* expr) override;
		void visit_EmptySyntaxExpression(ebnf::EmptySyntaxExpression* expr) override;
		void visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) override;
		void visit_NameSyntaxExpression(ebnf::NameSyntaxExpression* expr) override;
		void visit_StringSyntaxExpression(ebnf::StringSyntaxExpression* expr) override;
		void visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override;
		void visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) override;
		void visit_ZeroManySyntaxExpression(ebnf::ZeroManySyntaxExpression* expr) override;
		void visit_OneManySyntaxExpression(ebnf::OneManySyntaxExpression* expr) override;

	private:
		virtual void verify_sub_expr_attributes(ebnf::SyntaxExpression* expr) = 0;
		virtual conv::SimpleConversionType get_simple_conversion_type() const = 0;
		virtual ComplexConversionTypeAutoPtr get_complex_conversion_type() const = 0;
	};

}

BasicAttributesVerificationVisitor::BasicAttributesVerificationVisitor(
	AttributeVerificationContext* verification_context,
	const ns::FilePos& pos)
: AttributesVerificationVisitor(verification_context, pos)
{}

void BasicAttributesVerificationVisitor::visit_SyntaxExpression(ebnf::SyntaxExpression* expr) {
	throw ns::err_illegal_state();
}

void BasicAttributesVerificationVisitor::visit_EmptySyntaxExpression(ebnf::EmptySyntaxExpression* expr) {
	set_conversion(expr, new conv::EmptyConversion(expr));
}

void BasicAttributesVerificationVisitor::visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) {
	for (MPtr<ebnf::SyntaxExpression> sub_expr : expr->get_sub_expressions()) {
		verify_sub_expr_attributes(sub_expr.get());
	}
	set_conversion(expr, new conv::OrConversion(get_complex_conversion_type(), 0, expr));
}

void BasicAttributesVerificationVisitor::visit_NameSyntaxExpression(ebnf::NameSyntaxExpression* expr) {
	set_conversion(expr, new conv::NameConversion(expr, get_simple_conversion_type()));
}

void BasicAttributesVerificationVisitor::visit_StringSyntaxExpression(ebnf::StringSyntaxExpression* expr) {
	set_conversion(expr, new conv::StringConversion(expr, get_simple_conversion_type()));
}

void BasicAttributesVerificationVisitor::visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) {
	ebnf::SyntaxExpression* sub_expr = expr->get_sub_expression();
	verify_sub_expr_attributes(sub_expr);
	set_conversion(expr, new conv::ZeroOneConversion(get_complex_conversion_type(), 0, expr));
}

void BasicAttributesVerificationVisitor::visit_LoopSyntaxExpression(ebnf::LoopSyntaxExpression* expr) {
	const ebnf::LoopBody* body = expr->get_body();
	ebnf::SyntaxExpression* sub_expr = body->get_expression();
	verify_sub_expr_attributes(sub_expr);
	ebnf::SyntaxExpression* separator = body->get_separator();
	if (separator) {
		//Separator is always 'dead'.
		m_verification_context->verify_attributes_dead(separator);
	}
}

void BasicAttributesVerificationVisitor::visit_ZeroManySyntaxExpression(ebnf::ZeroManySyntaxExpression* expr) {
	visit_LoopSyntaxExpression(expr);
	set_conversion(expr, new conv::ZeroManyConversion(expr, get_simple_conversion_type()));
}

void BasicAttributesVerificationVisitor::visit_OneManySyntaxExpression(ebnf::OneManySyntaxExpression* expr) {
	visit_LoopSyntaxExpression(expr);
	set_conversion(expr, new conv::OneManyConversion(expr, get_simple_conversion_type()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//verify_attributes_top()
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	//
	//TopAttributesVerificationVisitor
	//

	class TopAttributesVerificationVisitor : public BasicAttributesVerificationVisitor {
		NONCOPYABLE(TopAttributesVerificationVisitor);

	public:
		TopAttributesVerificationVisitor(AttributeVerificationContext* verification_context, const ns::FilePos& pos);

		void visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override;
		void visit_SyntaxElement(ebnf::SyntaxElement* expr) override;
		void visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) override;
		void visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) override;
		void visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) override;
		void visit_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) override;

	private:
		void verify_sub_expr_attributes(ebnf::SyntaxExpression* expr) override;
		conv::SimpleConversionType get_simple_conversion_type() const override;
		ComplexConversionTypeAutoPtr get_complex_conversion_type() const override;

		void process_and_expression_attributes(AttributeScope& scope, ebnf::SyntaxAndExpression* expr);

		std::unique_ptr<ns::AndExpressionMeaning> create_and_expression_meaning(
			AttributeScope& scope,
			ebnf::SyntaxAndExpression* expr);
	};

}

TopAttributesVerificationVisitor::TopAttributesVerificationVisitor(
	AttributeVerificationContext* verification_context,
	const ns::FilePos& pos)
: BasicAttributesVerificationVisitor(verification_context, pos)
{}

void TopAttributesVerificationVisitor::visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) {
	RootScope root_scope;
	AttributeScope scope(&root_scope);

	process_and_expression_attributes(scope, expr);
	std::unique_ptr<ns::AndExpressionMeaning> meaning = create_and_expression_meaning(scope, expr);

	ns::SyntaxAndExpressionExtension* and_ext = expr->get_and_extension();
	and_ext->set_meaning(std::move(meaning));
}

void TopAttributesVerificationVisitor::visit_SyntaxElement(ebnf::SyntaxElement* expr) {
	ebnf::SyntaxExpression* sub_expr = expr->get_expression();
	m_verification_context->verify_attributes_top(sub_expr);
}

void TopAttributesVerificationVisitor::visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) {
	visit_SyntaxElement(expr);
	set_conversion(expr, new conv::TopAttributeConversion(expr));
}

void TopAttributesVerificationVisitor::visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) {
	visit_SyntaxElement(expr);
	set_conversion(expr, new conv::ThisConversion(expr));
}

void TopAttributesVerificationVisitor::visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) {
	ebnf::SyntaxExpression* sub_expr = expr->get_expression();
	m_verification_context->verify_attributes_top(sub_expr);
	set_conversion(expr, new conv::CastConversion(expr));
}

void TopAttributesVerificationVisitor::visit_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) {
	set_conversion(expr, new conv::ConstConversion(expr));//
}

void TopAttributesVerificationVisitor::verify_sub_expr_attributes(ebnf::SyntaxExpression* expr) {
	m_verification_context->verify_attributes_top(expr);
}

conv::SimpleConversionType TopAttributesVerificationVisitor::get_simple_conversion_type() const {
	return conv::SIMPLE_TOP;
}

ComplexConversionTypeAutoPtr TopAttributesVerificationVisitor::get_complex_conversion_type() const {
	return ComplexConversionTypeAutoPtr(new conv::TopComplexConversionType());
}

void TopAttributesVerificationVisitor::process_and_expression_attributes(
	AttributeScope& scope,
	ebnf::SyntaxAndExpression* expr)
{
	MContainer<const StringVector> attr_vector_container;
	init_subtree_attributes(&attr_vector_container, expr);
		
	AndBNFResultAutoPtr and_bnf_result;
	ns::SyntaxExpressionExtension* ext = expr->get_extension();
	if (ext->is_and_result()) {
		if (expr->get_type()) {
			throw ns::raise_error(m_pos, "AND expression has both '%this' and the class type specified");
		}
		and_bnf_result = AndBNFResultAutoPtr(new conv::ThisAndBNFResult(expr));
	} else {
		const AttrExprVector& and_attributes = ext->get_and_attributes();
		if (!and_attributes.empty() || expr->get_type()) {
			and_bnf_result = AndBNFResultAutoPtr(new conv::ClassAndBNFResult(expr));
		} else {
			and_bnf_result = AndBNFResultAutoPtr(new conv::VoidAndBNFResult());
		}
	}
	m_verification_context->process_and_expression(expr, &scope, and_bnf_result.get(), expr);

	clear_subtree_attributes(expr);
}

std::unique_ptr<ns::AndExpressionMeaning> TopAttributesVerificationVisitor::create_and_expression_meaning(
	AttributeScope& scope,
	ebnf::SyntaxAndExpression* expr)
{
	const StringVector& attributes = scope.get_attributes();
	const std::vector<util::MPtr<ebnf::ThisSyntaxElement>>& result_elements = scope.get_result_elements();
	const std::vector<util::MPtr<ebnf::SyntaxExpression>>& non_result_sub_expressions =
		scope.get_non_result_sub_expressions();

	bool has_attributes = !attributes.empty();

	if (!result_elements.empty()) {
		//It must have been already verified that there are neither attributes nor the explicit type.
		assert(!expr->get_type());
		assert(!has_attributes);
		return ns::make_unique1<ns::ThisAndExpressionMeaning>(non_result_sub_expressions, result_elements);
	} else if (expr->get_type() || has_attributes) {
		return ns::make_unique1<ns::ClassAndExpressionMeaning>(non_result_sub_expressions, has_attributes);
	} else {
		return ns::make_unique1<ns::VoidAndExpressionMeaning>(non_result_sub_expressions);
	}
}

//
//AttributeVerificationContext
//

void AttributeVerificationContext::verify_attributes_top(ebnf::SyntaxExpression* expr) {
	TopAttributesVerificationVisitor visitor(this, m_pos);
	expr->visit(&visitor);
}

void AttributeVerificationContext::verify_attributes_top_nt(ebnf::NonterminalDeclaration* nt) {
	m_pos = nt->get_name().pos();
	verify_attributes_top(nt->get_expression());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//verify_attributes_dead()
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	//
	//DeadAttributesVerificationVisitor
	//

	class DeadAttributesVerificationVisitor : public BasicAttributesVerificationVisitor {
		NONCOPYABLE(DeadAttributesVerificationVisitor);

	public:
		DeadAttributesVerificationVisitor(AttributeVerificationContext* verification_context, const ns::FilePos& pos);

		void visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override;
		void visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) override;
		void visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) override;
		void visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) override;
		void visit_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) override;

	private:
		void verify_sub_expr_attributes(ebnf::SyntaxExpression* expr) override;
		conv::SimpleConversionType get_simple_conversion_type() const override;
		ComplexConversionTypeAutoPtr get_complex_conversion_type() const override;
	};

}

DeadAttributesVerificationVisitor::DeadAttributesVerificationVisitor(
	AttributeVerificationContext* verification_context,
	const ns::FilePos& pos)
: BasicAttributesVerificationVisitor(verification_context, pos)
{}

void DeadAttributesVerificationVisitor::visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) {
	if (expr->get_type()) throw ns::raise_error(m_pos, "Dead AND expression cannot have an explicit type");
	for (MPtr<ebnf::SyntaxExpression> sub_expr : expr->get_sub_expressions()) {
		m_verification_context->verify_attributes_dead(sub_expr.get());
	}
	set_conversion(expr, new conv::VoidAndConversion(expr));
}

void DeadAttributesVerificationVisitor::visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) {
	const ns::syntax_string& name = expr->get_name();
	throw ns::raise_error(name, "Attribute '" + name.str() + "' is used in a dead expression");
}

void DeadAttributesVerificationVisitor::visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) {
	throw ns::raise_error(expr->get_pos(), "'%this' is used in a dead expression");
}

void DeadAttributesVerificationVisitor::visit_CastSyntaxExpression(ebnf::CastSyntaxExpression* expr) {
	throw ns::raise_error(m_pos, "Cast is used in a dead expression");
}

void DeadAttributesVerificationVisitor::visit_ConstSyntaxExpression(ebnf::ConstSyntaxExpression* expr) {
	throw ns::raise_error(m_pos, "Constant is used in a dead expression");
}

void DeadAttributesVerificationVisitor::verify_sub_expr_attributes(ebnf::SyntaxExpression* expr) {
	m_verification_context->verify_attributes_dead(expr);
}

conv::SimpleConversionType DeadAttributesVerificationVisitor::get_simple_conversion_type() const {
	return conv::SIMPLE_DEAD;
}

ComplexConversionTypeAutoPtr DeadAttributesVerificationVisitor::get_complex_conversion_type() const {
	return ComplexConversionTypeAutoPtr(new conv::DeadComplexConversionType());
}

//
//AttributeVerificationContext
//

void AttributeVerificationContext::verify_attributes_dead(ebnf::SyntaxExpression* expr) {
	DeadAttributesVerificationVisitor visitor(this, m_pos);
	expr->visit(&visitor);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//verify_attributes_and()
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	//
	//AndAttributesVerificationVisitor
	//

	class AndAttributesVerificationVisitor : public AttributesVerificationVisitor {
		NONCOPYABLE(AndAttributesVerificationVisitor);

		const ebnf::SyntaxAndExpression* const m_main_and_expr;
		AttributeScope* const m_scope;
		conv::AndBNFResult* const m_and_bnf_result;

	public:
		AndAttributesVerificationVisitor(
			AttributeVerificationContext* verification_context,
			const ns::FilePos& pos,
			const ebnf::SyntaxAndExpression* main_and_expr,
			AttributeScope* scope,
			conv::AndBNFResult* and_bnf_result);

		void visit_SyntaxExpression(ebnf::SyntaxExpression* expr) override;
		void visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) override;
		void visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override;
		void visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) override;
		void visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) override;
		void visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) override;
	};

}

AndAttributesVerificationVisitor::AndAttributesVerificationVisitor(
	AttributeVerificationContext* verification_context,
	const ns::FilePos& pos,
	const ebnf::SyntaxAndExpression* main_and_expr,
	AttributeScope* scope,
	conv::AndBNFResult* and_bnf_result)
	: AttributesVerificationVisitor(verification_context, pos),
	m_main_and_expr(main_and_expr),
	m_scope(scope),
	m_and_bnf_result(and_bnf_result)
{
	assert(m_main_and_expr);
}

void AndAttributesVerificationVisitor::visit_SyntaxExpression(ebnf::SyntaxExpression* expr) {
	throw ns::err_illegal_state();
}

void AndAttributesVerificationVisitor::visit_SyntaxOrExpression(ebnf::SyntaxOrExpression* expr) {
	AttributeScope compound_scope(static_cast<AttributeScope*>(nullptr));

	const std::vector<MPtr<ebnf::SyntaxExpression>>& sub_expressions = expr->get_sub_expressions();
	for (MPtr<ebnf::SyntaxExpression> sub_expr_ptr : sub_expressions) {
		ebnf::SyntaxExpression* sub_expr = sub_expr_ptr.get();
		const ns::SyntaxExpressionExtension* sub_ext = sub_expr->get_extension();

		if (sub_ext->is_and_result() || !sub_ext->get_and_attributes().empty()) {
			AttributeScope sub_scope(m_scope);
			m_verification_context->verify_attributes_and(m_main_and_expr, sub_expr, &sub_scope, m_and_bnf_result);

			//Add the scope of the current expression into the compound scope.
			compound_scope.add_scope(&sub_scope);
		} else {
			m_verification_context->verify_attributes_dead(sub_expr);
		}
	}
			
	m_scope->add_scope(&compound_scope);

	ComplexConversionTypeAutoPtr conversion_type = m_and_bnf_result->get_complex_conversion_type();
	set_conversion(expr, new conv::OrConversion(std::move(conversion_type), m_main_and_expr, expr));
}

void AndAttributesVerificationVisitor::visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) {
	if (expr->get_type()) throw ns::raise_error(m_pos, "Nested AND expression cannot have an explicit type");

	//Process sub-expressions.
	AttributeScope sub_scope(m_scope);
	m_verification_context->process_and_expression(m_main_and_expr, &sub_scope, m_and_bnf_result, expr);
	m_scope->add_scope(&sub_scope);
}

void AndAttributesVerificationVisitor::visit_NameSyntaxElement(ebnf::NameSyntaxElement* expr) {
	ebnf::SyntaxExpression* sub_expr = expr->get_expression();
	m_verification_context->verify_attributes_top(sub_expr);
	
	const ns::syntax_string& name = expr->get_name();
	m_scope->add_attribute(name);
	m_scope->add_non_result_sub_expression(expr);
	
	unique_ptr<const conv::Conversion> conversion(m_and_bnf_result->create_attribute_conversion(expr));
	expr->get_extension()->set_conversion(std::move(conversion));
}

void AndAttributesVerificationVisitor::visit_ThisSyntaxElement(ebnf::ThisSyntaxElement* expr) {
	ebnf::SyntaxExpression* sub_expr = expr->get_expression();
	m_verification_context->verify_attributes_top(sub_expr);
	
	m_scope->set_result_element(expr);
	set_conversion(expr, new conv::ThisConversion(expr));
}

void AndAttributesVerificationVisitor::visit_ZeroOneSyntaxExpression(ebnf::ZeroOneSyntaxExpression* expr) {
	m_verification_context->verify_attributes_and(m_main_and_expr, expr->get_sub_expression(), m_scope, m_and_bnf_result);

	ComplexConversionTypeAutoPtr conversion_type = m_and_bnf_result->get_complex_conversion_type();
	set_conversion(expr, new conv::ZeroOneConversion(std::move(conversion_type), m_main_and_expr, expr));
}

//
//AttributeVerificationContext
//

void AttributeVerificationContext::verify_attributes_and(
	const ebnf::SyntaxAndExpression* main_and_expr,
	ebnf::SyntaxExpression* expr,
	AttributeScope* scope,
	conv::AndBNFResult* and_bnf_result)
{
	AndAttributesVerificationVisitor visitor(this, m_pos, main_and_expr, scope, and_bnf_result);
	expr->visit(&visitor);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//EBNF_Builder
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ns::EBNF_Builder::verify_attributes() {
	assert(m_resolve_name_references_completed);
	assert(!m_verify_attributes_completed);

	MPtr<MContainer<ns::PartClassTypeDescriptor>> managed_part_classes =
		m_const_heap_ref->create_container<ns::PartClassTypeDescriptor>();

	AttributeVerificationContext verification_context(m_part_class_tags.get(), managed_part_classes);
	for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) {
		verification_context.verify_attributes_top_nt(nt);
	}

	m_verify_attributes_completed = true;
}
