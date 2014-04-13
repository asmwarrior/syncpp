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

//Type classes implementation.

#include <cassert>
#include <iostream>
#include <string>
#include "ebnf__def.h"
#include "types.h"
#include "util_mptr.h"
#include "util_string.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

using util::MPtr;
using util::String;

//
//Type
//

void synbin::types::Type::visit(TypeVisitor<void>* visitor) const {
	visit0(visitor);
}

bool types::Type::equals(const Type* type) const {
	return this == type;
}

bool types::Type::equals_concrete(const ArrayType* type) const {
	return false;
}

//
//PrimitiveType
//

types::PrimitiveType::PrimitiveType(const String& name)
	: m_name(name)
{}

const String& types::PrimitiveType::get_name() const {
	return m_name;
}

//
//UserPrimitiveType
//

types::UserPrimitiveType::UserPrimitiveType(const String& name)
	: PrimitiveType(name)
{}

void types::UserPrimitiveType::visit0(ns::TypeVisitor<void>* visitor) const {
	visitor->visit_UserPrimitiveType(this);
}

void types::UserPrimitiveType::print(std::ostream& out) const {
	out << "user:" << get_name();
}

//
//SystemPrimitiveType
//

types::SystemPrimitiveType::SystemPrimitiveType(const String& name)
	: PrimitiveType(name)
{}

void types::SystemPrimitiveType::visit0(ns::TypeVisitor<void>* visitor) const {
	visitor->visit_SystemPrimitiveType(this);
}

void types::SystemPrimitiveType::print(std::ostream& out) const {
	out << "sys:" << get_name();
}

//
//ClassType
//

void types::ClassType::visit0(ns::TypeVisitor<void>* visitor) const {
	visitor->visit_ClassType(this);
}

//
//NonterminalClassType
//

types::NonterminalClassType::NonterminalClassType(ebnf::NonterminalDeclaration* nt)
	: m_nt(nt)
{}

const String& types::NonterminalClassType::get_class_name() const {
	return m_nt->get_name().get_string();
}

void types::NonterminalClassType::visit0(ns::TypeVisitor<void>* visitor) const {
	visitor->visit_NonterminalClassType(this);
}

void types::NonterminalClassType::print(std::ostream& out) const {
	out << "nt:" << m_nt->get_name();
}

//
//NameClassType
//

types::NameClassType::NameClassType(const String& name)
	: m_name(name)
{}

const String& types::NameClassType::get_class_name() const {
	return m_name;
}

void types::NameClassType::visit0(ns::TypeVisitor<void>* visitor) const {
	visitor->visit_NameClassType(this);
}

void types::NameClassType::print(std::ostream& out) const {
	out << "cl:" << m_name;
}

//
//VoidType
//

void types::VoidType::visit0(ns::TypeVisitor<void>* visitor) const {
	visitor->visit_VoidType(this);
}

void types::VoidType::print(std::ostream& out) const {
	out << "void";
}

//
//ArrayType
//

types::ArrayType::ArrayType(MPtr<const types::Type> element_type)
	: m_element_type(element_type)
{
	assert(element_type.get());
}

void types::ArrayType::visit0(ns::TypeVisitor<void>* visitor) const {
	visitor->visit_ArrayType(this);
}

void types::ArrayType::print(std::ostream& out) const {
	out << "array[";
	m_element_type->print(out);
	out << "]";
}

bool types::ArrayType::equals(const Type* type) const {
	bool result = type->equals_concrete(this);
	return result;
}

bool types::ArrayType::equals_concrete(const ArrayType* type) const {
	bool result = m_element_type->equals(type->m_element_type.get());
	return result;
}
