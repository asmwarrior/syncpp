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

//Definitions used internally by SysClass functionality.

#ifndef SYNSAMPLE_CORE_SYSCLASS_INT_H_INCLUDED
#define SYNSAMPLE_CORE_SYSCLASS_INT_H_INCLUDED

#include "common.h"
#include "gc.h"
#include "name__dec.h"
#include "sysclass.h"
#include "value.h"

namespace syn_script {
	namespace rt {
		class SysConstructor;
		class SysMember;
	}
}

//
//SysConstructor
//

class syn_script::rt::SysConstructor : public gc::Object {
	NONCOPYABLE(SysConstructor);

protected:
	SysConstructor(){}

public:
	virtual gc::Local<Value> instantiate(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments) const = 0;
};

//
//SysMember
//

class syn_script::rt::SysMember : public gc::Object {
	NONCOPYABLE(SysMember);

	gc::Ref<const NameInfo> m_name_info;

protected:
	SysMember(){}
	void gc_enumerate_refs() override { gc_ref(m_name_info); }
	void initialize(const gc::Local<const NameInfo>& name_info){ m_name_info = name_info; }

public:
	const gc::Ref<const NameInfo>& get_name_info() const { return m_name_info; }

	virtual gc::Local<Value> get(const gc::Local<ExecContext>& context, const gc::Local<Value>& object) const = 0;
	virtual gc::Local<Value> get_static(const gc::Local<ExecContext>& context) const = 0;
};

//
//SysClass::InternalClass
//

class syn_script::rt::SysClass::InternalClass : public gc::Object{
	NONCOPYABLE(InternalClass);

	gc::Ref<SysConstructor> m_constructor;
	gc::Ref<gc::Array<SysMember>> m_members;

public:
	InternalClass();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<SysConstructor>& constructor, const gc::Local<gc::Array<SysMember>>& members);

	gc::Local<Value> instantiate(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments);

	gc::Local<Value> get_member(
		const gc::Local<ExecContext>& context,
		const gc::Local<const NameInfo>& name_info,
		const gc::Local<Value>& object) const;

	gc::Local<Value> get_member_static(
		const gc::Local<ExecContext>& context,
		const gc::Local<const NameInfo>& name_info) const;

private:
	gc::Local<SysMember> find_member(const gc::Local<const NameInfo>& name_info) const;
};

#endif//SYNSAMPLE_CORE_SYSCLASS_INT_H_INCLUDED
