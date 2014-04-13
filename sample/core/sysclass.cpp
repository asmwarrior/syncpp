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

//SysClass implementation.

#include <list>
#include <memory>

#include "ast.h"
#include "common.h"
#include "gc.h"
#include "name.h"
#include "stringex.h"
#include "sysclass.h"
#include "sysclass__int.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;
namespace ast = ss::ast;

//
//SysClass
//

rt::SysClass::SysClass(){}

void rt::SysClass::gc_enumerate_refs() {
	gc_ref(m_internal);
	gc_ref(m_class_value);
}

void rt::SysClass::initialize(const gc::Local<InternalClass>& internal) {
	m_internal = internal;
	m_class_value = gc::create<SysClassValue>(self(this));
}

gc::Local<rt::Value> rt::SysClass::get_class_value() const {
	return m_class_value;
}

gc::Local<rt::Value> rt::SysClass::instantiate(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments)
{
	return m_internal->instantiate(context, arguments);
}

gc::Local<rt::Value> rt::SysClass::get_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<const ss::NameInfo>& name_info,
	const gc::Local<Value>& object) const
{
	return m_internal->get_member(context, name_info, object);
}

gc::Local<rt::Value> rt::SysClass::get_member_static(
	const gc::Local<ExecContext>& context,
	const gc::Local<const ss::NameInfo>& name_info) const
{
	return m_internal->get_member_static(context, name_info);
}

//
//SysClass::InternalClass : implementation
//

rt::SysClass::InternalClass::InternalClass(){}

void rt::SysClass::InternalClass::gc_enumerate_refs() {
	gc_ref(m_constructor);
	gc_ref(m_members);
}

void rt::SysClass::InternalClass::initialize(
	const gc::Local<SysConstructor>& constructor,
	const gc::Local<gc::Array<SysMember>>& members)
{
	m_constructor = constructor;
	m_members = members;
}

gc::Local<rt::Value> rt::SysClass::InternalClass::instantiate(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments)
{
	if (!m_constructor) throw RuntimeError("Constructor is not defined");
	return m_constructor->instantiate(context, arguments);
}

gc::Local<rt::Value> rt::SysClass::InternalClass::get_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<const ss::NameInfo>& name_info,
	const gc::Local<Value>& object) const
{
	gc::Local<SysMember> member = find_member(name_info);
	return member->get(context, object);
}

gc::Local<rt::Value> rt::SysClass::InternalClass::get_member_static(
	const gc::Local<ExecContext>& context,
	const gc::Local<const ss::NameInfo>& name_info) const
{
	gc::Local<SysMember> member = find_member(name_info);
	return member->get_static(context);
}

gc::Local<rt::SysMember> rt::SysClass::InternalClass::find_member(
	const gc::Local<const ss::NameInfo>& name_info) const
{
	const NameID name_id = name_info->get_id();
	for (std::size_t i = 0, n = m_members->length(); i < n; ++i) {
		const gc::Ref<SysMember>& member = m_members->get(i);
		if (name_id == member->get_name_info()->get_id()) return member;
	}

	throw RuntimeError(std::string("Member not found: ") + name_info->get_str()->get_std_string());
}
