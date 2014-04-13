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

//AST: statements.

#include "api_basic.h"
#include "ast.h"
#include "ast_combined.h"
#include "gc.h"
#include "scope.h"
#include "stacktrace.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace ast = ss::ast;
namespace rt = ss::rt;
namespace gc = ss::gc;

//
//Statement
//

rt::StatementResult ast::Statement::execute(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	try {
		return execute_0(context, scope);
	} catch (const RuntimeError& e) {
		return rt::StatementResult::exception(ThrowStatement::create_exception_value(get_pos(), e));
	}
}

//
//DeclarationStatement
//

void ast::DeclarationStatement::gc_enumerate_refs() {
	Statement::gc_enumerate_refs();
	gc_ref(m_syn_declaration);
}

void ast::DeclarationStatement::syn_declaration(const ast_ptr<Declaration>& declaration) {
	m_syn_declaration = declaration;
}

gc::Local<ss::TextPos> ast::DeclarationStatement::get_pos() const {
	return m_syn_declaration->get_pos();
}

ast::ast_ptr<ast::Declaration> ast::DeclarationStatement::get_declaration() const {
	return m_syn_declaration;
}

void ast::DeclarationStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_declaration->bind_declare(context, scope);
	m_syn_declaration->bind_define(context, scope);
}

rt::StatementResult ast::DeclarationStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::Value> exception;
	m_syn_declaration->exec_define(context, scope, exception);
	if (!!exception) return rt::StatementResult::exception(exception);

	return rt::StatementResult::none();
}

//
//ExecutionStatement
//

ast::ast_ptr<ast::Declaration> ast::ExecutionStatement::get_declaration() const {
	return ast::ast_ptr<ast::Declaration>();
}

//
//EmptyStatement
//

void ast::EmptyStatement::gc_enumerate_refs() {
	ExecutionStatement::gc_enumerate_refs();
	gc_ref(m_syn_pos);
}

void ast::EmptyStatement::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

gc::Local<ss::TextPos> ast::EmptyStatement::get_pos() const {
	return m_syn_pos;
}

void ast::EmptyStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	//Nothing.
}

rt::StatementResult ast::EmptyStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	return rt::StatementResult::none();
}

//
//ExpressionStatement
//

void ast::ExpressionStatement::gc_enumerate_refs() {
	ExecutionStatement::gc_enumerate_refs();
	gc_ref(m_syn_expression);
}

void ast::ExpressionStatement::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

gc::Local<ss::TextPos> ast::ExpressionStatement::get_pos() const {
	return m_syn_expression->get_start_pos();
}

void ast::ExpressionStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_expression->bind(context, scope);
}

rt::StatementResult ast::ExpressionStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::Value> exception;
	m_syn_expression->evaluate(context, scope, exception);
	if (!!exception) return rt::StatementResult::exception(exception);
	return rt::StatementResult::none();
}

//
//IfStatement
//

void ast::IfStatement::gc_enumerate_refs() {
	ExecutionStatement::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_expression);
	gc_ref(m_syn_true_statement);
	gc_ref(m_syn_false_statement);
}

void ast::IfStatement::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::IfStatement::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

void ast::IfStatement::syn_true_statement(const ast_ptr<Statement>& true_statement) {
	m_syn_true_statement = true_statement;
}

void ast::IfStatement::syn_false_statement(const ast_ptr<Statement>& false_statement) {
	m_syn_false_statement = false_statement;
}

gc::Local<ss::TextPos> ast::IfStatement::get_pos() const {
	return m_syn_pos;
}

void ast::IfStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_expression->bind(context, scope);
	m_syn_true_statement->bind(context, scope);
	if (!!m_syn_false_statement) m_syn_false_statement->bind(context, scope);
}

rt::StatementResult ast::IfStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::Value> exception;
	gc::Local<rt::Value> value = m_syn_expression->evaluate(context, scope, exception);
	if (!!exception) return rt::StatementResult::exception(exception);

	if (value->get_boolean()) {
		return m_syn_true_statement->execute(context, scope);
	} else if (!!m_syn_false_statement) {
		return m_syn_false_statement->execute(context, scope);
	} else {
		return rt::StatementResult::none();
	}
}

//
//LoopStatement
//

void ast::LoopStatement::gc_enumerate_refs() {
	ExecutionStatement::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_expression);
	gc_ref(m_syn_statement);
	gc_ref(m_scope_descriptor);
}

void ast::LoopStatement::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::LoopStatement::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

void ast::LoopStatement::syn_statement(const ast_ptr<Statement>& statement) {
	m_syn_statement = statement;
}

gc::Local<ss::TextPos> ast::LoopStatement::get_pos() const {
	return m_syn_pos;
}

const ast::ast_ref<ast::Expression>& ast::LoopStatement::get_expression() const {
	return m_syn_expression;
}

const ast::ast_ref<ast::Statement>& ast::LoopStatement::get_statement() const {
	return m_syn_statement;
}

void ast::LoopStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	std::unique_ptr<rt::BindScope> sub_scope = scope->create_nested_block(true);

	bind_loop(context, sub_scope.get());
	if (!!m_syn_expression) m_syn_expression->bind(context, sub_scope.get());
	m_syn_statement->bind(context, sub_scope.get());

	m_scope_descriptor = sub_scope->create_scope_descriptor();
}

rt::StatementResult ast::LoopStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::ExecScope> sub_scope = scope->create_nested_scope(m_scope_descriptor, gc::Local<rt::Value>());
	return exec_loop(context, sub_scope);
}

void ast::LoopStatement::bind_loop(rt::BindContext* context, rt::BindScope* scope) {
	//Nothing.
}

//
//RegularLoopStatement
//

rt::StatementResult ast::RegularLoopStatement::exec_loop(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::Value> exception;

	exec_loop_init(context, scope, exception);
	if (!!exception) return rt::StatementResult::exception(exception);

	for (;;) {
		if (!!get_expression()) {
			gc::Local<rt::Value> value = get_expression()->evaluate(context, scope, exception);
			if (!!exception) return rt::StatementResult::exception(exception);
			if (!value->get_boolean()) break;
		}

		rt::StatementResult result = get_statement()->execute(context, scope);
		rt::StatementResultType result_type = result.get_type();
		if (rt::StatementResultType::BREAK == result_type) {
			break;
		} else if (rt::StatementResultType::NONE != result_type
			&& rt::StatementResultType::CONTINUE != result_type)
		{
			return result;
		}

		exec_loop_update(context, scope, exception);
		if (!!exception) return rt::StatementResult::exception(exception);
	}

	return rt::StatementResult::none();
}

void ast::RegularLoopStatement::exec_loop_init(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	//Nothing.
}

void ast::RegularLoopStatement::exec_loop_update(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	//Nothing.
}

//
//RegularForStatement
//

void ast::RegularForStatement::gc_enumerate_refs() {
	RegularLoopStatement::gc_enumerate_refs();
	gc_ref(m_syn_init);
	gc_ref(m_syn_update);
}

void ast::RegularForStatement::syn_init(const ast_ptr<ForInit>& init) {
	m_syn_init = init;
}

void ast::RegularForStatement::syn_update(const ast_ptr<const ast_node_list<Expression>>& update) {
	m_syn_update = update;
}

void ast::RegularForStatement::bind_loop(rt::BindContext* context, rt::BindScope* scope) {
	if (!!m_syn_init) m_syn_init->bind(context, scope);
	for (const ast_ref<Expression>& expr : *m_syn_update) expr->bind(context, scope);
}

void ast::RegularForStatement::exec_loop_init(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	if (!!m_syn_init) m_syn_init->execute(context, scope, exception);
}

void ast::RegularForStatement::exec_loop_update(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	for (const ast_ref<Expression>& expr : *m_syn_update) {
		expr->evaluate(context, scope, exception);
		if (!!exception) return;
	}
}

//
//ForEachValueIterator
//

class ast::ForEachValueIterator : public rt::InternalValueIterator {
public:
	ast::ForEachStatement* const m_stmt;
	const gc::Local<rt::ExecContext>& m_context;
	const gc::Local<rt::ExecScope>& m_scope;
	rt::StatementResult m_result;

	ForEachValueIterator(
		ast::ForEachStatement* stmt,
		const gc::Local<rt::ExecContext>& context,
		const gc::Local<rt::ExecScope>& scope)
		: m_stmt(stmt),
		m_context(context),
		m_scope(scope),
		m_result(rt::StatementResultType::NONE)
	{}

	bool iterate(const gc::Local<rt::Value>& value) override {
		m_stmt->m_name_descriptor->set_modify(m_scope, value);
		m_result = m_stmt->get_statement()->execute(m_context, m_scope);
		return rt::StatementResultType::NONE == m_result.get_type()
			|| rt::StatementResultType::CONTINUE == m_result.get_type();
	}
};

//
//ForEachStatement
//

void ast::ForEachStatement::gc_enumerate_refs() {
	LoopStatement::gc_enumerate_refs();
	gc_ref(m_syn_variable);
	gc_ref(m_name_descriptor);
}

void ast::ForEachStatement::syn_new_variable(bool new_variable) {
	m_syn_new_variable = new_variable;
}

void ast::ForEachStatement::syn_variable(const SynName& variable) {
	m_syn_variable = variable;
}

void ast::ForEachStatement::bind_loop(rt::BindContext* context, rt::BindScope* scope) {
	if (m_syn_new_variable) {
		m_name_descriptor = scope->declare_variable(m_syn_variable, false);
	} else {
		m_name_descriptor = scope->lookup(m_syn_variable);
	}
}

rt::StatementResult ast::ForEachStatement::exec_loop(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::Value> exception;
	gc::Local<rt::Value> value = get_expression()->evaluate(context, scope, exception);
	if (!!exception) return rt::StatementResult::exception(exception);
	assert(!!value);

	ForEachValueIterator iterator(this, context, scope);
	value->iterate(iterator);

	rt::StatementResultType result_type = iterator.m_result.get_type();
	if (rt::StatementResultType::THROW == result_type
		|| rt::StatementResultType::RETURN == result_type)
	{
		return iterator.m_result;
	}

	return rt::StatementResult::none();
}

void ast::VariableForInit::gc_enumerate_refs() {
	ForInit::gc_enumerate_refs();
	gc_ref(m_syn_variables);
}

void ast::VariableForInit::syn_variables(const ast_ptr<const ast_node_list<ForVariableDeclaration>>& variables) {
	m_syn_variables = variables;
}

void ast::VariableForInit::bind(rt::BindContext* context, rt::BindScope* scope) {
	for (const ast_ref<ForVariableDeclaration>& var : *m_syn_variables) var->bind(context, scope);
}

void ast::VariableForInit::execute(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	for (const ast_ref<ForVariableDeclaration>& var : *m_syn_variables) {
		var->execute(context, scope, exception);
		if (!!exception) return;
	}
}

//
//ForVariableDeclaration
//

void ast::ForVariableDeclaration::gc_enumerate_refs() {
	Node::gc_enumerate_refs();
	gc_ref(m_syn_name);
	gc_ref(m_syn_expression);
	gc_ref(m_name_descriptor);
}

void ast::ForVariableDeclaration::syn_name(const SynName& name) {
	m_syn_name = name;
}

void ast::ForVariableDeclaration::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

void ast::ForVariableDeclaration::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_name_descriptor = scope->declare_variable(m_syn_name, false);
	m_syn_expression->bind(context, scope);
}

void ast::ForVariableDeclaration::execute(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> value = m_syn_expression->evaluate(context, scope, exception);
	if (!!exception) return;
	m_name_descriptor->set_initialize(scope, value);
}

//
//ExpressionForInit
//

void ast::ExpressionForInit::gc_enumerate_refs() {
	ForInit::gc_enumerate_refs();
	gc_ref(m_syn_expressions);
}

void ast::ExpressionForInit::syn_expressions(const ast_ptr<const ast_node_list<Expression>>& expressions) {
	m_syn_expressions = expressions;
}

void ast::ExpressionForInit::bind(rt::BindContext* context, rt::BindScope* scope) {
	for (const ast_ref<Expression>& expr : *m_syn_expressions) expr->bind(context, scope);
}

void ast::ExpressionForInit::execute(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	for (const ast_ref<Expression>& expr : *m_syn_expressions) {
		expr->evaluate(context, scope, exception);
		if (!!exception) return;
	}
}

//
//BlockStatement
//

void ast::BlockStatement::gc_enumerate_refs() {
	ExecutionStatement::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_block);
	gc_ref(m_scope_descriptor);
}

void ast::BlockStatement::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::BlockStatement::syn_block(const ast_ptr<Block>& block) {
	m_syn_block = block;
}

gc::Local<ss::TextPos> ast::BlockStatement::get_pos() const {
	return m_syn_pos;
}

void ast::BlockStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	std::unique_ptr<rt::BindScope> sub_scope = scope->create_nested_block(false);
	m_syn_block->bind(context, sub_scope.get());
	m_scope_descriptor = sub_scope->create_scope_descriptor();
}

rt::StatementResult ast::BlockStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::ExecScope> sub_scope = scope->create_nested_scope(m_scope_descriptor, nullptr);
	return m_syn_block->execute(context, sub_scope);
}

//
//TryStatement
//

void ast::TryStatement::gc_enumerate_refs() {
	ExecutionStatement::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_try_statement);
	gc_ref(m_syn_catch_variable);
	gc_ref(m_syn_catch_statement);
	gc_ref(m_syn_finally_statement);
	gc_ref(m_catch_scope_descriptor);
	gc_ref(m_catch_name_descriptor);
}

void ast::TryStatement::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::TryStatement::syn_try_statement(const ast_ptr<Statement>& try_statement) {
	m_syn_try_statement = try_statement;
}

void ast::TryStatement::syn_catch_variable(const SynName& catch_variable) {
	m_syn_catch_variable = catch_variable;
}

void ast::TryStatement::syn_catch_statement(const ast_ptr<Statement>& catch_statement) {
	m_syn_catch_statement = catch_statement;
}

void ast::TryStatement::syn_finally_statement(const ast_ptr<Statement>& finally_statement) {
	m_syn_finally_statement = finally_statement;
}

gc::Local<ss::TextPos> ast::TryStatement::get_pos() const {
	return m_syn_pos;
}

void ast::TryStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_try_statement->bind(context, scope);
	if (!!m_syn_catch_statement) {
		std::unique_ptr<rt::BindScope> sub_scope = scope->create_nested_block(false);
		m_catch_name_descriptor = sub_scope->declare_variable(m_syn_catch_variable, false);
		m_syn_catch_statement->bind(context, sub_scope.get());
		m_catch_scope_descriptor = sub_scope->create_scope_descriptor();
	}
	if (!!m_syn_finally_statement) m_syn_finally_statement->bind(context, scope);
}

rt::StatementResult ast::TryStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	rt::StatementResult result = m_syn_try_statement->execute(context, scope);

	if (rt::StatementResultType::THROW == result.get_type() && !!m_syn_catch_statement) {
		gc::Local<rt::ExecScope> sub_scope =
			scope->create_nested_scope(m_catch_scope_descriptor, gc::Local<rt::Value>());
		m_catch_name_descriptor->set_initialize(sub_scope, result.get_value());
		result = m_syn_catch_statement->execute(context, sub_scope);
	}

	if (!!m_syn_finally_statement) {
		rt::StatementResult finally_result = m_syn_finally_statement->execute(context, scope);
		if (rt::StatementResultType::NONE != finally_result.get_type()) {
			result = finally_result;
		}
	}

	return result;
}

//
//ControlStatement
//

void ast::ControlStatement::gc_enumerate_refs() {
	ExecutionStatement::gc_enumerate_refs();
	gc_ref(m_syn_pos);
}

void ast::ControlStatement::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

gc::Local<ss::TextPos> ast::ControlStatement::get_pos() const {
	return m_syn_pos;
}

//
//ContinueStatement
//

void ast::ContinueStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	if (!scope->is_loop_control_statement_allowed()) {
		throw CompilationError(get_pos(), "Not in a loop");
	}
}

rt::StatementResult ast::ContinueStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	return rt::StatementResult(rt::StatementResultType::CONTINUE);
}

//
//BreakStatement
//

void ast::BreakStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	if (!scope->is_loop_control_statement_allowed()) {
		throw CompilationError(get_pos(), "Not in a loop");
	}
}

rt::StatementResult ast::BreakStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	return rt::StatementResult(rt::StatementResultType::BREAK);
}

//
//ReturnStatement
//

void ast::ReturnStatement::gc_enumerate_refs() {
	ControlStatement::gc_enumerate_refs();
	gc_ref(m_syn_return_value);
}

void ast::ReturnStatement::syn_return_value(const ast_ptr<Expression>& return_value) {
	m_syn_return_value = return_value;
}

void ast::ReturnStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	if (!!m_syn_return_value) m_syn_return_value->bind(context, scope);
}

rt::StatementResult ast::ReturnStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::Value> result;
	if (!!m_syn_return_value) {
		gc::Local<rt::Value> exception;
		result = m_syn_return_value->evaluate(context, scope, exception);
		if (!!exception) return rt::StatementResult::exception(exception);
	} else {
		result = context->get_value_factory()->get_void_value();
	}

	return rt::StatementResult(rt::StatementResultType::RETURN, result);
}

//
//ThrowStatement
//

void ast::ThrowStatement::gc_enumerate_refs() {
	ControlStatement::gc_enumerate_refs();
	gc_ref(m_syn_expression);
}

void ast::ThrowStatement::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

void ast::ThrowStatement::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_expression->bind(context, scope);
}

gc::Local<rt::Value> ast::ThrowStatement::create_exception_value(
	const gc::Local<ss::TextPos>& text_pos,
	const ss::RuntimeError& e)
{
	StringLoc str = gc::create<String>(e.get_msg());
	gc::Local<rt::Value> value = gc::create<rt::StringValue>(str);
	const gc::Local<TextPos>& actual_pos = !!e.get_pos() ? e.get_pos() : text_pos;
	return create_exception_value(actual_pos, value);
}

gc::Local<rt::Value> ast::ThrowStatement::create_exception_value(
	const gc::Local<ss::TextPos>& text_pos,
	const gc::Local<rt::Value>& value)
{
	return gc::create<rt::ExceptionValue>(value, rt::StackTraceMark::get_stack_trace(text_pos));
}

rt::StatementResult ast::ThrowStatement::execute_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::Value> exception;
	gc::Local<rt::Value> value = m_syn_expression->evaluate(context, scope, exception);
	if (!exception && !dynamic_cast<rt::ExceptionValue*>(value.get())) {
		value = create_exception_value(get_pos(), value);
	}
	return rt::StatementResult::exception(value);
}
