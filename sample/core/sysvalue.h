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

//Definition of SysObjectValue class.

#ifndef SYNSAMPLE_CORE_SYSVALUE_H_INCLUDED
#define SYNSAMPLE_CORE_SYSVALUE_H_INCLUDED

#include "gc.h"
#include "scope__dec.h"
#include "sysclass__dec.h"
#include "sysclassbld.h"
#include "value.h"

namespace syn_script {
	namespace rt {
		class SysObjectValue;
		class SysClassValue;
		class SysNamespaceValue;
	}
}

//
//SysObjectValue
//

//Base class for all API object values.
class syn_script::rt::SysObjectValue : public syn_script::rt::ReferenceValue {
	NONCOPYABLE(SysObjectValue);

protected:
	SysObjectValue();

public:
	gc::Local<Value> get_member(
		const gc::Local<ExecContext>& context,
		const gc::Local<ExecScope>& scope,
		const gc::Local<const NameInfo>& name_info) override final;

	static void check_arguments(
		const gc::Local<gc::Array<rt::Value>>& arguments,
		std::size_t min_count,
		std::size_t max_count);

	static void check_arguments(
		const gc::Local<gc::Array<rt::Value>>& arguments,
		std::size_t min_count);

protected:
	virtual std::size_t get_sys_class_id() const = 0;

private:
	gc::Local<SysClass> get_sys_class(const gc::Local<ExecContext>& context) const;
};

//
//SysClassValue
//

//API class value.
class syn_script::rt::SysClassValue : public ReferenceValue {
	NONCOPYABLE(SysClassValue);

	gc::Ref<SysClass> m_sys_class;

public:
	SysClassValue();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<SysClass>& sys_class);

	gc::Local<Value> get_member(
		const gc::Local<ExecContext>& context,
		const gc::Local<ExecScope>& scope,
		const gc::Local<const NameInfo>& name_info) override;

	gc::Local<Value> instantiate(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments,
		gc::Local<Value>& exception) override;
};

//
//SysNamespaceValue
//

//API namespace value.
class syn_script::rt::SysNamespaceValue : public ReferenceValue {
	NONCOPYABLE(SysNamespaceValue);

	friend class SysAPI<SysNamespaceValue>;
	class API;

	gc::Ref<SysClass> m_sys_class;

public:
	SysNamespaceValue();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<SysClass>& sys_class);

	gc::Local<Value> get_member(
		const gc::Local<ExecContext>& context,
		const gc::Local<ExecScope>& scope,
		const gc::Local<const NameInfo>& name_info) override;
};

#endif//SYNSAMPLE_CORE_SYSVALUE_H_INCLUDED
