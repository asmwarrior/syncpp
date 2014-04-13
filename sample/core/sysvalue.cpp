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

//System object value class implementation.

#include <sstream>
#include <string>

#include "common.h"
#include "gc.h"
#include "name.h"
#include "sysclass.h"
#include "sysvalue.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace rt = ss::rt;

//
//SysObjectValue
//

rt::SysObjectValue::SysObjectValue(){}

gc::Local<rt::Value> rt::SysObjectValue::get_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<ExecScope>& scope,
	const gc::Local<const ss::NameInfo>& name_info)
{
	gc::Local<SysClass> cls = get_sys_class(context);
	return cls->get_member(context, name_info, self(this));
}

void rt::SysObjectValue::check_arguments(
	const gc::Local<gc::Array<rt::Value>>& arguments,
	std::size_t min_count,
	std::size_t max_count)
{
	std::size_t cnt = arguments->length();
	if (cnt >= min_count && cnt <= max_count) return;

	std::ostringstream sout;
	sout << "Wrong number of arguments: expected " << min_count;
	if (max_count != min_count) sout << "..." << max_count;
	sout << ", was " << cnt;

	throw RuntimeError(sout.str());
}

void rt::SysObjectValue::check_arguments(
	const gc::Local<gc::Array<rt::Value>>& arguments,
	std::size_t min_count)
{
	check_arguments(arguments, min_count, min_count);
}

gc::Local<rt::SysClass> rt::SysObjectValue::get_sys_class(const gc::Local<ExecContext>& context) const {
	std::size_t class_id = get_sys_class_id();
	gc::Local<SysClass> cls = context->get_value_factory()->get_sys_class(class_id);
	assert(!!cls);
	return cls;
}

//
//SysClassValue
//

rt::SysClassValue::SysClassValue(){}

void rt::SysClassValue::gc_enumerate_refs() {
	gc_ref(m_sys_class);
}

void rt::SysClassValue::initialize(const gc::Local<SysClass>& sys_class) {
	assert(!!sys_class);
	m_sys_class = sys_class;
}

gc::Local<rt::Value> rt::SysClassValue::get_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<ExecScope>& scope,
	const gc::Local<const ss::NameInfo>& name_info)
{
	return m_sys_class->get_member_static(context, name_info);
}

gc::Local<rt::Value> rt::SysClassValue::instantiate(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	gc::Local<Value>& exception)
{
	return m_sys_class->instantiate(context, arguments);
}

//
//SysNamespaceValue
//

rt::SysNamespaceValue::SysNamespaceValue(){}

void rt::SysNamespaceValue::gc_enumerate_refs() {
	gc_ref(m_sys_class);
}

void rt::SysNamespaceValue::initialize(const gc::Local<SysClass>& sys_class) {
	m_sys_class = sys_class;
}

gc::Local<rt::Value> rt::SysNamespaceValue::get_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<ExecScope>& scope,
	const gc::Local<const ss::NameInfo>& name_info)
{
	return m_sys_class->get_member_static(context, name_info);
}
