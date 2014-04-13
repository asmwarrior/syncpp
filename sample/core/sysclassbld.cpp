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

//SysClass builder implementation.

#include <list>
#include <map>
#include <memory>
#include <vector>

#include "ast.h"
#include "api_basic.h"
#include "common.h"
#include "gc.h"
#include "name.h"
#include "stringex.h"
#include "sysclass.h"
#include "sysclass__int.h"
#include "sysclassbld__imp.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;
namespace ast = ss::ast;


namespace syn_script {
	namespace rt {
		class MemberAccess;
		class FieldAccess;
		class MethodAccess;
		class FunctionStaticFieldAccess;
		class FunctionDynamicFieldAccess;
		class FunctionStaticMethodAccess;
		class FunctionDynamicMethodAccess;
		class MethodValue;
		class StaticMethodValue;
		class DynamicMethodValue;
		class ConcreteSysConstructor;
		class SysField;
		class SysMethod;

		typedef gc::Array<MethodAccess> MethodAccessArray;
		typedef std::vector<gc::Local<MethodAccess>> MethodAccessVector;

		gc::Local<rt::MethodAccess> find_appropriate_method(
			const gc::Local<MethodAccessArray>& methods,
			const gc::Local<gc::Array<rt::Value>>& arguments);

		gc::Local<MethodAccessArray> create_methods_array(const MethodAccessVector& vec);
	}
}

namespace {
	typedef std::vector<const rt::BasicSysClassInitializer*> ClassInitVector;

	ClassInitVector& get_sys_class_initializers() {
		static ClassInitVector vec;
		return vec;
	}

	std::size_t add_sys_class_initializer(rt::BasicSysClassInitializer* init) {
		ClassInitVector& vec = get_sys_class_initializers();
		std::size_t id = vec.size();
		vec.push_back(init);
		return id;
	}

	struct NameKey {
		gc::Local<const ss::NameInfo> m_name_info;

		NameKey() : m_name_info(nullptr){}
		explicit NameKey(const gc::Local<const ss::NameInfo>& name_info) : m_name_info(name_info){}
	};

	bool operator==(const NameKey& a, const NameKey& b) {
		if (!a.m_name_info) {
			return !b.m_name_info;
		} else if (!b.m_name_info) {
			return false;
		}
		return a.m_name_info->get_id() == b.m_name_info->get_id();
	}

	bool operator<(const NameKey& a, const NameKey& b) {
		if (!a.m_name_info) {
			return !!b.m_name_info;
		} else if (!b.m_name_info) {
			return false;
		}
		return a.m_name_info->get_id() < b.m_name_info->get_id();
	}
}

//
//MemberAccess : definition
//

class rt::MemberAccess : public gc::Object {
	NONCOPYABLE(MemberAccess);

protected:
	MemberAccess(){}
};

//
//FieldAccess : definition
//

class rt::FieldAccess : public MemberAccess {
	NONCOPYABLE(FieldAccess);

protected:
	FieldAccess();

public:
	virtual gc::Local<Value> get_static(const gc::Local<ExecContext>& context) const;
	virtual gc::Local<Value> get(const gc::Local<ExecContext>& context, const gc::Local<Value>& object) const;
};

//
//MethodAccess : definition
//

class rt::MethodAccess : public MemberAccess {
	NONCOPYABLE(MethodAccess);

protected:
	MethodAccess();

public:
	virtual bool is_static() const = 0;
	virtual bool accepts_arguments(const gc::Local<gc::Array<Value>>& arguments) const = 0;

	virtual gc::Local<Value> invoke_static(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments) const;

	virtual gc::Local<Value> invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments,
		const gc::Local<Value>& object) const;
};

//
//FunctionStaticFieldAccess : definition
//

class rt::FunctionStaticFieldAccess : public FieldAccess {
	NONCOPYABLE(FunctionStaticFieldAccess);

	gc::Ref<StaticFieldAdapter> m_adapter;

public:
	FunctionStaticFieldAccess();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<StaticFieldAdapter>& adapter);

	gc::Local<Value> get_static(const gc::Local<ExecContext>& context) const override;
};

//
//FunctionDynamicFieldAccess : definition
//

class rt::FunctionDynamicFieldAccess : public FieldAccess {
	NONCOPYABLE(FunctionDynamicFieldAccess);

	gc::Ref<DynamicFieldAdapter> m_adapter;

public:
	FunctionDynamicFieldAccess();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<DynamicFieldAdapter>& adapter);

	gc::Local<Value> get(const gc::Local<ExecContext>& context, const gc::Local<Value>& object) const override;
};

//
//FunctionStaticMethodAccess : definition
//

class rt::FunctionStaticMethodAccess : public MethodAccess {
	NONCOPYABLE(FunctionStaticMethodAccess);

	gc::Ref<StaticMethodAdapter> m_adapter;

public:
	FunctionStaticMethodAccess();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<StaticMethodAdapter>& adapter);

	bool is_static() const override;
	bool accepts_arguments(const gc::Local<gc::Array<Value>>& arguments) const override;

	gc::Local<Value> invoke_static(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments) const override;
};

//
//FunctionDynamicMethodAccess : definition
//

class rt::FunctionDynamicMethodAccess : public MethodAccess {
	NONCOPYABLE(FunctionDynamicMethodAccess);

	gc::Ref<DynamicMethodAdapter> m_adapter;

public:
	FunctionDynamicMethodAccess();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<DynamicMethodAdapter>& adapter);

	bool is_static() const override;
	bool accepts_arguments(const gc::Local<gc::Array<Value>>& arguments) const override;

	gc::Local<Value> invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments,
		const gc::Local<Value>& object) const override;
};

//
//MethodValue : definition
//

class rt::MethodValue : public Value {
	NONCOPYABLE(MethodValue);

	gc::Ref<MethodAccessArray> m_methods;

protected:
	MethodValue();

public:
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<MethodAccessArray>& methods);

	gc::Local<Value> invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments,
		gc::Local<Value>& exception) override final;

protected:
	virtual gc::Local<Value> do_invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments,
		const gc::Local<MethodAccess>& method) const = 0;
};

//
//StaticMethodValue : definition
//

class rt::StaticMethodValue : public MethodValue {
	NONCOPYABLE(StaticMethodValue);

public:
	StaticMethodValue();

protected:
	gc::Local<Value> do_invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments,
		const gc::Local<MethodAccess>& method) const override;
};

//
//DynamicMethodValue : definition
//

class rt::DynamicMethodValue : public MethodValue {
	NONCOPYABLE(DynamicMethodValue);

	gc::Ref<Value> m_object;

public:
	DynamicMethodValue();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<MethodAccessArray>& methods, const gc::Local<Value>& object);

protected:
	gc::Local<Value> do_invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments,
		const gc::Local<MethodAccess>& method) const override;
};

//
//BasicSysClassBuilder::InternalBuilder : definition
//

class rt::BasicSysClassBuilder::InternalBuilder {
	NONCOPYABLE(InternalBuilder);

	NameRegistry& m_name_registry;

	std::map<NameKey, gc::Local<FieldAccess>> m_fields;
	std::map<NameKey, MethodAccessVector> m_methods;
	MethodAccessVector m_constructors;

public:
	InternalBuilder(NameRegistry& name_registry);

	void add_field(const char* name, const gc::Local<FieldAccess>& field);
	void add_method(const char* name, const gc::Local<MethodAccess>& method);
	void add_constructor(const gc::Local<MethodAccess>& method);

	gc::Local<SysClass> create_sys_class() const;

private:
	gc::Local<SysMethod> create_sys_method(
		const gc::Local<const NameInfo>& name_info,
		const MethodAccessVector& methods) const;
};

//
//ConcreteSysConstructor : definition
//

class rt::ConcreteSysConstructor : public SysConstructor {
	NONCOPYABLE(ConcreteSysConstructor);

	gc::Ref<MethodAccessArray> m_methods;

public:
	ConcreteSysConstructor();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<MethodAccessArray>& methods);

	gc::Local<Value> instantiate(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments) const override;
};

//
//SysField : definition
//

class rt::SysField : public SysMember {
	NONCOPYABLE(SysField);

	gc::Ref<FieldAccess> m_field_access;

public:
	SysField();
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<const NameInfo>& name_info, const gc::Local<FieldAccess> field_access);

	gc::Local<Value> get(const gc::Local<ExecContext>& context, const gc::Local<Value>& object) const override;
	gc::Local<Value> get_static(const gc::Local<ExecContext>& context) const override;
};

//
//SysMethod : definition
//

class rt::SysMethod : public SysMember {
	NONCOPYABLE(SysMethod);

	gc::Ref<Value> m_static_value;
	gc::Ref<MethodAccessArray> m_method_accesses;

public:
	SysMethod();
	void gc_enumerate_refs() override;

	void initialize(
		const gc::Local<const NameInfo>& name_info,
		const gc::Local<Value>& static_value,
		const gc::Local<MethodAccessArray>& method_accesses);

	gc::Local<Value> get(const gc::Local<ExecContext>& context, const gc::Local<Value>& object) const override;
	gc::Local<Value> get_static(const gc::Local<ExecContext>& context) const override;
};

//
//BasicSysClassBuilder
//

rt::BasicSysClassBuilder::BasicSysClassBuilder(ss::NameRegistry& name_registry)
: m_internal(new InternalBuilder(name_registry))
{}

//The destructor has to be defined in a .cpp file - in order to make unique_ptr work with undefined class InternalBuilder.
rt::BasicSysClassBuilder::~BasicSysClassBuilder(){}

void rt::BasicSysClassBuilder::add_constructor_0(const gc::Local<StaticMethodAdapter>& adapter) {
	m_internal->add_constructor(gc::create<FunctionStaticMethodAccess>(adapter));
}

void rt::BasicSysClassBuilder::add_static_field_0(const char* name, const gc::Local<StaticFieldAdapter>& adapter) {
	m_internal->add_field(name, gc::create<FunctionStaticFieldAccess>(adapter));
}

void rt::BasicSysClassBuilder::add_static_method_0(const char* name, const gc::Local<StaticMethodAdapter>& adapter) {
	m_internal->add_method(name, gc::create<FunctionStaticMethodAccess>(adapter));
}

void rt::BasicSysClassBuilder::add_field_0(const char* name, const gc::Local<DynamicFieldAdapter>& adapter) {
	m_internal->add_field(name, gc::create<FunctionDynamicFieldAccess>(adapter));
}

void rt::BasicSysClassBuilder::add_method_0(const char* name, const gc::Local<DynamicMethodAdapter>& adapter) {
	m_internal->add_method(name, gc::create<FunctionDynamicMethodAccess>(adapter));
}

gc::Local<rt::SysClass> rt::BasicSysClassBuilder::create_sys_class() const {
	return m_internal->create_sys_class();
}

//
//BasicSysClassInitializer
//

rt::BasicSysClassInitializer::BasicSysClassInitializer()
: m_class_id(add_sys_class_initializer(this))
{}

const std::vector<const rt::BasicSysClassInitializer*>& rt::BasicSysClassInitializer::get_class_initializers() {
	return ::get_sys_class_initializers();
}

std::size_t rt::BasicSysClassInitializer::get_class_id() const {
	return m_class_id;
}

//
//SysClassInitializer
//

rt::SysClassInitializer::SysClassInitializer(Adapter*&& adapter)
: m_adapter(adapter)
{}

gc::Local<rt::SysClass> rt::SysClassInitializer::create_sys_class(ss::NameRegistry& name_registry) const {
	return m_adapter->create_sys_class(name_registry);
}

//
//FieldAccess : implementation
//

rt::FieldAccess::FieldAccess(){}

gc::Local<rt::Value> rt::FieldAccess::get_static(const gc::Local<ExecContext>& context) const {
	throw RuntimeError("Not a static field");
}

gc::Local<rt::Value> rt::FieldAccess::get(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& object) const
{
	return get_static(context);
}

//
//MethodAccess : implementation
//

rt::MethodAccess::MethodAccess(){}

gc::Local<rt::Value> rt::MethodAccess::invoke_static(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments) const
{
	assert(false);
	throw SystemError("Not a static method");
}

gc::Local<rt::Value> rt::MethodAccess::invoke(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	const gc::Local<Value>& object) const
{
	return invoke_static(context, arguments);
}

//
//FunctionStaticFieldAccess : implementation
//

rt::FunctionStaticFieldAccess::FunctionStaticFieldAccess(){}

void rt::FunctionStaticFieldAccess::gc_enumerate_refs() {
	gc_ref(m_adapter);
}

void rt::FunctionStaticFieldAccess::initialize(const gc::Local<StaticFieldAdapter>& adapter) {
	m_adapter = adapter;
}

gc::Local<rt::Value> rt::FunctionStaticFieldAccess::get_static(const gc::Local<ExecContext>& context) const {
	return m_adapter->get(context);
}

//
//FunctionDynamicFieldAccess : implementation
//

rt::FunctionDynamicFieldAccess::FunctionDynamicFieldAccess(){}

void rt::FunctionDynamicFieldAccess::gc_enumerate_refs() {
	gc_ref(m_adapter);
}

void rt::FunctionDynamicFieldAccess::initialize(const gc::Local<DynamicFieldAdapter>& adapter) {
	m_adapter = adapter;
}

gc::Local<rt::Value> rt::FunctionDynamicFieldAccess::get(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& object) const
{
	return m_adapter->get(context, object);
}

//
//FunctionStaticMethodAccess : implementation
//

rt::FunctionStaticMethodAccess::FunctionStaticMethodAccess(){}

void rt::FunctionStaticMethodAccess::gc_enumerate_refs() {
	gc_ref(m_adapter);
}

void rt::FunctionStaticMethodAccess::initialize(const gc::Local<StaticMethodAdapter>& adapter) {
	m_adapter = adapter;
}

bool rt::FunctionStaticMethodAccess::is_static() const {
	return true;
}

bool rt::FunctionStaticMethodAccess::accepts_arguments(const gc::Local<gc::Array<Value>>& arguments) const {
	return m_adapter->accepts_arguments(arguments);
}

gc::Local<rt::Value> rt::FunctionStaticMethodAccess::invoke_static(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments) const
{
	return m_adapter->invoke(context, arguments);
}

//
//FunctionDynamicMethodAccess : definition
//

rt::FunctionDynamicMethodAccess::FunctionDynamicMethodAccess(){}

void rt::FunctionDynamicMethodAccess::gc_enumerate_refs() {
	gc_ref(m_adapter);
}

void rt::FunctionDynamicMethodAccess::initialize(const gc::Local<DynamicMethodAdapter>& adapter) {
	m_adapter = adapter;
}

bool rt::FunctionDynamicMethodAccess::is_static() const {
	return false;
}

bool rt::FunctionDynamicMethodAccess::accepts_arguments(const gc::Local<gc::Array<Value>>& arguments) const {
	return m_adapter->accepts_arguments(arguments);
}

gc::Local<rt::Value> rt::FunctionDynamicMethodAccess::invoke(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	const gc::Local<Value>& object) const
{
	return m_adapter->invoke(context, object, arguments);
}

//
//MethodValue : implemenetation
//

rt::MethodValue::MethodValue(){}

void rt::MethodValue::gc_enumerate_refs() {
	gc_ref(m_methods);
}

void rt::MethodValue::initialize(const gc::Local<MethodAccessArray>& methods) {
	m_methods = methods;
}

gc::Local<rt::Value> rt::MethodValue::invoke(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	gc::Local<Value>& exception)
{
	gc::Local<MethodAccess> method = find_appropriate_method(m_methods, arguments);
	return do_invoke(context, arguments, method);
}

//
//StaticMethodValue : implemenetation
//

rt::StaticMethodValue::StaticMethodValue(){}

gc::Local<rt::Value> rt::StaticMethodValue::do_invoke(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	const gc::Local<MethodAccess>& method) const
{
	return method->invoke_static(context, arguments);
}

//
//DynamicMethodValue : implemenetation
//

rt::DynamicMethodValue::DynamicMethodValue(){}

void rt::DynamicMethodValue::gc_enumerate_refs() {
	MethodValue::gc_enumerate_refs();
	gc_ref(m_object);
}

void rt::DynamicMethodValue::initialize(
	const gc::Local<MethodAccessArray>& methods,
	const gc::Local<Value>& object)
{
	MethodValue::initialize(methods);
	m_object = object;
}

gc::Local<rt::Value> rt::DynamicMethodValue::do_invoke(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	const gc::Local<MethodAccess>& method) const
{
	return method->invoke(context, arguments, m_object);
}

//
//BasicSysClassBuilder::InternalBuilder : definition
//

rt::BasicSysClassBuilder::InternalBuilder::InternalBuilder(ss::NameRegistry& name_registry)
: m_name_registry(name_registry)
{}

void rt::BasicSysClassBuilder::InternalBuilder::add_field(
	const char* name,
	const gc::Local<FieldAccess>& field)
{
	gc::Local<const NameInfo> name_info = m_name_registry.register_name(name);
	NameKey name_key(name_info);

	if (m_fields.find(name_key) != m_fields.end()) {
		throw RuntimeError(std::string("Duplicated fields: ") + name);
	}
	if (m_methods.find(name_key) != m_methods.end()) {
		throw RuntimeError(std::string("Field-method conflict: ") + name);
	}

	m_fields[name_key] = field;
}

void rt::BasicSysClassBuilder::InternalBuilder::add_method(
	const char* name,
	const gc::Local<MethodAccess>& method)
{
	gc::Local<const NameInfo> name_info = m_name_registry.register_name(name);
	NameKey name_key(name_info);

	if (m_fields.find(name_key) != m_fields.end()) {
		throw RuntimeError(std::string("Method-field conflict: ") + name);
	}

	m_methods[name_key].push_back(method);
}

void rt::BasicSysClassBuilder::InternalBuilder::add_constructor(const gc::Local<MethodAccess>& method) {
	m_constructors.push_back(method);
}

gc::Local<rt::SysClass> rt::BasicSysClassBuilder::InternalBuilder::create_sys_class() const {
	std::size_t member_cnt = m_fields.size() + m_methods.size();
	gc::Local<gc::Array<SysMember>> members = gc::Array<SysMember>::create(member_cnt);

	std::size_t member_idx = 0;
	for (auto iter : m_fields) {
		members->get(member_idx++) = gc::create<SysField>(iter.first.m_name_info, iter.second);
	}
	for (auto iter : m_methods) {
		members->get(member_idx++) = create_sys_method(iter.first.m_name_info, iter.second);
	}

	gc::Local<SysConstructor> constructor;
	if (!m_constructors.empty()) {
		gc::Local<MethodAccessArray> constructor_methods = create_methods_array(m_constructors);
		constructor = gc::create<ConcreteSysConstructor>(constructor_methods);
	}

	gc::Local<SysClass::InternalClass> internal_class = gc::create<SysClass::InternalClass>(constructor, members);
	return gc::create<SysClass>(internal_class);
}

gc::Local<rt::SysMethod> rt::BasicSysClassBuilder::InternalBuilder::create_sys_method(
	const gc::Local<const ss::NameInfo>& name_info,
	const MethodAccessVector& methods) const
{
	gc::Local<Value> static_value;

	std::size_t static_cnt = 0;
	for (const gc::Local<MethodAccess>& method : methods) {
		if (method->is_static()) ++static_cnt;
	}
	if (static_cnt) {
		gc::Local<MethodAccessArray> static_methods = MethodAccessArray::create(static_cnt);
		std::size_t static_idx = 0;
		for (const gc::Local<MethodAccess>& method : methods) {
			if (method->is_static()) static_methods->get(static_idx++) = method;
		}
		static_value = gc::create<StaticMethodValue>(static_methods);
	}

	gc::Local<MethodAccessArray> all_methods = create_methods_array(methods);
	return gc::create<SysMethod>(name_info, static_value, all_methods);
}

//
//ConcreteSysConstructor : implementation
//

rt::ConcreteSysConstructor::ConcreteSysConstructor(){}

void rt::ConcreteSysConstructor::gc_enumerate_refs() {
	gc_ref(m_methods);
}

void rt::ConcreteSysConstructor::initialize(const gc::Local<MethodAccessArray>& methods) {
	m_methods = methods;
}

gc::Local<rt::Value> rt::ConcreteSysConstructor::instantiate(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments) const
{
	gc::Local<MethodAccess> method = find_appropriate_method(m_methods, arguments);
	return method->invoke_static(context, arguments);
}

//
//SysField : implementation
//

rt::SysField::SysField(){}

void rt::SysField::gc_enumerate_refs() {
	SysMember::gc_enumerate_refs();
	gc_ref(m_field_access);
}

void rt::SysField::initialize(
	const gc::Local<const ss::NameInfo>& name_info,
	const gc::Local<FieldAccess> field_access)
{
	SysMember::initialize(name_info);
	m_field_access = field_access;
}

gc::Local<rt::Value> rt::SysField::get(const gc::Local<ExecContext>& context, const gc::Local<Value>& object) const {
	return m_field_access->get(context, object);
}

gc::Local<rt::Value> rt::SysField::get_static(const gc::Local<ExecContext>& context) const {
	return m_field_access->get_static(context);
}

//
//SysMethod : implementation
//

rt::SysMethod::SysMethod(){}

void rt::SysMethod::gc_enumerate_refs() {
	SysMember::gc_enumerate_refs();
	gc_ref(m_static_value);
	gc_ref(m_method_accesses);
}

void rt::SysMethod::initialize(
	const gc::Local<const ss::NameInfo>& name_info,
	const gc::Local<Value>& static_value,
	const gc::Local<MethodAccessArray>& method_accesses)
{
	SysMember::initialize(name_info);
	m_static_value = static_value;
	m_method_accesses = method_accesses;
}

gc::Local<rt::Value> rt::SysMethod::get(const gc::Local<ExecContext>& context, const gc::Local<Value>& object) const {
	const gc::Local<MethodAccessArray>& methods = m_method_accesses;
	return gc::create<DynamicMethodValue>(methods, object);
}

gc::Local<rt::Value> rt::SysMethod::get_static(const gc::Local<ExecContext>& context) const {
	if (!m_static_value) throw RuntimeError("Not a static method");
	return m_static_value;
}

//
//(Functions)
//

gc::Local<rt::MethodAccess> rt::find_appropriate_method(
	const gc::Local<MethodAccessArray>& methods,
	const gc::Local<gc::Array<Value>>& arguments)
{
	for (std::size_t i = 0, n = methods->length(); i < n; ++i) {
		const gc::Ref<MethodAccess>& method = methods->get(i);
		if (method->accepts_arguments(arguments)) return method;
	}

	throw ss::RuntimeError("Wrong method arguments");
}

gc::Local<rt::MethodAccessArray> rt::create_methods_array(const MethodAccessVector& vec) {
	std::size_t cnt = vec.size();
	gc::Local<MethodAccessArray> methods = MethodAccessArray::create(cnt);
	for (std::size_t i = 0; i < cnt; ++i) methods->get(i) = vec[i];
	return methods;
}

gc::Local<rt::Value> rt::adapter__result(const gc::Local<ExecContext>& context,	bool v) {
	return context->get_value_factory()->get_boolean_value(v);
}

gc::Local<rt::Value> rt::adapter__result(const gc::Local<ExecContext>& context,	ss::ScriptIntegerType v) {
	return context->get_value_factory()->get_integer_value(v);
}

gc::Local<rt::Value> rt::adapter__result(const gc::Local<ExecContext>& context, const ss::StringLoc& v) {
	if (!v) return context->get_value_factory()->get_null_value();
	return context->get_value_factory()->get_string_value(v);
}

gc::Local<rt::Value> rt::adapter__result(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& v)
{
	if (!v) return context->get_value_factory()->get_null_value();
	return gc::create<ArrayValue>(v);
}

bool rt::adapter__convert_argument(
	const gc::Local<Value>& v,
	AdapterTag<bool>)
{
	return v->get_boolean();
}

ss::ScriptIntegerType rt::adapter__convert_argument(
	const gc::Local<Value>& v,
	AdapterTag<ss::ScriptIntegerType>)
{
	return v->get_integer();
}

ss::StringLoc rt::adapter__convert_argument(
	const gc::Local<Value>& v,
	AdapterTag<const ss::StringLoc&>)
{
	return v->get_string();
}

gc::Local<ss::ByteArray> rt::adapter__convert_argument(
	const gc::Local<Value>& v,
	AdapterTag<const gc::Local<ss::ByteArray>&>)
{
	gc::Local<ByteArrayValue> array_value = v.cast_opt<ByteArrayValue>();
	if (!array_value) throw RuntimeError("Wrong argument type");
	return array_value->get_array();
}

const gc::Local<rt::Value>& rt::adapter__convert_argument(
	const gc::Local<Value>& v,
	AdapterTag<const gc::Local<Value>&>)
{
	return v;
}

gc::Local<gc::Array<rt::Value>> rt::adapter__convert_argument(
	const gc::Local<Value>& v,
	AdapterTag<const gc::Local<gc::Array<Value>>&>)
{
	gc::Local<ArrayValue> array_value = v.cast<ArrayValue>();
	return array_value->get_array();
}
