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

//SysClass builder. Used to construct a SysClass object.

#ifndef SYNSAMPLE_CORE_SYSCLASSBLD_H_INCLUDED
#define SYNSAMPLE_CORE_SYSCLASSBLD_H_INCLUDED

#include <functional>

#include "common.h"
#include "gc.h"
#include "name__dec.h"
#include "sysclass__dec.h"
#include "value.h"

namespace syn_script {
	namespace rt {
		class MethodAdapter;
		class StaticFieldAdapter;
		class StaticMethodAdapter;
		class DynamicFieldAdapter;
		class DynamicMethodAdapter;

		class BasicSysClassBuilder;
		template<class T> class SysClassBuilder;

		class BasicSysClassInitializer;
		class SysClassInitializer;

		template<class T> class SysAPI;
	}
}

//
//BasicSysClassBuilder
//

class syn_script::rt::BasicSysClassBuilder {
	NONCOPYABLE(BasicSysClassBuilder);

	class InternalBuilder;

protected:
	const std::unique_ptr<InternalBuilder> m_internal;

protected:
	BasicSysClassBuilder(NameRegistry& name_registry);

public:
	virtual ~BasicSysClassBuilder();

	gc::Local<SysClass> create_sys_class() const;

protected:
	void add_constructor_0(const gc::Local<StaticMethodAdapter>& adapter);
	void add_static_field_0(const char* name, const gc::Local<StaticFieldAdapter>& adapter);
	void add_static_method_0(const char* name, const gc::Local<StaticMethodAdapter>& adapter);
	void add_field_0(const char* name, const gc::Local<DynamicFieldAdapter>& adapter);
	void add_method_0(const char* name, const gc::Local<DynamicMethodAdapter>& adapter);
};

//
//SysClassBuilder
//

template<class T>
class syn_script::rt::SysClassBuilder : public BasicSysClassBuilder {
	NONCOPYABLE(SysClassBuilder);

public:
	SysClassBuilder(NameRegistry& name_registry) : BasicSysClassBuilder(name_registry){}

	template<class C>
	void add_class(const char* name);

	template<class... Args>
	void add_constructor(gc::Local<T> (*fn)(const gc::Local<ExecContext>&, Args...));

	void add_static_field(const char* name, const gc::Local<Value>& value);

	template<class R>
	void add_static_field(const char* name, R (*fn)(const gc::Local<ExecContext>&));

	template<class R, class... Args>
	void add_static_method(const char* name, R (*fn)(const gc::Local<ExecContext>&, Args...));

	template<class R>
	void add_field(const char* name, R (T::*fn)(const gc::Local<ExecContext>&));

	template<class R, class... Args>
	void add_method(const char* name, R (T::*fn)(const gc::Local<ExecContext>&, Args...));
};

//
//BasicSysClassInitializer
//

class syn_script::rt::BasicSysClassInitializer {
	NONCOPYABLE(BasicSysClassInitializer);

	const std::size_t m_class_id;
	
protected:
	BasicSysClassInitializer();

public:
	static const std::vector<const BasicSysClassInitializer*>& get_class_initializers();

	std::size_t get_class_id() const;
	virtual gc::Local<SysClass> create_sys_class(NameRegistry& name_registry) const = 0;
};

//
//SysClassInitializer
//

class syn_script::rt::SysClassInitializer : public BasicSysClassInitializer {
	NONCOPYABLE(SysClassInitializer);

public:
	class Adapter {
		NONCOPYABLE(Adapter);

	public:
		Adapter(){}
		virtual ~Adapter(){}
		virtual gc::Local<SysClass> create_sys_class(NameRegistry& name_registry) = 0;
	};

	std::unique_ptr<Adapter> m_adapter;

public:
	SysClassInitializer(Adapter*&& adapter);
	gc::Local<SysClass> create_sys_class(NameRegistry& name_registry) const override;
};

#endif//SYNSAMPLE_CORE_SYSCLASSBLD_H_INCLUDED
