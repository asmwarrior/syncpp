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

//Scope classes implementation.

#include <algorithm>
#include <atomic>
#include <memory>

#include "ast.h"
#include "ast_combined.h"
#include "common.h"
#include "gc.h"
#include "name.h"
#include "scope.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;
namespace ast = ss::ast;

///////////////////////////////////////////////////////////////////////////////////////////////////
//Plain classes.
///////////////////////////////////////////////////////////////////////////////////////////////////

//
//ScopeID
//

rt::ScopeID::ScopeID(std::size_t id) : m_id(id){}
rt::ScopeID::ScopeID() : m_id(BAD_ID){}

std::size_t rt::ScopeID::get_id() const {
	return m_id;
}

bool rt::ScopeID::operator==(const ScopeID& id) const {
	return m_id == id.m_id;
}

bool rt::ScopeID::operator!=(const ScopeID& id) const {
	return m_id != id.m_id;
}

rt::ScopeID::operator bool() const {
	return m_id != BAD_ID;
}

bool rt::ScopeID::operator!() const {
	return m_id == BAD_ID;
}

//
//ScopeDescriptor
//

void rt::ScopeDescriptor::gc_enumerate_refs() {
	Object::gc_enumerate_refs();
	gc_ref(m_accessible_scopes);
}

void rt::ScopeDescriptor::initialize(
	const ScopeID& id,
	const ScopeID& outer_id,
	std::size_t scope_idx,
	std::size_t size,
	const gc::Local<ScopeIDArray>& accessible_scopes)
{
	m_id = id;
	m_outer_id = outer_id;
	m_scope_idx = scope_idx;
	m_size = size;
	m_accessible_scopes = accessible_scopes;
}

const rt::ScopeID& rt::ScopeDescriptor::get_id() const {
	return m_id;
}

const rt::ScopeID& rt::ScopeDescriptor::get_outer_id() const {
	return m_outer_id;
}

std::size_t rt::ScopeDescriptor::get_scope_idx() const {
	return m_scope_idx;
}

std::size_t rt::ScopeDescriptor::get_size() const {
	return m_size;
}

bool rt::ScopeDescriptor::is_scope_accessible(const ScopeID& id) const {
	for (const ScopeID& acc_id : *m_accessible_scopes) {
		if (acc_id == id) return true;
	}
	return false;
}

//
//StatementResult
//

rt::StatementResult::StatementResult(StatementResultType type) {
	assert(rt::StatementResultType::NONE == type
		|| rt::StatementResultType::BREAK == type
		|| rt::StatementResultType::CONTINUE == type);
	m_type = type;
}

rt::StatementResult::StatementResult(StatementResultType type, const gc::Local<Value>& value) {
	assert(rt::StatementResultType::RETURN == type
		|| rt::StatementResultType::THROW == type);
	assert(!!value);
	m_type = type;
	m_value = value;
}

rt::StatementResultType rt::StatementResult::get_type() const {
	return m_type;
}

gc::Local<rt::Value> rt::StatementResult::get_value() const {
	return m_value;
}

rt::StatementResult rt::StatementResult::none() {
	return StatementResult(StatementResultType::NONE);
}

rt::StatementResult rt::StatementResult::exception(const gc::Local<Value>& exception) {
	return StatementResult(StatementResultType::THROW, exception);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//Non-GC classes.
///////////////////////////////////////////////////////////////////////////////////////////////////

//
//FieldNameDescriptor : definition
//

class rt::FieldNameDescriptor : public NameDescriptor {
	NONCOPYABLE(FieldNameDescriptor);

	std::size_t m_name_ofs;

protected:
	FieldNameDescriptor(){}
	void initialize(const ScopeID& scope_id, std::size_t scope_ofs, std::size_t name_ofs);

public:
	gc::Local<Value> get(const gc::Local<ExecScope>& scope) const override final;
	void set_initialize(const gc::Local<ExecScope>& scope, const gc::Local<Value>& value) const override final;

protected:
	std::size_t get_name_ofs() const;
};

//
//VariableNameDescriptor : definition
//

class rt::VariableNameDescriptor : public FieldNameDescriptor {
	NONCOPYABLE(VariableNameDescriptor);

public:
	VariableNameDescriptor(){}
	void initialize(const ScopeID& scope_id, std::size_t scope_ofs, std::size_t name_ofs);

	DeclarationType get_declaration_type() const override;
	void set_modify(const gc::Local<ExecScope>& scope, const gc::Local<Value>& value) const override final;
};

//
//ConstantNameDescriptor : definition
//

class rt::ConstantNameDescriptor : public FieldNameDescriptor {
	NONCOPYABLE(ConstantNameDescriptor);

public:
	ConstantNameDescriptor(){}
	void initialize(const ScopeID& scope_id, std::size_t scope_ofs, std::size_t name_ofs);

	DeclarationType get_declaration_type() const override;
};

//
//FunctionNameDescriptor : definition
//

class rt::FunctionNameDescriptor : public NameDescriptor {
	NONCOPYABLE(FunctionNameDescriptor);

	gc::Ref<ast::FunctionDeclaration> m_declaration;

public:
	FunctionNameDescriptor(){}
	void gc_enumerate_refs() override;
	void initialize(const ScopeID& scope_id, std::size_t scope_ofs, const gc::Local<ast::FunctionDeclaration>& declaration);

	DeclarationType get_declaration_type() const override;
	gc::Local<Value> get(const gc::Local<ExecScope>& scope) const override;
};

//
//ClassNameDescriptor : definition
//

class rt::ClassNameDescriptor : public NameDescriptor {
	NONCOPYABLE(ClassNameDescriptor);

	gc::Ref<ast::ClassDeclaration> m_declaration;

public:
	ClassNameDescriptor(){}
	void gc_enumerate_refs() override;
	void initialize(const ScopeID& scope_id, std::size_t scope_ofs, const gc::Local<ast::ClassDeclaration>& declaration);

	DeclarationType get_declaration_type() const override;
	gc::Local<Value> get(const gc::Local<ExecScope>& scope) const override;
};

//
//BindContext::Internal
//

class rt::BindContext::Internal {
	NONCOPYABLE(Internal);

	std::atomic<std::size_t> m_scope_id_sequence;

public:
	Internal(){}

	std::size_t allocate_scope_id() {
		if (m_scope_id_sequence >= SIZE_MAX) throw std::runtime_error("Scope ID overflow");
		return m_scope_id_sequence++;
	}
};

//
//BindContext
//

rt::BindContext::BindContext(ss::NameTable& name_table, const ValueFactory& value_factory)
: m_internal(new Internal()),
m_name_table(name_table),
m_value_factory(value_factory)
{}

rt::BindContext::~BindContext() {
	delete m_internal;
}

ss::NameTable& rt::BindContext::get_name_table() const {
	return m_name_table;
}

const rt::ValueFactory* rt::BindContext::get_value_factory() const {
	return &m_value_factory;
}

std::unique_ptr<rt::BindScope> rt::BindContext::create_root_scope() {
	//Cannot use make_unique() because of GCC.
	return std::unique_ptr<BindScope>(new BindScope(this, nullptr, 0, BAD_OFS));
}

rt::ScopeID rt::BindContext::allocate_scope_id() {
	return ScopeID(m_internal->allocate_scope_id());
}

//
//BindScope
//

rt::BindScope::BindScope(
	BindContext* context,
	BindScope* outer_scope,
	std::size_t scope_ofs,
	std::size_t this_scope_ofs,
	bool loop)
	: m_context(context),
	m_outer_scope(outer_scope),
	m_scope_ofs(scope_ofs),
	m_id(context->allocate_scope_id()),
	m_this_scope_ofs(this_scope_ofs),
	m_loop(loop),
	m_closed(false)
{}

rt::BindScope::BindScope(
	BindContext* context,
	BindScope* outer_scope,
	std::size_t scope_ofs,
	std::size_t this_scope_ofs)
	: BindScope(context, outer_scope, scope_ofs, this_scope_ofs, false)
{}

rt::BindScope* rt::BindScope::get_outer_scope() const {
	return m_outer_scope;
}

const rt::ScopeID& rt::BindScope::get_id() const {
	return m_id;
}
std::size_t rt::BindScope::get_this_scope_ofs() const {
	return m_this_scope_ofs;
}

bool rt::BindScope::is_loop_control_statement_allowed() const {
	return m_loop;
}

gc::Local<rt::NameDescriptor> rt::BindScope::lookup(const gc::Local<ast::AstName>& name) const {
	const BindScope* scope = this;
	while (scope) {
		gc::Local<NameDescriptor> descriptor = scope->lookup_0(name->get_id());
		if (!!descriptor) return descriptor;
		scope = scope->get_outer_scope();
	}

	throw CompilationError(name->pos(), std::string("Name not found: ") + name->get_str()->get_std_string());
}

bool rt::BindScope::contains_name(const ss::NameID& name) const {
	const BindScope* scope = this;
	while (scope) {
		if (scope->contains_name_0(name)) return true;
		scope = scope->get_outer_scope();
	}
	return false;
}

gc::Local<rt::NameDescriptor> rt::BindScope::declare_variable(const gc::Local<ast::AstName>& name, bool constant) {
	check_name_conflict(name);

	std::size_t idx = m_idx_to_name.size();

	gc::Local<FieldNameDescriptor> desc;
	if (constant) {
		desc = gc::create<ConstantNameDescriptor>(m_id, m_scope_ofs, idx);
	} else {
		desc = gc::create<VariableNameDescriptor>(m_id, m_scope_ofs, idx);
	}

	m_idx_to_name.push_back(name->get_info());
	m_name_to_desc[name->get_id()] = desc;
	return desc;
}

gc::Local<rt::NameDescriptor> rt::BindScope::declare_function(
	const gc::Local<ast::AstName>& name,
	const gc::Local<ast::FunctionDeclaration>& declaration)
{
	check_name_conflict(name);
	gc::Local<NameDescriptor> desc = gc::create<FunctionNameDescriptor>(m_id, m_scope_ofs, declaration);
	m_name_to_desc[name->get_id()] = desc;
	return desc;
}

gc::Local<rt::NameDescriptor> rt::BindScope::declare_class(
	const gc::Local<ast::AstName>& name,
	const gc::Local<ast::ClassDeclaration>& declaration)
{
	check_name_conflict(name);
	gc::Local<NameDescriptor> desc = gc::create<ClassNameDescriptor>(m_id, m_scope_ofs, declaration);
	m_name_to_desc[name->get_id()] = desc;
	return desc;
}

gc::Local<rt::NameDescriptor> rt::BindScope::declare_sys_constant(
	const gc::Local<const ss::NameInfo>& name_info)
{
	check_name_conflict(name_info, nullptr);

	std::size_t idx = m_idx_to_name.size();
	gc::Local<NameDescriptor> desc = gc::create<ConstantNameDescriptor>(m_id, m_scope_ofs, idx);
	m_idx_to_name.push_back(name_info);
	m_name_to_desc[name_info->get_id()] = desc;

	return desc;
}

std::unique_ptr<rt::BindScope> rt::BindScope::create_nested_scope(bool nested_this_allowed) {
	check_not_closed();
	std::size_t sub_scope_ofs = m_scope_ofs + 1;
	std::size_t sub_this_scope_ofs = nested_this_allowed ? sub_scope_ofs : m_this_scope_ofs;
	return std::unique_ptr<BindScope>(new BindScope(m_context, this, sub_scope_ofs, sub_this_scope_ofs));
}

std::unique_ptr<rt::BindScope> rt::BindScope::create_nested_block(bool nested_loop) {
	check_not_closed();
	std::size_t sub_scope_ofs = m_scope_ofs + 1;
	bool sub_loop = m_loop | nested_loop;
	return std::unique_ptr<BindScope>(new BindScope(m_context, this, sub_scope_ofs, m_this_scope_ofs, sub_loop));
}

gc::Local<rt::ScopeDescriptor> rt::BindScope::create_scope_descriptor() {
	check_not_closed();
	std::size_t size = m_idx_to_name.size();
	ScopeID outer_id = m_outer_scope ? m_outer_scope->get_id() : ScopeID();
	gc::Local<ScopeIDArray> accessible_scopes = get_accessible_scopes();
	gc::Local<ScopeDescriptor> desc = gc::create<ScopeDescriptor>(m_id, outer_id, m_scope_ofs, size, accessible_scopes);
	m_closed = true;
	return desc;
}

gc::Local<rt::NameDescriptor> rt::BindScope::lookup_0(const ss::NameID& name_id) const {
	auto desc_iter = m_name_to_desc.find(name_id);
	return desc_iter != m_name_to_desc.end() ? desc_iter->second : nullptr;
}

bool rt::BindScope::contains_name_0(const ss::NameID& name_id) const {
	return m_name_to_desc.find(name_id) != m_name_to_desc.end();
}

void rt::BindScope::check_not_closed() const {
	if (m_closed) throw SystemError("BindScope is closed");
}

void rt::BindScope::check_name_conflict(const gc::Local<ast::AstName>& name) const {
	check_name_conflict(name->get_info(), name->pos());
}

void rt::BindScope::check_name_conflict(
	const gc::Local<const ss::NameInfo>& name_info,
	const gc::Local<ss::TextPos>& text_pos) const
{
	check_not_closed();

	const NameID& name_id = name_info->get_id();
	if (contains_name(name_id)) {
		throw CompilationError(text_pos, std::string("Name conflict: ") + name_info->get_str()->get_std_string());
	}
}

gc::Local<rt::ScopeIDArray> rt::BindScope::get_accessible_scopes() const {
	std::size_t count = 0;
	const BindScope* scope = this;
	while (scope) {
		++count;
		scope = scope->m_outer_scope;
	}

	gc::Local<ScopeIDArray> scopes = ScopeIDArray::create(count);
	scope = this;
	while (scope) {
		assert(count);
		--count;
		(*scopes)[count] = scope->m_id;
		scope = scope->m_outer_scope;
	}
	assert(!count);

	return scopes;
}

//
//NameDescriptor
//

void rt::NameDescriptor::initialize(const ScopeID& scope_id, std::size_t scope_ofs) {
	m_scope_id = scope_id;
	m_scope_ofs = scope_ofs;
}

void rt::NameDescriptor::set_initialize(const gc::Local<ExecScope>& scope, const gc::Local<Value>& value) const {
	throw SystemError("Cannot modify value");
}

void rt::NameDescriptor::set_modify(const gc::Local<ExecScope>& scope, const gc::Local<Value>& value) const {
	throw SystemError("Cannot modify value");
}

const rt::ScopeID& rt::NameDescriptor::get_scope_id() const {
	return m_scope_id;
}

std::size_t rt::NameDescriptor::get_scope_ofs() const {
	return m_scope_ofs;
}

//
//FieldNameDescriptor : implementation
//

void rt::FieldNameDescriptor::initialize(const ScopeID& scope_id, std::size_t scope_ofs, std::size_t name_ofs) {
	NameDescriptor::initialize(scope_id, scope_ofs),
	m_name_ofs = name_ofs;
}

void rt::FieldNameDescriptor::set_initialize(const gc::Local<ExecScope>& scope, const gc::Local<Value>& value) const {
	assert(!!value);
	assert(!value->is_void());
	gc::Ref<Value>& ref = scope->get(get_scope_id(), get_scope_ofs(), get_name_ofs());
	assert(!!ref);
	assert(ref->is_undefined());
	ref = value;
}

gc::Local<rt::Value> rt::FieldNameDescriptor::get(const gc::Local<ExecScope>& scope) const {
	return scope->get(get_scope_id(), get_scope_ofs(), m_name_ofs);
}

std::size_t rt::FieldNameDescriptor::get_name_ofs() const {
	return m_name_ofs;
}

//
//VariableNameDescriptor : implementation
//

void rt::VariableNameDescriptor::initialize(const ScopeID& scope_id, std::size_t scope_ofs, std::size_t name_ofs) {
	FieldNameDescriptor::initialize(scope_id, scope_ofs, name_ofs);
}

rt::DeclarationType rt::VariableNameDescriptor::get_declaration_type() const {
	return DeclarationType::VARIABLE;
}

void rt::VariableNameDescriptor::set_modify(const gc::Local<ExecScope>& scope, const gc::Local<Value>& value) const {
	assert(!value->is_void());
	gc::Ref<Value>& ref = scope->get(get_scope_id(), get_scope_ofs(), get_name_ofs());
	ref = value;
}

//
//ConstantNameDescriptor
//

void rt::ConstantNameDescriptor::initialize(const ScopeID& scope_id, std::size_t scope_ofs, std::size_t name_ofs) {
	FieldNameDescriptor::initialize(scope_id, scope_ofs, name_ofs);
}

rt::DeclarationType rt::ConstantNameDescriptor::get_declaration_type() const {
	return DeclarationType::CONSTANT;
}

//
//FunctionNameDescriptor
//

void rt::FunctionNameDescriptor::gc_enumerate_refs() {
	NameDescriptor::gc_enumerate_refs();
	gc_ref(m_declaration);
}

void rt::FunctionNameDescriptor::initialize(
	const ScopeID& scope_id,
	std::size_t scope_ofs,
	const gc::Local<ast::FunctionDeclaration>& declaration)
{
	NameDescriptor::initialize(scope_id, scope_ofs);
	m_declaration = declaration;
}

rt::DeclarationType rt::FunctionNameDescriptor::get_declaration_type() const {
	return DeclarationType::FUNCTION;
}

gc::Local<rt::Value> rt::FunctionNameDescriptor::get(const gc::Local<ExecScope>& scope) const {
	gc::Local<ExecScope> target_scope = scope->get_target_scope(get_scope_id(), get_scope_ofs());
	return gc::create<rt::FunctionValue>(target_scope, m_declaration->get_expression().local());
}

//
//ClassNameDescriptor
//

void rt::ClassNameDescriptor::gc_enumerate_refs() {
	NameDescriptor::gc_enumerate_refs();
	gc_ref(m_declaration);
}

void rt::ClassNameDescriptor::initialize(
	const ScopeID& scope_id,
	std::size_t scope_ofs,
	const gc::Local<ast::ClassDeclaration>& declaration)
{
	NameDescriptor::initialize(scope_id, scope_ofs),
	m_declaration = declaration;
}

rt::DeclarationType rt::ClassNameDescriptor::get_declaration_type() const {
	return DeclarationType::CLASS;
}

gc::Local<rt::Value> rt::ClassNameDescriptor::get(const gc::Local<ExecScope>& scope) const {
	gc::Local<ExecScope> target_scope = scope->get_target_scope(get_scope_id(), get_scope_ofs());
	return gc::create<rt::ClassValue>(target_scope, m_declaration->get_expression().local());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//GC classes.
///////////////////////////////////////////////////////////////////////////////////////////////////

//
//ExecContext
//

void rt::ExecContext::initialize(BindContext* bind_context) {
	m_bind_context = bind_context;
}

gc::Local<rt::ExecScope> rt::ExecContext::create_root_scope(const gc::Local<ScopeDescriptor>& scope_descriptor) {
	return gc::create<ExecScope>(self(this), scope_descriptor, nullptr, nullptr);
}

rt::BindContext* rt::ExecContext::get_bind_context() {
	return m_bind_context;
}

const rt::ValueFactory* rt::ExecContext::get_value_factory() const {
	return m_bind_context->get_value_factory();
}

gc::Local<rt::Value> rt::ExecContext::get_undefined_value() const {
	return get_value_factory()->get_undefined_value();
}

//
//ExecScope
//

rt::ExecScope::ExecScope(){}

void rt::ExecScope::gc_enumerate_refs() {
	gc_ref(m_context);
	gc_ref(m_descriptor);
	gc_ref(m_outer_scope);
	gc_ref(m_values);
	gc_ref(m_this_value);
}

void rt::ExecScope::initialize(
	const gc::Local<ExecContext>& context,
	const gc::Local<ScopeDescriptor>& descriptor,
	const gc::Local<ExecScope>& outer_scope,
	const gc::Local<Value>& this_value)
{
	m_context = context;
	m_descriptor = descriptor;

	ScopeID outer_id = descriptor->get_outer_id();
	if (outer_id) {
		assert(!!outer_scope);
		assert(outer_id == outer_scope->get_id());
	} else {
		assert(!outer_scope);
	}

	m_outer_scope = outer_scope;
	m_scope_idx = !!outer_scope ? outer_scope->m_scope_idx + 1 : 0;
	m_this_value = this_value;

	m_values = gc::Array<Value>::create(descriptor->get_size());
	gc::Local<Value> undefined = context->get_value_factory()->get_undefined_value();
	for (std::size_t i = 0, n = m_values->length(); i < n; ++i) m_values->get(i) = undefined;
}

const rt::ScopeID& rt::ExecScope::get_id() const {
	return m_descriptor->get_id();
}

const gc::Ref<rt::ScopeDescriptor>& rt::ExecScope::get_scope_descriptor() const {
	return m_descriptor;
}

void rt::ExecScope::check_id(const ScopeID& expected_id) {
	if (get_id() != expected_id) throw SystemError("Scope ID missmatch");
}

void rt::ExecScope::check_idx(std::size_t expected_idx) {
	if (m_scope_idx != expected_idx) throw SystemError("Scope index missmatch");
}

gc::Ref<rt::Value>& rt::ExecScope::get(
	const ScopeID& scope_id,
	std::size_t scope_ofs,
	std::size_t name_ofs)
{
	gc::Local<ExecScope> scope = get_target_scope(scope_id, scope_ofs);
	gc::Ref<Value>& ref = scope->m_values->get(name_ofs);
	return ref;
}

gc::Local<rt::Value> rt::ExecScope::get_this(std::size_t scope_ofs) {
	if (!m_this_value) throw SystemError("No 'this' in current scope");
	return m_this_value;
}

gc::Local<rt::ExecScope> rt::ExecScope::create_nested_scope(
	const gc::Local<ScopeDescriptor>& scope_descriptor,
	const gc::Local<Value>& sub_this_value)
{
	return gc::create<ExecScope>(
		gc::Local<ExecContext>(m_context),
		scope_descriptor,
		self(this),
		!!sub_this_value ? sub_this_value : m_this_value);
}

gc::Local<rt::ExecScope> rt::ExecScope::get_target_scope(const ScopeID& scope_id, std::size_t scope_ofs) {
	gc::Local<ExecScope> scope = self(this);
	while (!!scope && scope->m_scope_idx > scope_ofs) scope = scope->m_outer_scope;
	if (!scope || scope->m_scope_idx != scope_ofs) throw SystemError("Target scope not found");

	scope->check_id(scope_id);
	return scope;
}
