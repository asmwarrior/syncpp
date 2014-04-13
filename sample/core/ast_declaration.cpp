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

//AST: declaration classes.

#include <iostream>

#include "ast.h"
#include "ast_combined.h"
#include "gc.h"
#include "scope.h"
#include "value.h"

namespace ss = syn_script;
namespace ast = ss::ast;
namespace rt = ss::rt;
namespace gc = ss::gc;

//
//Declaration
//

void ast::Declaration::gc_enumerate_refs() {
	Node::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_name);
	gc_ref(m_name_descriptor);
}

void ast::Declaration::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::Declaration::syn_name(const SynName& name) {
	m_syn_name = name;
}

const gc::Ref<ss::TextPos>& ast::Declaration::get_pos() const {
	return m_syn_pos;
}

const gc::Ref<ast::AstName>& ast::Declaration::get_name() const {
	return m_syn_name;
}

const rt::NameDescriptor* ast::Declaration::get_name_descriptor() const {
	return m_name_descriptor.get();
}

ast::ast_ptr<ast::FunctionDeclaration> ast::Declaration::get_function_opt() {
	return nullptr;
}

ast::ModifierType ast::Declaration::get_default_access() const {
	return ModifierType::PRIVATE;
}

void ast::Declaration::bind_declare(rt::BindContext* context, rt::BindScope* scope) {
	m_scope_id = scope->get_id();
	m_name_descriptor = bind_declare_0(context, scope);
}

void ast::Declaration::exec_define(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	scope->check_id(m_scope_id);
	exec_define_0(context, scope, exception, m_name_descriptor.get());
}

//
//VariableDeclaration
//

void ast::VariableDeclaration::gc_enumerate_refs() {
	Declaration::gc_enumerate_refs();
	gc_ref(m_syn_expression);
}

void ast::VariableDeclaration::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

rt::DeclarationType ast::VariableDeclaration::get_declaration_type() const {
	return rt::DeclarationType::VARIABLE;
}

gc::Local<const rt::NameDescriptor> ast::VariableDeclaration::bind_declare_0(
	rt::BindContext* context,
	rt::BindScope* scope)
{
	return scope->declare_variable(get_name(), false);
}

void ast::VariableDeclaration::bind_define(rt::BindContext* context, rt::BindScope* scope) {
	if (!!m_syn_expression) m_syn_expression->bind(context, scope);
}

void ast::VariableDeclaration::exec_define_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	const rt::NameDescriptor* name_desc)
{
	if (!!m_syn_expression) {
		gc::Local<rt::Value> value = m_syn_expression->evaluate(context, scope, exception);
		if (!!exception) return;

		if (value->is_void()) throw RuntimeError(get_pos(), "Cannot initialize a variable with void value");

		name_desc->set_initialize(scope, value);
	}
}

//
//ConstantDeclaration
//

void ast::ConstantDeclaration::gc_enumerate_refs() {
	Declaration::gc_enumerate_refs();
	gc_ref(m_syn_expression);
}

void ast::ConstantDeclaration::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

rt::DeclarationType ast::ConstantDeclaration::get_declaration_type() const {
	return rt::DeclarationType::CONSTANT;
}

gc::Local<const rt::NameDescriptor> ast::ConstantDeclaration::bind_declare_0(
	rt::BindContext* context,
	rt::BindScope* scope)
{
	return scope->declare_variable(get_name(), true);
}

void ast::ConstantDeclaration::bind_define(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_expression->bind(context, scope);
}

void ast::ConstantDeclaration::exec_define_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	const rt::NameDescriptor* name_desc)
{
	gc::Local<rt::Value> value = m_syn_expression->evaluate(context, scope, exception);
	if (!!exception) return;
	name_desc->set_initialize(scope, value);
}

//
//FunctionDeclaration
//

void ast::FunctionDeclaration::gc_enumerate_refs() {
	Declaration::gc_enumerate_refs();
	gc_ref(m_syn_parameters);
	gc_ref(m_syn_body);
	gc_ref(m_expression);
}

void ast::FunctionDeclaration::syn_parameters(const ast_ptr<FunctionFormalParameters>& parameters) {
	m_syn_parameters = parameters;
}

void ast::FunctionDeclaration::syn_body(const ast_ptr<FunctionBody>& body) {
	m_syn_body = body;
}

rt::DeclarationType ast::FunctionDeclaration::get_declaration_type() const {
	return rt::DeclarationType::FUNCTION;
}

ast::ast_ptr<ast::FunctionDeclaration> ast::FunctionDeclaration::get_function_opt() {
	return self(this);
}

ast::ModifierType ast::FunctionDeclaration::get_default_access() const {
	return ModifierType::PUBLIC;
}

const ast::ast_ref<ast::FunctionExpression>& ast::FunctionDeclaration::get_expression() const {
	return m_expression;
}

gc::Local<const rt::NameDescriptor> ast::FunctionDeclaration::bind_declare_0(
	rt::BindContext* context,
	rt::BindScope* scope)
{
	//A constructor has no name.
	return !!get_name() ? scope->declare_function(get_name(), self(this)) : nullptr;
}

void ast::FunctionDeclaration::bind_define(rt::BindContext* context, rt::BindScope* scope) {
	m_expression = gc::create<FunctionExpression>(m_syn_parameters.local(), m_syn_body.local());
	m_expression->bind(context, scope);
}

void ast::FunctionDeclaration::exec_define_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	const rt::NameDescriptor* name_desc)
{
	//Nothing.
}

//
//FunctionFormalParameters
//

void ast::FunctionFormalParameters::gc_enumerate_refs() {
	Node::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_parameters);
}

void ast::FunctionFormalParameters::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::FunctionFormalParameters::syn_parameters(const ast_ptr<const ast_node_list<AstName>>& parameters) {
	m_syn_parameters = parameters;
}

const gc::Ref<ss::TextPos>& ast::FunctionFormalParameters::get_pos() const {
	return m_syn_pos;
}

const ast::ast_ref<const ast::ast_node_list<ast::AstName>>& ast::FunctionFormalParameters::get_parameters() const {
	return m_syn_parameters;
}

//
//FunctionBody
//

void ast::FunctionBody::gc_enumerate_refs() {
	Node::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_block);
}

void ast::FunctionBody::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::FunctionBody::syn_block(const ast_ptr<Block>& block) {
	m_syn_block = block;
}

const gc::Ref<ss::TextPos>& ast::FunctionBody::get_pos() const {
	return m_syn_pos;
}

const ast::ast_ref<ast::Block>& ast::FunctionBody::get_block() {
	return m_syn_block;
}

//
//ClassDeclaration
//

void ast::ClassDeclaration::gc_enumerate_refs() {
	Declaration::gc_enumerate_refs();
	gc_ref(m_syn_body);
	gc_ref(m_expression);
}

void ast::ClassDeclaration::syn_body(const ast_ptr<ClassBody>& body) {
	m_syn_body = body;
}

const ast::ast_ref<ast::ClassExpression>& ast::ClassDeclaration::get_expression() const {
	return m_expression;
}

rt::DeclarationType ast::ClassDeclaration::get_declaration_type() const {
	return rt::DeclarationType::CLASS;
}

gc::Local<const rt::NameDescriptor> ast::ClassDeclaration::bind_declare_0(
	rt::BindContext* context,
	rt::BindScope* scope)
{
	return scope->declare_class(get_name(), self(this));
}

void ast::ClassDeclaration::bind_define(rt::BindContext* context, rt::BindScope* scope) {
	m_expression = gc::create<ClassExpression>(get_pos().local(), m_syn_body.local());
	m_expression->bind(context, scope);
}

void ast::ClassDeclaration::exec_define_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	const rt::NameDescriptor* name_desc)
{
	//Nothing.
}

//
//ClassBody
//

void ast::ClassBody::gc_enumerate_refs() {
	Node::gc_enumerate_refs();
	gc_ref(m_syn_members);
	gc_ref(m_constructor);
}

void ast::ClassBody::syn_members(const ast_ptr<const ast_node_list<ClassMemberDeclaration>>& members) {
	m_syn_members = members;
}

namespace {
	template<class T>
	ast::ast_ptr<ast::ast_node_list<T>> erase_one(
		const ast::ast_ref<const ast::ast_node_list<T>>& vec,
		std::size_t erase_index)
	{
		std::size_t len = vec->size();
		ast::ast_ptr<ast::ast_node_list<T>> res = gc::Array<T>::create(len - 1);
		std::size_t ofs = 0;
		for (std::size_t i = 0; i < len; ++i) {
			if (i != erase_index) (*res)[ofs++] = (*vec)[i];
		}
		return res;
	}
}

void ast::ClassBody::bind_constructor() {
	for (std::size_t i = 0, n = m_syn_members->size(); i < n; ++i) {
		const ast_ref<ClassMemberDeclaration>& d = (*m_syn_members)[i];
		ast_ptr<FunctionDeclaration> fn = d->get_declaration()->get_function_opt();
		if (!!fn && !fn->get_name()) {
			m_constructor = fn;
			m_syn_members = erase_one(m_syn_members, i);
			break;
		}
	}
}

const ast::ast_ref<const ast::ast_node_list<ast::ClassMemberDeclaration>>& ast::ClassBody::get_members() const {
	return m_syn_members;
}

const ast::ast_ref<ast::FunctionDeclaration>& ast::ClassBody::get_constructor() {
	return m_constructor;
}

//
//ClassMemberDeclaration
//

ast::ClassMemberDeclaration::ClassMemberDeclaration()
: m_syn_modifier(ModifierType::NONE),
m_private(true)
{}

void ast::ClassMemberDeclaration::gc_enumerate_refs() {
	Node::gc_enumerate_refs();
	gc_ref(m_syn_declaration);
}

void ast::ClassMemberDeclaration::syn_modifier(ModifierType modifier) {
	m_syn_modifier = modifier;
}

void ast::ClassMemberDeclaration::syn_declaration(const ast_ptr<Declaration>& declaration) {
	m_syn_declaration = declaration;
}

const ast::ast_ref<ast::Declaration>& ast::ClassMemberDeclaration::get_declaration() const {
	return m_syn_declaration;
}

bool ast::ClassMemberDeclaration::is_private() const {
	return m_private;
}

void ast::ClassMemberDeclaration::bind_declare(rt::BindContext* context, rt::BindScope* scope) {
	ModifierType modifier = m_syn_modifier;
	if (ModifierType::NONE == modifier) modifier = m_syn_declaration->get_default_access();
	m_private = ModifierType::PRIVATE == modifier;

	m_syn_declaration->bind_declare(context, scope);
}

void ast::ClassMemberDeclaration::bind_define(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_declaration->bind_define(context, scope);
}
