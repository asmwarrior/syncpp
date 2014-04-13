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

//AST: expressions.

#include <cassert>

#include "api_basic.h"
#include "ast.h"
#include "ast_combined.h"
#include "gc.h"
#include "name.h"
#include "scope.h"
#include "stacktrace.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace ast = ss::ast;
namespace rt = ss::rt;
namespace gc = ss::gc;

namespace {

	gc::Local<gc::Array<rt::Value>> evaluate_arguments(
		const gc::Local<rt::ExecContext>& context,
		const gc::Local<rt::ExecScope>& scope,
		const ast::ast_ref<const ast::ast_node_list<ast::Expression>>& syn_arguments,
		gc::Local<rt::Value>& exception)
	{
		std::size_t arg_cnt = syn_arguments->size();

		gc::Local<gc::Array<rt::Value>> arguments = gc::Array<rt::Value>::create(arg_cnt);
		for (std::size_t i = 0; i < arg_cnt; ++i) {
			const ast::ast_ref<ast::Expression>& syn_arg = (*syn_arguments)[i];
			gc::Local<rt::Value> arg = syn_arg->evaluate(context, scope, exception);
			if (!!exception) return arguments;
			arguments->get(i) = arg;
		}

		return arguments;
	}

}

//
//Expression
//

gc::Local<ss::TextPos> ast::Expression::get_start_pos() const {
	return get_pos();
}

bool ast::Expression::is_assignment_allowed() const {
	return false;
}

bool ast::Expression::is_invocation_allowed() const {
	return false;
}

bool ast::Expression::is_instantiation_allowed() const {
	return false;
}

gc::Local<rt::Value> ast::Expression::evaluate(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	try {
		gc::Local<rt::Value> result = evaluate_0(context, scope, exception);
		assert(!!exception || !!result);
		return result;
	} catch (const RuntimeError& e) {
		exception = ThrowStatement::create_exception_value(get_pos(), e);
		return gc::Local<rt::Value>();
	}
}

gc::Local<rt::Value> ast::Expression::modify(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	rt::ValueModifier& modifier)
{
	try {
		gc::Local<rt::Value> result = modify_0(context, scope, exception, modifier);
		assert(!!result);
		return result;
	} catch (const RuntimeError& e) {
		exception = ThrowStatement::create_exception_value(get_pos(), e);
		return gc::Local<rt::Value>();
	}
}

gc::Local<rt::Value> ast::Expression::modify_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	rt::ValueModifier& modifier)
{
	throw RuntimeError("Not an lvalue");
}

//
//BinaryExpression
//

void ast::BinaryExpression::gc_enumerate_refs() {
	Expression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_left);
	gc_ref(m_syn_right);
}

void ast::BinaryExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::BinaryExpression::syn_op(const AstBinOp& op) {
	m_syn_op = op;
}

void ast::BinaryExpression::syn_left(const ast_ptr<Expression>& left) {
	m_syn_left = left;
}

void ast::BinaryExpression::syn_right(const ast_ptr<Expression>& right) {
	m_syn_right = right;
}

const rt::BinaryOperator* ast::BinaryExpression::get_op() const {
	return m_op;
}

const ast::ast_ref<ast::Expression>& ast::BinaryExpression::get_left() const {
	return m_syn_left;
}

const ast::ast_ref<ast::Expression>& ast::BinaryExpression::get_right() const {
	return m_syn_right;
}

gc::Local<ss::TextPos> ast::BinaryExpression::get_pos() const {
	return m_syn_pos;
}

gc::Local<ss::TextPos> ast::BinaryExpression::get_start_pos() const {
	return m_syn_left->get_start_pos();
}

void ast::BinaryExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_left->bind(context, scope);
	m_syn_right->bind(context, scope);

	switch (m_syn_op) {
	case AstBinOp::NONE:
		m_op = nullptr;
		break;
	case AstBinOp::ADD:
		m_op = &rt::AddBinaryOperator::Instance;
		break;
	case AstBinOp::SUB:
		m_op = &rt::SubBinaryOperator::Instance;
		break;
	case AstBinOp::MUL:
		m_op = &rt::MulBinaryOperator::Instance;
		break;
	case AstBinOp::DIV:
		m_op = &rt::DivBinaryOperator::Instance;
		break;
	case AstBinOp::MOD:
		m_op = &rt::ModBinaryOperator::Instance;
		break;
	case AstBinOp::LAND:
		m_op = &rt::LogicalAndBinaryOperator::Instance;
		break;
	case AstBinOp::LOR:
		m_op = &rt::LogicalOrBinaryOperator::Instance;
		break;
	case AstBinOp::EQ:
		m_op = &rt::EqBinaryOperator::Instance;
		break;
	case AstBinOp::NE:
		m_op = &rt::NeBinaryOperator::Instance;
		break;
	case AstBinOp::LT:
		m_op = &rt::LtBinaryOperator::Instance;
		break;
	case AstBinOp::GT:
		m_op = &rt::GtBinaryOperator::Instance;
		break;
	case AstBinOp::LE:
		m_op = &rt::LeBinaryOperator::Instance;
		break;
	case AstBinOp::GE:
		m_op = &rt::GeBinaryOperator::Instance;
		break;
	default:
		throw SystemError(m_syn_pos, "Invalid binary operator");
	}
}

//
//AssignmentExpression::Modifier
//

class ast::AssignmentExpression::Modifier : public rt::ValueModifier {
	const gc::Local<rt::ExecContext> m_context;
	const rt::BinaryOperator* m_op;
	const gc::Local<rt::Value> m_right_value;

public:
	Modifier(
		const gc::Local<rt::ExecContext>& context,
		const rt::BinaryOperator* op,
		const gc::Local<rt::Value>& right_value)
		: m_context(context),
		m_op(op),
		m_right_value(right_value)
	{}

	gc::Local<rt::Value> modify_short(gc::Local<rt::Value>& result) override {
		if (m_op) return nullptr;
		result = m_right_value;
		return m_right_value;
	}

	gc::Local<rt::Value> modify(const gc::Local<rt::Value>& value, gc::Local<rt::Value>& result) override {
		if (m_op) {
			result = m_op->evaluate(m_context, value, m_right_value);
		} else {
			result = m_right_value;
		}
		assert(!!result);
		return result;
	}
};

//
//AssignmentExpression
//

bool ast::AssignmentExpression::is_invocation_allowed() const {
	return get_right()->is_invocation_allowed();
}

bool ast::AssignmentExpression::is_instantiation_allowed() const {
	return get_right()->is_instantiation_allowed();
}

void ast::AssignmentExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	BinaryExpression::bind(context, scope);
	if (!get_left()->is_assignment_allowed()) {
		throw CompilationError(get_pos(), std::string("Left operand is not an lvalue"));
	}
}

gc::Local<rt::Value> ast::AssignmentExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> right = get_right()->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	if (right->is_undefined()) throw RuntimeError(get_pos(), "The value is undefined");
	if (right->is_void()) throw RuntimeError(get_pos(), "Cannot assign a void value");

	Modifier modifier(context, get_op(), right);
	gc::Local<rt::Value> result = get_left()->modify(context, scope, exception, modifier);
	assert(!!exception || !!result);
	return result;
}

//
//ConditionalExpression
//

void ast::ConditionalExpression::gc_enumerate_refs() {
	Expression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_condition);
	gc_ref(m_syn_true_expression);
	gc_ref(m_syn_false_expression);
}

void ast::ConditionalExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::ConditionalExpression::syn_condition(const ast_ptr<Expression>& condition) {
	m_syn_condition = condition;
}

void ast::ConditionalExpression::syn_true_expression(const ast_ptr<Expression>& true_expression) {
	m_syn_true_expression = true_expression;
}

void ast::ConditionalExpression::syn_false_expression(const ast_ptr<Expression>& false_expression) {
	m_syn_false_expression = false_expression;
}

gc::Local<ss::TextPos> ast::ConditionalExpression::get_pos() const {
	return m_syn_pos;
}

gc::Local<ss::TextPos> ast::ConditionalExpression::get_start_pos() const {
	return m_syn_condition->get_start_pos();
}

bool ast::ConditionalExpression::is_invocation_allowed() const {
	return m_syn_true_expression->is_invocation_allowed()
		&& m_syn_false_expression->is_invocation_allowed();
}

bool ast::ConditionalExpression::is_instantiation_allowed() const {
	return m_syn_true_expression->is_instantiation_allowed()
		&& m_syn_false_expression->is_instantiation_allowed();
}

void ast::ConditionalExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_condition->bind(context, scope);
	m_syn_true_expression->bind(context, scope);
	m_syn_false_expression->bind(context, scope);
}

gc::Local<rt::Value> ast::ConditionalExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> condition = m_syn_condition->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	const ast_ref<Expression>& expression = condition->get_boolean()
		? m_syn_true_expression : m_syn_false_expression;
	return expression->evaluate(context, scope, exception);
}

//
//RegularBinaryExpression
//

void ast::RegularBinaryExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	BinaryExpression::bind(context, scope);
	assert(get_op());
}

gc::Local<rt::Value> ast::RegularBinaryExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	const rt::BinaryOperator* op = get_op();

	gc::Local<rt::Value> a = get_left()->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	gc::Local<rt::Value> result = op->evaluate_short(context, a);
	if (!result) {
		gc::Local<rt::Value> b = get_right()->evaluate(context, scope, exception);
		if (!!exception) return context->get_undefined_value();

		result = op->evaluate(context, a, b);
	}

	return result;
}

//
//UnaryExpression
//

void ast::UnaryExpression::gc_enumerate_refs() {
	Expression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_expression);
}

void ast::UnaryExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::UnaryExpression::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

gc::Local<ss::TextPos> ast::UnaryExpression::get_pos() const {
	return m_syn_pos;
}

const ast::ast_ref<ast::Expression>& ast::UnaryExpression::get_expression() const {
	return m_syn_expression;
}

void ast::UnaryExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_expression->bind(context, scope);
}

//
//RegularUnaryExpression
//

void ast::RegularUnaryExpression::syn_op(const AstUnOp& op) {
	m_syn_op = op;
}

void ast::RegularUnaryExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	UnaryExpression::bind(context, scope);

	switch (m_syn_op) {
	case AstUnOp::PLUS:
		m_op = &rt::PlusUnaryOperator::Instance;
		break;
	case AstUnOp::MINUS:
		m_op = &rt::MinusUnaryOperator::Instance;
		break;
	case AstUnOp::LNOT:
		m_op = &rt::LogicalNotUnaryOperator::Instance;
		break;
	default:
		throw SystemError(get_pos(), "Invalid unary operator");
	}
}

gc::Local<rt::Value> ast::RegularUnaryExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> a = get_expression()->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	return m_op->evaluate(context, a);
}

//
//IncrementDecrementExpression::Modifier
//

class ast::IncrementDecrementExpression::Modifier : public rt::ValueModifier {
	const gc::Local<rt::ExecContext> m_context;
	const IncrementDecrementExpression* const m_expression;

public:
	Modifier(const gc::Local<rt::ExecContext>& context, const IncrementDecrementExpression* expression)
		: m_context(context),
		m_expression(expression)
	{}

	gc::Local<rt::Value> modify(const gc::Local<rt::Value>& value, gc::Local<rt::Value>& result) override {
		if (value->is_undefined()) {
			throw RuntimeError(m_expression->get_pos(), "The value of the operand is undefined");
		}
		if (value->is_void()) {
			throw RuntimeError(m_expression->get_pos(), "The value of the operand is void");
		}

		gc::Local<rt::Value> new_value;
		rt::OperandType type = value->get_operand_type();
		if (rt::OperandType::INTEGER == type) {
			ScriptIntegerType x = value->get_integer();
			m_expression->m_syn_increment ? ++x : --x;
			new_value = m_context->get_value_factory()->get_integer_value(x);
		} else if (rt::OperandType::FLOAT == type) {
			ScriptFloatType x = value->get_float();
			m_expression->m_syn_increment ? ++x : --x;
			new_value = m_context->get_value_factory()->get_float_value(x);
		}

		result = m_expression->m_syn_postfix ? value : new_value;
		return new_value;
	}
};

//
//IncrementDecrementExpression
//

void ast::IncrementDecrementExpression::syn_increment(bool increment) {
	m_syn_increment = increment;
}

void ast::IncrementDecrementExpression::syn_postfix(bool postfix) {
	m_syn_postfix = postfix;
}

gc::Local<ss::TextPos> ast::IncrementDecrementExpression::get_start_pos() const {
	return m_syn_postfix ? get_expression()->get_start_pos() : get_pos();
}

gc::Local<rt::Value> ast::IncrementDecrementExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	Modifier modifier(context, this);
	return get_expression()->modify(context, scope, exception, modifier);
}

//
//MemberExpression
//

void ast::MemberExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_object);
	gc_ref(m_syn_name);
}

void ast::MemberExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::MemberExpression::syn_object(const ast_ptr<Expression>& object) {
	m_syn_object = object;
}

void ast::MemberExpression::syn_name(const SynName& name) {
	m_syn_name = name;
}

gc::Local<ss::TextPos> ast::MemberExpression::get_pos() const {
	return m_syn_pos;
}

gc::Local<ss::TextPos> ast::MemberExpression::get_start_pos() const {
	return m_syn_object->get_start_pos();
}

bool ast::MemberExpression::is_assignment_allowed() const {
	return true;
}

bool ast::MemberExpression::is_invocation_allowed() const {
	return true;
}

bool ast::MemberExpression::is_instantiation_allowed() const {
	return true;
}

void ast::MemberExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_object->bind(context, scope);
}

gc::Local<rt::Value> ast::MemberExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> object = m_syn_object->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	return object->get_member(context, scope, m_syn_name->get_info());
}

gc::Local<rt::Value> ast::MemberExpression::modify_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	rt::ValueModifier& modifier)
{
	gc::Local<rt::Value> object = m_syn_object->evaluate(context, scope, exception);
	if (!!exception) return exception;

	gc::Local<rt::Value> result;
	gc::Local<rt::Value> new_value = modifier.modify_short(result);
	if (!new_value) {
		//TODO Do not lookup the member by name twice here.
		gc::Local<rt::Value> old_value = object->get_member(context, scope, m_syn_name->get_info());
		new_value = modifier.modify(old_value, result);
		assert(!!new_value);
	}

	object->set_member(context, scope, m_syn_name->get_info(), new_value);
	return result;
}

//
//InvocationExpression
//

void ast::InvocationExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_function);
	gc_ref(m_syn_arguments);
}

void ast::InvocationExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::InvocationExpression::syn_function(const ast_ptr<Expression>& function) {
	m_syn_function = function;
}

void ast::InvocationExpression::syn_arguments(const ast_ptr<const ast_node_list<Expression>>& arguments) {
	m_syn_arguments = arguments;
}

gc::Local<ss::TextPos> ast::InvocationExpression::get_pos() const {
	return m_syn_pos;
}

gc::Local<ss::TextPos> ast::InvocationExpression::get_start_pos() const {
	return m_syn_function->get_start_pos();
}

bool ast::InvocationExpression::is_invocation_allowed() const {
	return true;
}

bool ast::InvocationExpression::is_instantiation_allowed() const {
	return true;
}

void ast::InvocationExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_function->bind(context, scope);
	for (const ast_ref<Expression>& expr : *m_syn_arguments) {
		expr->bind(context, scope);
	}
	if (!m_syn_function->is_invocation_allowed()) {
		throw CompilationError(m_syn_pos, std::string("Not a function"));
	}
}

gc::Local<rt::Value> ast::InvocationExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> function = m_syn_function->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	gc::Local<gc::Array<rt::Value>> arguments = evaluate_arguments(context, scope, m_syn_arguments, exception);
	if (!!exception) return context->get_undefined_value();

	rt::StackTraceMark stack_trace(m_syn_pos);
	gc::Local<rt::Value> value = function->invoke(context, arguments, exception);
	if (!!exception) return context->get_undefined_value();

	return value;
}

//
//NewObjectExpression
//

void ast::NewObjectExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_type_expr);
	gc_ref(m_syn_arguments);
}

void ast::NewObjectExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::NewObjectExpression::syn_type_expr(const ast_ptr<Expression>& type_expr) {
	m_syn_type_expr = type_expr;
}

void ast::NewObjectExpression::syn_arguments(const ast_ptr<const ast_node_list<Expression>>& arguments) {
	m_syn_arguments = arguments;
}

gc::Local<ss::TextPos> ast::NewObjectExpression::get_pos() const {
	return m_syn_pos;
}

void ast::NewObjectExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_type_expr->bind(context, scope);
	for (const ast_ref<Expression>& expr : *m_syn_arguments) {
		expr->bind(context, scope);
	}
	if (!m_syn_type_expr->is_instantiation_allowed()) {
		throw CompilationError(m_syn_pos, std::string("Not a type"));
	}
}

gc::Local<rt::Value> ast::NewObjectExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> type = m_syn_type_expr->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	gc::Local<gc::Array<rt::Value>> arguments = evaluate_arguments(context, scope, m_syn_arguments, exception);
	if (!!exception) return context->get_undefined_value();

	rt::StackTraceMark stack_trace(m_syn_pos);
	gc::Local<rt::Value> value = type->instantiate(context, arguments, exception);
	if (!!exception) return context->get_undefined_value();

	return value;
}

//
//NewArrayExpression
//

void ast::NewArrayExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_length);
}

void ast::NewArrayExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::NewArrayExpression::syn_length(const ast_ptr<Expression>& length) {
	m_syn_length = length;
}

gc::Local<ss::TextPos> ast::NewArrayExpression::get_pos() const {
	return m_syn_pos;
}

void ast::NewArrayExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_length->bind(context, scope);
}

gc::Local<rt::Value> ast::NewArrayExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> length = m_syn_length->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	ScriptIntegerType len = length->get_integer();
	const std::size_t max_len = std::numeric_limits<std::size_t>::max();
	if (len < 0 || len > max_len) throw RuntimeError(m_syn_pos, "Array length out of range");
	
	const std::size_t array_len = scriptint_to_size(len);
	gc::Local<gc::Array<rt::Value>> array = gc::Array<rt::Value>::create(array_len);

	gc::Local<rt::Value> nullval = context->get_value_factory()->get_null_value();
	for (std::size_t i = 0; i < len; ++i) (*array)[i] = nullval;

	return gc::create<rt::ArrayValue>(array);
}

//
//ArrayExpression
//

void ast::ArrayExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_expressions);
}

void ast::ArrayExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::ArrayExpression::syn_expressions(const ast_ptr<const ast_node_list<Expression>>& expressions) {
	m_syn_expressions = expressions;
}

gc::Local<ss::TextPos> ast::ArrayExpression::get_pos() const {
	return m_syn_pos;
}

void ast::ArrayExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	for (const ast_ref<Expression>& expr : *m_syn_expressions) {
		expr->bind(context, scope);
	}
}

gc::Local<rt::Value> ast::ArrayExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	std::size_t len = m_syn_expressions->size();
	gc::Local<gc::Array<rt::Value>> array = gc::Array<rt::Value>::create(len);

	for (std::size_t i = 0; i < len; ++i) {
		gc::Local<rt::Value> value = (*m_syn_expressions)[i]->evaluate(context, scope, exception);
		if (!!exception) return context->get_undefined_value();
		array->get(i) = value;
	}

	gc::Local<rt::ArrayValue> array_value = gc::create<rt::ArrayValue>(array);
	return array_value;
}

//
//SubscriptExpression
//

void ast::SubscriptExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_array);
	gc_ref(m_syn_index);
}

void ast::SubscriptExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::SubscriptExpression::syn_array(const ast_ptr<Expression>& array) {
	m_syn_array = array;
}

void ast::SubscriptExpression::syn_index(const ast_ptr<Expression>& index) {
	m_syn_index = index;
}

gc::Local<ss::TextPos> ast::SubscriptExpression::get_pos() const {
	return m_syn_pos;
}

gc::Local<ss::TextPos> ast::SubscriptExpression::get_start_pos() const {
	return m_syn_array->get_start_pos();
}

bool ast::SubscriptExpression::is_assignment_allowed() const {
	return true;
}

bool ast::SubscriptExpression::is_invocation_allowed() const {
	return true;
}

bool ast::SubscriptExpression::is_instantiation_allowed() const {
	return true;
}

void ast::SubscriptExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_array->bind(context, scope);
	m_syn_index->bind(context, scope);
}

gc::Local<rt::Value> ast::SubscriptExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> array = m_syn_array->evaluate(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	std::size_t idx = evaluate_index(context, scope, exception);
	if (!!exception) return context->get_undefined_value();

	gc::Local<rt::Value> result = array->get_array_element(context, idx);
	assert(!!result);
	return result;
}

gc::Local<rt::Value> ast::SubscriptExpression::modify_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	rt::ValueModifier& modifier)
{
	gc::Local<rt::Value> array = m_syn_array->evaluate(context, scope, exception);
	if (!!exception) return exception;

	std::size_t idx = evaluate_index(context, scope, exception);
	if (!!exception) return exception;

	gc::Local<rt::Value> result;
	gc::Local<rt::Value> new_value = modifier.modify_short(result);
	if (!new_value) {
		gc::Local<rt::Value> old_value = array->get_array_element(context, idx);
		new_value = modifier.modify(old_value, result);
		assert(!!new_value);
	}

	array->set_array_element(context, idx, new_value);
	return result;
}

std::size_t ast::SubscriptExpression::evaluate_index(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> index = m_syn_index->evaluate(context, scope, exception);
	if (!!exception) return 0;

	ScriptIntegerType idx = index->get_integer();
	const std::size_t max_len = std::numeric_limits<std::size_t>::max();
	if (idx < 0 || idx > max_len) throw RuntimeError(m_syn_pos, "Index out of range");
	return static_cast<std::size_t>(idx);
}

//
//NameExpression
//

void ast::NameExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_name);
	gc_ref(m_name_descriptor);
}

void ast::NameExpression::syn_name(const SynName& name) {
	m_syn_name = name;
}

gc::Local<ss::TextPos> ast::NameExpression::get_pos() const {
	return m_syn_name->pos();
}

bool ast::NameExpression::is_assignment_allowed() const {
	return rt::DeclarationType::VARIABLE == m_name_descriptor->get_declaration_type();
}

bool ast::NameExpression::is_invocation_allowed() const {
	rt::DeclarationType type = m_name_descriptor->get_declaration_type();
	return rt::DeclarationType::FUNCTION == type
		|| rt::DeclarationType::VARIABLE == type
		|| rt::DeclarationType::CONSTANT == type;
}

bool ast::NameExpression::is_instantiation_allowed() const {
	rt::DeclarationType type = m_name_descriptor->get_declaration_type();
	return rt::DeclarationType::CLASS == type
		|| rt::DeclarationType::VARIABLE == type
		|| rt::DeclarationType::CONSTANT == type;
}

void ast::NameExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_scope_id = scope->get_id();
	m_name_descriptor = scope->lookup(m_syn_name);
}

gc::Local<rt::Value> ast::NameExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	scope->check_id(m_scope_id);
	gc::Local<rt::Value> value = m_name_descriptor->get(scope);
	assert(!value->is_void());
	if (value->is_undefined()) throw RuntimeError(m_syn_name->pos(), "Undefined value");
	return value;
}

gc::Local<rt::Value> ast::NameExpression::modify_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception,
	rt::ValueModifier& modifier)
{
	scope->check_id(m_scope_id);

	gc::Local<rt::Value> result;
	gc::Local<rt::Value> old_value = m_name_descriptor->get(scope);
	gc::Local<rt::Value> new_value = modifier.modify(old_value, result);
	assert(!!result);

	m_name_descriptor->set_modify(scope, new_value);
	return result;
}

//
//ThisExpression
//

void ast::ThisExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
}

void ast::ThisExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

gc::Local<ss::TextPos> ast::ThisExpression::get_pos() const {
	return m_syn_pos;
}

void ast::ThisExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_scope_ofs = scope->get_this_scope_ofs();
	if (rt::BAD_OFS == m_scope_ofs) {
		throw CompilationError(m_syn_pos, "No 'this' in current scope");
	}
}

gc::Local<rt::Value> ast::ThisExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	return scope->get_this(m_scope_ofs);
}

//
//FunctionExpression
//

void ast::FunctionExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_parameters);
	gc_ref(m_syn_body);
	gc_ref(m_parameter_descriptors);
	gc_ref(m_scope_descriptor);
}

void ast::FunctionExpression::initialize(){}

void ast::FunctionExpression::initialize(
	const ast_ptr<FunctionFormalParameters>& parameters,
	const ast_ptr<FunctionBody>& body)
{
	m_syn_parameters = parameters;
	m_syn_body = body;
}

void ast::FunctionExpression::syn_parameters(const ast_ptr<FunctionFormalParameters>& parameters) {
	m_syn_parameters = parameters;
}

void ast::FunctionExpression::syn_body(const ast_ptr<FunctionBody>& body) {
	m_syn_body = body;
}

gc::Local<ss::TextPos> ast::FunctionExpression::get_pos() const {
	return m_syn_body->get_pos();;
}

gc::Local<ss::TextPos> ast::FunctionExpression::get_start_pos() const {
	return !!m_syn_parameters ? m_syn_parameters->get_pos() : m_syn_body->get_pos();
}

bool ast::FunctionExpression::is_invocation_allowed() const {
	return true;
}

void ast::FunctionExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	std::unique_ptr<rt::BindScope> sub_scope = scope->create_nested_scope(false);

	if (!!m_syn_parameters) {
		ast_ptr<const ast_node_list<AstName>> parameters = m_syn_parameters->get_parameters();
		std::size_t param_cnt = parameters->size();
		m_parameter_descriptors = gc::Array<rt::NameDescriptor>::create(param_cnt);
		for (std::size_t i = 0; i < param_cnt; ++i) {
			(*m_parameter_descriptors)[i] = sub_scope->declare_variable((*parameters)[i], false);
		}
	}

	m_syn_body->get_block()->bind(context, sub_scope.get());
	m_scope_descriptor = sub_scope->create_scope_descriptor();
}

gc::Local<rt::Value> ast::FunctionExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	return gc::create<rt::FunctionValue>(scope, self(this));
}

gc::Local<rt::Value> ast::FunctionExpression::invoke(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	const gc::Local<gc::Array<rt::Value>>& arguments,
	gc::Local<rt::Value>& exception) const
{
	gc::Local<rt::ExecScope> sub_scope =
		scope->create_nested_scope(m_scope_descriptor, gc::Local<rt::Value>());

	if (!!m_syn_parameters) {
		ast_ptr<const ast::ast_node_list<ast::AstName>> parameters = m_syn_parameters->get_parameters();
		std::size_t arg_cnt = arguments->length();
		if (arg_cnt != parameters->size()) throw RuntimeError("Wrong number of arguments");

		for (std::size_t i = 0; i < arg_cnt; ++i) {
			gc::Local<rt::NameDescriptor> name_desc = (*m_parameter_descriptors)[i];
			gc::Local<rt::Value> v = arguments->get(i);
			name_desc->set_initialize(sub_scope, v);
		}
	}

	rt::StatementResult result = m_syn_body->get_block()->execute(context, sub_scope);
	if (rt::StatementResultType::RETURN == result.get_type()) {
		return result.get_value();
	} else if (rt::StatementResultType::THROW == result.get_type()) {
		exception = result.get_value();
		return context->get_undefined_value();
	} else {
		return context->get_value_factory()->get_void_value();
	}
}

//
//ClassExpression
//

void ast::ClassExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_body);
	gc_ref(m_scope_descriptor);
}

void ast::ClassExpression::initialize(){}

void ast::ClassExpression::initialize(const SynPos& pos, const ast_ptr<ClassBody>& body) {
	m_syn_pos = pos;
	m_syn_body = body;
}

void ast::ClassExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::ClassExpression::syn_body(const ast_ptr<ClassBody>& body) {
	m_syn_body = body;
}

gc::Local<ss::TextPos> ast::ClassExpression::get_pos() const {
	return m_syn_pos;
}

bool ast::ClassExpression::is_instantiation_allowed() const {
	return true;
}

void ast::ClassExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	std::unique_ptr<rt::BindScope> sub_scope = scope->create_nested_scope(true);

	m_syn_body->bind_constructor();
	ast_ptr<FunctionDeclaration> constructor = m_syn_body->get_constructor();
	const ast_ref<const ast_node_list<ClassMemberDeclaration>>& members = m_syn_body->get_members();

	if (!!constructor) constructor->bind_declare(context, sub_scope.get());
	for (const ast_ref<ClassMemberDeclaration>& d : *members) {
		d->bind_declare(context, sub_scope.get());
	}

	if (!!constructor) constructor->bind_define(context, sub_scope.get());
	for (const ast_ref<ClassMemberDeclaration>& d : *members) {
		d->bind_define(context, sub_scope.get());
	}

	m_scope_descriptor = sub_scope->create_scope_descriptor();
}

gc::Local<rt::Value> ast::ClassExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	return gc::create<rt::ClassValue>(scope, self(this));
}

gc::Local<rt::Value> ast::ClassExpression::instantiate(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	const gc::Local<gc::Array<rt::Value>>& arguments,
	gc::Local<rt::Value>& exception) const
{
	gc::Local<rt::ObjectValue> object = gc::create<rt::ObjectValue>(self(this), scope, m_scope_descriptor.local());
	gc::Local<rt::ExecScope> object_scope = object->get_object_scope();

	for (const ast_ref<ClassMemberDeclaration>& d : *m_syn_body->get_members()) {
		d->get_declaration()->exec_define(context, object_scope, exception);
		if (!!exception) return context->get_undefined_value();
	}

	ast_ptr<FunctionDeclaration> constructor = m_syn_body->get_constructor();
	if (!!constructor) {
		const ast_ref<FunctionExpression>& con_expr = constructor->get_expression();
		gc::Local<rt::Value> value = con_expr->invoke(context, object_scope, arguments, exception);
		if (!!exception) return context->get_undefined_value();
		if (!value->is_void()) throw RuntimeError(constructor->get_pos(), "Constructor must return nothing");
	}

	return object;
}

gc::Local<rt::Value> ast::ClassExpression::get_object_member(
	const gc::Local<rt::ExecScope>& object_scope,
	const gc::Local<rt::ExecScope>& access_scope,
	const gc::Local<const ss::NameInfo>& name_info) const
{
	const ast_ref<Declaration>& d = access_declaration(access_scope, name_info);
	return d->get_name_descriptor()->get(object_scope);
}

void ast::ClassExpression::set_object_member(
	const gc::Local<rt::ExecScope>& object_scope,
	const gc::Local<rt::ExecScope>& access_scope,
	const gc::Local<const ss::NameInfo>& name_info,
	const gc::Local<rt::Value>& value) const
{
	const ast_ref<Declaration>& d = access_declaration(access_scope, name_info);
	const rt::NameDescriptor* desc = d->get_name_descriptor();
	if (rt::DeclarationType::VARIABLE != desc->get_declaration_type()) {
		throw RuntimeError(
			std::string("Cannot modify a non-variable member: ") + name_info->get_str()->get_std_string());
	}
	d->get_name_descriptor()->set_modify(object_scope, value);
}

const ast::ast_ref<ast::Declaration>& ast::ClassExpression::access_declaration(
	const gc::Local<rt::ExecScope>& access_scope,
	const gc::Local<const ss::NameInfo>& name_info) const
{
	const ast_ref<ClassMemberDeclaration>& class_d = find_declaration(name_info);
	if (class_d->is_private()) {
		const rt::ScopeID& scope_id = m_scope_descriptor->get_id();
		if (!access_scope->get_scope_descriptor()->is_scope_accessible(scope_id)) {
			throw RuntimeError(std::string("Member is not accessible: ") + name_info->get_str()->get_std_string());
		}
	}
	return class_d->get_declaration();
}

const ast::ast_ref<ast::ClassMemberDeclaration>& ast::ClassExpression::find_declaration(
	const gc::Local<const ss::NameInfo>& name_info) const
{
	const NameID& name_id = name_info->get_id();
	for (const ast_ref<ClassMemberDeclaration>& class_d : *m_syn_body->get_members()) {
		const ast_ref<Declaration>& d = class_d->get_declaration();
		if (d->get_name()->get_id() == name_id) return class_d;
	}
	throw RuntimeError(std::string("Member not found: ") + name_info->get_str()->get_std_string());
}

//
//LiteralExpression
//

void ast::LiteralExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_value = get_runtime_value(context);
}

gc::Local<rt::Value> ast::LiteralExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	return m_value;
}

//
//IntegerLiteralExpression
//

void ast::IntegerLiteralExpression::gc_enumerate_refs() {
	LiteralExpression::gc_enumerate_refs();
	gc_ref(m_syn_value);
}

void ast::IntegerLiteralExpression::syn_value(const SynInteger& value) {
	m_syn_value = value;
}

gc::Local<ss::TextPos> ast::IntegerLiteralExpression::get_pos() const {
	return m_syn_value->pos();
}

gc::Local<rt::Value> ast::IntegerLiteralExpression::get_runtime_value(rt::BindContext* context) const {
	return context->get_value_factory()->get_integer_value(m_syn_value->value());
}

//
//FloatingPointLiteralExpression
//

void ast::FloatingPointLiteralExpression::gc_enumerate_refs() {
	LiteralExpression::gc_enumerate_refs();
	gc_ref(m_syn_value);
}

void ast::FloatingPointLiteralExpression::syn_value(const SynFloat& value) {
	m_syn_value = value;
}

gc::Local<ss::TextPos> ast::FloatingPointLiteralExpression::get_pos() const {
	return m_syn_value->pos();
}

gc::Local<rt::Value> ast::FloatingPointLiteralExpression::get_runtime_value(rt::BindContext* context) const {
	return context->get_value_factory()->get_float_value(m_syn_value->value());
}

//
//StringLiteralExpression
//

void ast::StringLiteralExpression::gc_enumerate_refs() {
	LiteralExpression::gc_enumerate_refs();
	gc_ref(m_syn_value);
}

void ast::StringLiteralExpression::syn_value(const SynString& value) {
	m_syn_value = value;
}

gc::Local<ss::TextPos> ast::StringLiteralExpression::get_pos() const {
	return m_syn_value->pos();
}

gc::Local<rt::Value> ast::StringLiteralExpression::get_runtime_value(rt::BindContext* context) const {
	//TODO Use cache for empty strings.
	//TODO Use cache for constant string literals defined in the code.
	return gc::create<rt::StringValue>(m_syn_value->value());
}

//
//BooleanLiteralExpression
//

void ast::BooleanLiteralExpression::gc_enumerate_refs() {
	Expression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
}

void ast::BooleanLiteralExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::BooleanLiteralExpression::syn_value(bool value) {
	m_syn_value = value;
}

gc::Local<ss::TextPos> ast::BooleanLiteralExpression::get_pos() const {
	return m_syn_pos;
}

void ast::BooleanLiteralExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	//Nothing.
}

gc::Local<rt::Value> ast::BooleanLiteralExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	return context->get_value_factory()->get_boolean_value(m_syn_value);
}

//
//NullExpression
//

void ast::NullExpression::gc_enumerate_refs() {
	Expression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
}

void ast::NullExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

gc::Local<ss::TextPos> ast::NullExpression::get_pos() const {
	return m_syn_pos;
}

void ast::NullExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	//Nothing.
}

gc::Local<rt::Value> ast::NullExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	return context->get_value_factory()->get_null_value();
}

//
//TypeofExpression
//

void ast::TypeofExpression::gc_enumerate_refs() {
	TerminalExpression::gc_enumerate_refs();
	gc_ref(m_syn_pos);
	gc_ref(m_syn_expression);
}

void ast::TypeofExpression::syn_pos(const SynPos& pos) {
	m_syn_pos = pos;
}

void ast::TypeofExpression::syn_expression(const ast_ptr<Expression>& expression) {
	m_syn_expression = expression;
}

gc::Local<ss::TextPos> ast::TypeofExpression::get_pos() const {
	return m_syn_pos;
}

void ast::TypeofExpression::bind(rt::BindContext* context, rt::BindScope* scope) {
	m_syn_expression->bind(context, scope);
}

gc::Local<rt::Value> ast::TypeofExpression::evaluate_0(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope,
	gc::Local<rt::Value>& exception)
{
	gc::Local<rt::Value> value = m_syn_expression->evaluate(context, scope, exception);
	if (!!exception) return value;

	StringLoc str = value->typeof(context);
	return context->get_value_factory()->get_string_value(str);
}
