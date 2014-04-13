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

//SysClass definition.

#ifndef SYNSAMPLE_CORE_SYSCLASS_H_INCLUDED
#define SYNSAMPLE_CORE_SYSCLASS_H_INCLUDED

#include "common.h"
#include "gc.h"
#include "name__dec.h"
#include "sysclass__dec.h"
#include "value.h"

//
//SysClass
//

//Describes an API class: constructors, fields, methods.
class syn_script::rt::SysClass : public ObjectEx {
	NONCOPYABLE(SysClass);

public:
	class InternalClass;

private:
	gc::Ref<InternalClass> m_internal;
	gc::Ref<Value> m_class_value;

public:
	SysClass();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<InternalClass>& internal);

	gc::Local<Value> get_class_value() const;

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
};

#endif//SYNSAMPLE_CORE_SYSCLASS_H_INCLUDED
