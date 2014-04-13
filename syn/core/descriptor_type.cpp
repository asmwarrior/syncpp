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

//Implementation of type descriptor classes.

#include "descriptor_type.h"
#include "types.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace types = ns::types;
namespace util = ns::util;

using util::MPtr;
using util::String;

//
//TypeDescriptor
//

bool ns::TypeDescriptor::is_void() const {
	return false;
}

const ns::PrimitiveTypeDescriptor* ns::TypeDescriptor::as_primitive_type() const {
	return nullptr;
}

const ns::ClassTypeDescriptor* ns::TypeDescriptor::as_class_type() const {
	return nullptr;
}

bool ns::TypeDescriptor::accepts(const TypeDescriptor* type) const {
	return equals(type);
}

bool ns::TypeDescriptor::equals_class(const ClassTypeDescriptor* type) const {
	return false;
}

bool ns::TypeDescriptor::equals_part_class(const PartClassTypeDescriptor* type) const {
	return false;
}

bool ns::TypeDescriptor::equals_list(const ListTypeDescriptor* type) const {
	return false;
}

bool ns::TypeDescriptor::equals_primitive(const PrimitiveTypeDescriptor* type) const {
	return false;
}

//
//VoidTypeDescriptor
//

ns::VoidTypeDescriptor::VoidTypeDescriptor()
{}

bool ns::VoidTypeDescriptor::is_void() const {
	return true;
}

bool ns::VoidTypeDescriptor::equals(const TypeDescriptor* type) const {
	return type->is_void();
}

void ns::VoidTypeDescriptor::visit(TypeDescriptorVisitor* visitor) const {
	visitor->visit_VoidTypeDescriptor(this);
}

void ns::VoidTypeDescriptor::print(std::ostream& out) const {
	out << "void";
}

//
//AbstractClassTypeDescriptor
//

ns::AbstractClassTypeDescriptor::AbstractClassTypeDescriptor()
{}

//
//ClassTypeDescriptor
//

ns::ClassTypeDescriptor::ClassTypeDescriptor(std::size_t index, const String& class_name)
	: m_index(index),
	m_class_name(class_name)
{
	assert(!m_class_name.empty());
}

std::size_t ns::ClassTypeDescriptor::get_index() const {
	return m_index;
}

const String& ns::ClassTypeDescriptor::get_class_name() const {
	return m_class_name;
}

const ns::ClassTypeDescriptor* ns::ClassTypeDescriptor::as_class_type() const {
	return this;
}

bool ns::ClassTypeDescriptor::equals(const TypeDescriptor* type) const {
	return this == type ? true : type->equals_class(this);
}

bool ns::ClassTypeDescriptor::accepts(const TypeDescriptor* type) const {
	//Any class type is accepted by any other.
	return !!type->as_class_type();
}

void ns::ClassTypeDescriptor::visit(TypeDescriptorVisitor* visitor) const {
	visitor->visit_ClassTypeDescriptor(this);
}

void ns::ClassTypeDescriptor::print(std::ostream& out) const {
	out << "class ";
	out << m_class_name;
}

bool ns::ClassTypeDescriptor::equals_class(const ClassTypeDescriptor* type) const {
	return m_class_name == type->m_class_name;
}

//
//PartClassTypeDescriptor
//

ns::PartClassTypeDescriptor::PartClassTypeDescriptor(MPtr<const ClassTypeDescriptor> class_type, std::size_t tag_index)
	: m_class_type(class_type),
	m_tag_index(tag_index)
{}

const MPtr<const ns::ClassTypeDescriptor> ns::PartClassTypeDescriptor::get_class_type() const {
	return m_class_type;
}

bool ns::PartClassTypeDescriptor::equals(const TypeDescriptor* type) const {
	return type->equals_part_class(this);
}

void ns::PartClassTypeDescriptor::visit(TypeDescriptorVisitor* visitor) const {
	visitor->visit_PartClassTypeDescriptor(this);
}

void ns::PartClassTypeDescriptor::print(std::ostream& out) const {
	out << "part ???";
}

bool ns::PartClassTypeDescriptor::equals_part_class(const PartClassTypeDescriptor* type) const {
	return m_tag_index == type->m_tag_index && m_class_type->equals(type->m_class_type.get());
}

//
//ListTypeDescriptor
//

ns::ListTypeDescriptor::ListTypeDescriptor(MPtr<const TypeDescriptor> element_type)
	: m_element_type(element_type)
{
	assert(!!m_element_type);
}

MPtr<const ns::TypeDescriptor> ns::ListTypeDescriptor::get_element_type() const {
	return m_element_type;
}

bool ns::ListTypeDescriptor::equals(const TypeDescriptor* type) const {
	return this == type ? true : type->equals_list(this);
}

void ns::ListTypeDescriptor::visit(TypeDescriptorVisitor* visitor) const {
	visitor->visit_ListTypeDescriptor(this);
}

void ns::ListTypeDescriptor::print(std::ostream& out) const {
	out << "list [ ";
	m_element_type->print(out);
	out << " ]";
}

bool ns::ListTypeDescriptor::equals_list(const ListTypeDescriptor* type) const {
	return m_element_type->equals(type->m_element_type.get());
}

//
//PrimitiveTypeDescriptor
//

ns::PrimitiveTypeDescriptor::PrimitiveTypeDescriptor(const types::PrimitiveType* primitive_type)
	: m_primitive_type(primitive_type)
{
	assert(m_primitive_type);
}

const ns::PrimitiveTypeDescriptor* ns::PrimitiveTypeDescriptor::as_primitive_type() const {
	return this;
}

const types::PrimitiveType* ns::PrimitiveTypeDescriptor::get_primitive_type() const {
	return m_primitive_type;
}

bool ns::PrimitiveTypeDescriptor::equals(const TypeDescriptor* type) const {
	return this == type ? true : type->equals_primitive(this);
}

void ns::PrimitiveTypeDescriptor::visit(TypeDescriptorVisitor* visitor) const {
	visitor->visit_PrimitiveTypeDescriptor(this);
}

void ns::PrimitiveTypeDescriptor::print(std::ostream& out) const {
	out << "type ";
	m_primitive_type->print(out);
}

bool ns::PrimitiveTypeDescriptor::equals_primitive(const PrimitiveTypeDescriptor* type) const {
	return m_primitive_type->equals(type->m_primitive_type);
}
