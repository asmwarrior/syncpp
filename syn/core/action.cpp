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

//Implementation of classes representing semantic actions (actions associated with BNF productions).

#include <cassert>
#include <memory>
#include <string>

#include "action.h"
#include "codegen.h"
#include "ebnf__def.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace util = ns::util;

using std::unique_ptr;
using util::MPtr;

//
//Action
//

MPtr<const ns::ClassTypeDescriptor> ns::Action::get_part_class_type_class() const {
	return nullptr;
}

//
//VoidAction
//

ns::VoidAction::VoidAction(MPtr<const VoidTypeDescriptor> void_type)
	: AbstractAction<VoidTypeDescriptor>(void_type)
{}

void ns::VoidAction::visit(ActionVisitor* visitor) const {
	visitor->visit_VoidAction(this);
}

void ns::VoidAction::print(std::ostream& out) const {
	out << "void";
}

//
//CopyAction
//

ns::CopyAction::CopyAction(MPtr<const TypeDescriptor> type)
	: AbstractAction<TypeDescriptor>(type)
{}

void ns::CopyAction::visit(ActionVisitor* visitor) const {
	visitor->visit_CopyAction(this);
}

void ns::CopyAction::print(std::ostream& out) const {
	out << "$0";
}

//
//CastAction
//

ns::CastAction::CastAction(
	MPtr<const ClassTypeDescriptor> cast_type,
	MPtr<const ClassTypeDescriptor> actual_type)
	: AbstractAction<ClassTypeDescriptor>(cast_type),
	m_actual_type(actual_type)
{
	assert(!!m_actual_type);
}

MPtr<const ns::ClassTypeDescriptor> ns::CastAction::get_actual_type() const {
	return m_actual_type;
}

void ns::CastAction::visit(ActionVisitor* visitor) const {
	visitor->visit_CastAction(this);
}

void ns::CastAction::print(std::ostream& out) const {
	out << "cast( $0 , ";
	m_result_type->print(out);
	out << " )";
}

//
//AbstractClassAction
//

ns::AbstractClassAction::AbstractClassAction(
	MPtr<const ClassTypeDescriptor> class_type,
	unique_ptr<const ActionDefs::AttributeVector> attributes,
	unique_ptr<const ActionDefs::PartClassVector> part_classes,
	unique_ptr<const ActionDefs::ClassVector> classes)
	: m_class_type(class_type),
	m_attributes(std::move(attributes)),
	m_part_classes(std::move(part_classes)),
	m_classes(std::move(classes))
{
	assert(!!m_class_type);
}

MPtr<const ns::ClassTypeDescriptor> ns::AbstractClassAction::get_class_type() const {
	return m_class_type;
}

namespace {
	const ns::ActionDefs::AttributeVector g_empty_attributes;
	const ns::ActionDefs::PartClassVector g_empty_part_classes;
	const ns::ActionDefs::ClassVector g_empty_classes;
}//namespace

const ns::ActionDefs::AttributeVector& ns::AbstractClassAction::get_attributes() const {
	return m_attributes.get() ? *m_attributes : g_empty_attributes;
}

const ns::ActionDefs::PartClassVector& ns::AbstractClassAction::get_part_classes() const {
	return m_part_classes.get() ? *m_part_classes : g_empty_part_classes;
}

const ns::ActionDefs::ClassVector& ns::AbstractClassAction::get_classes() const {
	return m_classes.get() ? *m_classes : g_empty_classes;
}

void ns::AbstractClassAction::print(std::ostream& out) const {
	print_header(out);
	out << "(";
	bool empty = true;

	if (m_attributes.get()) {
		for (const ActionDefs::AttributeField& attr : *m_attributes) {
			out << " " << attr.m_name << "($" << attr.m_offset << ")";
			empty = false;
		}
	}

	if (m_part_classes.get()) {
		for (const ActionDefs::PartClassField& part : *m_part_classes) {
			out << " part:($" << part.m_offset << ")";
			empty = false;
		}
	}

	if (m_classes.get()) {
		for (const ActionDefs::ClassField& cls : *m_classes) {
			out << " this($" << cls.m_offset << ")";
			empty = false;
		}
	}

	out << (empty ? "" : " ") << ")";
}

//
//ClassAction
//

ns::ClassAction::ClassAction(
	MPtr<const ClassTypeDescriptor> class_type,
	unique_ptr<const ActionDefs::AttributeVector> attributes,
	unique_ptr<const ActionDefs::PartClassVector> part_classes,
	unique_ptr<const ActionDefs::ClassVector> classes)
	: AbstractClassAction(class_type, std::move(attributes), std::move(part_classes), std::move(classes))
{}

MPtr<const ns::TypeDescriptor> ns::ClassAction::get_result_type() const {
	return get_class_type();
}

void ns::ClassAction::visit(ActionVisitor* visitor) const {
	visitor->visit_ClassAction(this);
}

void ns::ClassAction::print_header(std::ostream& out) const {
	out << "new ";
	get_class_type()->print(out);
}

//
//PartClassAction
//

ns::PartClassAction::PartClassAction(
	MPtr<const PartClassTypeDescriptor> part_class_type,
	unique_ptr<const ActionDefs::AttributeVector> attributes,
	unique_ptr<const ActionDefs::PartClassVector> part_classes,
	unique_ptr<const ActionDefs::ClassVector> classes)
	: AbstractClassAction(part_class_type->get_class_type(), std::move(attributes), std::move(part_classes), std::move(classes)),
	m_part_class_type(part_class_type)
{
	assert(!!m_part_class_type);
}

MPtr<const ns::TypeDescriptor> ns::PartClassAction::get_result_type() const {
	return m_part_class_type;
}

MPtr<const ns::ClassTypeDescriptor> ns::PartClassAction::get_part_class_type_class() const {
	return get_class_type();
}

void ns::PartClassAction::visit(ActionVisitor* visitor) const {
	visitor->visit_PartClassAction(this);
}

void ns::PartClassAction::print_header(std::ostream& out) const {
	out << "part:";
}

//
//ResultAndAction
//

ns::ResultAndAction::ResultAndAction(MPtr<const TypeDescriptor> type, std::size_t result_index)
	: AbstractAction<TypeDescriptor>(type),
	m_result_index(result_index)
{}

std::size_t ns::ResultAndAction::get_result_index() const {
	return m_result_index;
}

void ns::ResultAndAction::visit(ActionVisitor* visitor) const {
	visitor->visit_ResultAndAction(this);
}

void ns::ResultAndAction::print(std::ostream& out) const {
	out << "$" << m_result_index;
}

//
//ListAction
//

ns::ListAction::ListAction(MPtr<const ListTypeDescriptor> list_type)
	: AbstractAction<ListTypeDescriptor>(list_type)
{}

//
//FirstListAction
//

ns::FirstListAction::FirstListAction(MPtr<const ListTypeDescriptor> list_type)
	: ListAction(list_type)
{}

void ns::FirstListAction::visit(ActionVisitor* visitor) const {
	visitor->visit_FirstListAction(this);
}

void ns::FirstListAction::print(std::ostream& out) const {
	out << "list().add($0)";
}

//
//NextListAction
//

ns::NextListAction::NextListAction(MPtr<const ListTypeDescriptor> list_type, bool separator)
	: ListAction(list_type),
	m_separator(separator)
{}

bool ns::NextListAction::is_separator() const {
	return m_separator;
}

void ns::NextListAction::visit(ActionVisitor* visitor) const {
	visitor->visit_NextListAction(this);
}

void ns::NextListAction::print(std::ostream& out) const {
	out << "list($0).add($" << (m_separator ? 2 : 1) << ")";
}

//
//ConstAction
//

ns::ConstAction::ConstAction(MPtr<const PrimitiveTypeDescriptor> type, const ebnf::ConstExpression* const_expr)
	: AbstractAction<PrimitiveTypeDescriptor>(type),
	m_const_expr(const_expr)
{}

const ebnf::ConstExpression* ns::ConstAction::get_const_expr() const {
	return m_const_expr;
}

void ns::ConstAction::visit(ActionVisitor* visitor) const {
	visitor->visit_ConstAction(this);
}

void ns::ConstAction::print(std::ostream& out) const {
	out << "<";
	m_const_expr->print(out);
	out << ">";
}
