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

//Implementation of descriptor classes.

#include <cassert>

#include "action.h"
#include "descriptor.h"
#include "descriptor_type.h"
#include "util_string.h"

namespace ns = synbin;
namespace util = ns::util;

using util::MPtr;
using util::String;

//
//SymDescriptor
//

ns::SymDescriptor::SymDescriptor(MPtr<const TypeDescriptor> type)
	: m_type(type)
{
	assert(!!m_type);
}

MPtr<const ns::TypeDescriptor> ns::SymDescriptor::get_type() const {
	return m_type;
}

//
//TrDescriptor
//

ns::TrDescriptor::TrDescriptor(MPtr<const TypeDescriptor> type)
	: SymDescriptor(type)
{}

//
//NameTrDescriptor
//

ns::NameTrDescriptor::NameTrDescriptor(MPtr<const TypeDescriptor> type, const String& name)
	: TrDescriptor(type),
	m_name(name)
{}

const String& ns::NameTrDescriptor::get_name() const {
	return m_name;
}

void ns::NameTrDescriptor::generate_constant_name(std::ostream& out) const {
	out << "T_" << m_name;
}

void ns::NameTrDescriptor::generate_constant_comment(std::ostream& out) const {
	//No comments.
}

void ns::NameTrDescriptor::generate_token_str(std::ostream& out) const {
	//No str.
}

void ns::NameTrDescriptor::print(std::ostream& out) const {
	out << m_name;
}

//
//StrTrDescriptor
//

ns::StrTrDescriptor::StrTrDescriptor(MPtr<const TypeDescriptor> type, const String& str, std::size_t id, bool is_name)
	: TrDescriptor(type),
	m_str(str),
	m_id(id),
	m_is_name(is_name)
{}

const String& ns::StrTrDescriptor::get_str() const {
	return m_str;
}

bool ns::StrTrDescriptor::is_name() const {
	return m_is_name;
}

void ns::StrTrDescriptor::generate_constant_name(std::ostream& out) const {
	out << "C" << m_id;
}

void ns::StrTrDescriptor::generate_constant_comment(std::ostream& out) const {
	out << " //\"" << m_str << "\"";
}

void ns::StrTrDescriptor::generate_token_str(std::ostream& out) const {
	out << m_str;
}

void ns::StrTrDescriptor::print(std::ostream& out) const {
	out << '"' << m_str << '"';
}

//
//NtDescriptor
//

ns::NtDescriptor::NtDescriptor(MPtr<const TypeDescriptor> type, const String& bnf_name)
	: SymDescriptor(type),
	m_bnf_name(bnf_name)
{}

const String& ns::NtDescriptor::get_bnf_name() const {
	return m_bnf_name;
}

//
//UserNtDescriptor
//

ns::UserNtDescriptor::UserNtDescriptor(MPtr<const TypeDescriptor> type, const String& bnf_name, const String& name)
	: NtDescriptor(type, bnf_name),
	m_name(name)
{}

const String& ns::UserNtDescriptor::get_name() const {
	return m_name;
}

const ns::UserNtDescriptor* ns::UserNtDescriptor::as_user_nt() const {
	return this;
}

void ns::UserNtDescriptor::print(std::ostream& out) const {
	out << m_name;
}

//
//AutoNtDescriptor
//

ns::AutoNtDescriptor::AutoNtDescriptor(MPtr<const TypeDescriptor> type, const util::String& bnf_name)
	: NtDescriptor(type, bnf_name)
{}

const ns::UserNtDescriptor* ns::AutoNtDescriptor::as_user_nt() const {
	return nullptr;
}

void ns::AutoNtDescriptor::print(std::ostream& out) const {
	out << "auto:" << get_bnf_name();
}

//
//PrDescriptor
//

ns::PrDescriptor::PrDescriptor(MPtr<const Action> action)
	: m_action(action)
{}

MPtr<const ns::TypeDescriptor> ns::PrDescriptor::get_type() const {
	return m_action->get_result_type();
}

MPtr<const ns::Action> ns::PrDescriptor::get_action() const {
	return m_action;
}
