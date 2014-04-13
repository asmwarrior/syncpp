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

//Core value classes implementation.

#include <cassert>
#include <memory>

#include "api_basic.h"
#include "api_io.h"
#include "ast.h"
#include "ast_combined.h"
#include "common.h"
#include "gc.h"
#include "name.h"
#include "stacktrace.h"
#include "stringex.h"
#include "sysclass.h"
#include "sysclassbld__imp.h"
#include "value.h"
#include "value_core.h"
#include "value_util.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;
namespace ast = ss::ast;

//
//BooleanValue
//

rt::OperandType rt::BooleanValue::get_operand_type() const {
	return OperandType::BOOLEAN;
}

bool rt::BooleanValue::get_boolean() const {
	return get_value();
}

ss::StringLoc rt::BooleanValue::to_string(const gc::Local<ExecContext>& context) const {
	const ValueFactory* value_factory = context->get_value_factory();
	return get_value() ? value_factory->get_true_str() : value_factory->get_false_str();
}

ss::StringLoc rt::BooleanValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("boolean");
}

bool rt::BooleanValue::value_equals(const gc::Local<const Value>& value) const {
	const BooleanValue* v = dynamic_cast<const BooleanValue*>(value.get());
	return v && get_value() == v->get_value();
}

std::size_t rt::BooleanValue::value_hash_code() const {
	return get_value();
}

int rt::BooleanValue::value_compare_to(const gc::Local<Value>& value) const {
	const BooleanValue* v = dynamic_cast<const BooleanValue*>(value.get());
	if (!v) throw RuntimeError("wrong type");
	bool a = get_value();
	bool b = v->get_value();
	return a < b ? -1 : (a > b ? 1 : 0);
}

//
//IntegerValue
//

rt::OperandType rt::IntegerValue::get_operand_type() const {
	return rt::OperandType::INTEGER;
}

ss::ScriptIntegerType rt::IntegerValue::get_integer() const {
	return get_value();
}

ss::StringLoc rt::IntegerValue::to_string(const gc::Local<ExecContext>& context) const {
	return integer_to_string(context, get_value());
}

ss::StringLoc rt::IntegerValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("integer");
}

bool rt::IntegerValue::value_equals(const gc::Local<const Value>& value) const {
	const IntegerValue* v = dynamic_cast<const IntegerValue*>(value.get());
	return v && get_value() == v->get_value();
}

std::size_t rt::IntegerValue::value_hash_code() const {
	std::size_t hash = scriptint_to_hashcode(get_value());
	return hash;
}

int rt::IntegerValue::value_compare_to(const gc::Local<Value>& value) const {
	const IntegerValue* v = dynamic_cast<const IntegerValue*>(value.get());
	if (!v) throw RuntimeError("wrong type");
	ScriptIntegerType a = get_value();
	ScriptIntegerType b = v->get_value();
	return scriptint_sign(a - b);
}

//
//FloatValue
//

rt::OperandType rt::FloatValue::get_operand_type() const {
	return rt::OperandType::FLOAT;
}

ss::ScriptFloatType rt::FloatValue::get_float() const {
	return get_value();
}

ss::StringLoc rt::FloatValue::to_string(const gc::Local<ExecContext>& context) const {
	return float_to_string(context, get_value());
}

ss::StringLoc rt::FloatValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("float");
}

int rt::FloatValue::value_compare_to(const gc::Local<Value>& value) const {
	const FloatValue* v = dynamic_cast<const FloatValue*>(value.get());
	if (!v) throw RuntimeError("wrong type");
	ScriptFloatType a = get_value();
	ScriptFloatType b = v->get_value();
	return a < b ? -1 : (a > b ? 1 : 0);
}

//
//UndefinedValue
//

bool rt::UndefinedValue::is_undefined() const {
	return true;
}

//
//VoidValue
//

bool rt::VoidValue::is_void() const {
	return true;
}

//
//NullValue
//

bool rt::NullValue::is_null() const {
	return true;
}

ss::StringLoc rt::NullValue::to_string(const gc::Local<ExecContext>& context) const {
	return context->get_value_factory()->get_null_str();
}

bool rt::NullValue::iterate(InternalValueIterator& iterator) {
	throw null_pointer_error();
}

gc::Local<rt::Value> rt::NullValue::get_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<ExecScope>& scope,
	const gc::Local<const ss::NameInfo>& name_info)
{
	throw null_pointer_error();
}

gc::Local<rt::Value> rt::NullValue::get_array_element(const gc::Local<ExecContext>& context, std::size_t index) {
	throw null_pointer_error();
}

gc::Local<rt::Value> rt::NullValue::invoke(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	gc::Local<rt::Value>& exception)
{
	throw null_pointer_error();
}

gc::Local<rt::Value> rt::NullValue::instantiate(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	gc::Local<rt::Value>& exception)
{
	throw null_pointer_error();
}

ss::StringLoc rt::NullValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("null");
}

bool rt::NullValue::value_equals(const gc::Local<const Value>& value) const {
	throw null_pointer_error();
}

std::size_t rt::NullValue::value_hash_code() const {
	throw null_pointer_error();
}

int rt::NullValue::value_compare_to(const gc::Local<Value>& value) const {
	throw null_pointer_error();
}

ss::RuntimeError rt::NullValue::null_pointer_error() const {
	return RuntimeError("Null pointer access");
}

//
//ArrayValue::API
//

class rt::ArrayValue::API : public SysAPI<ArrayValue> {
	void init() override {
		bld->add_field("length", &ArrayValue::api_length);
		bld->add_method("sort", &ArrayValue::api_sort);
	}
};

//
//ArrayValue
//

void rt::ArrayValue::gc_enumerate_refs() {
	gc_ref(m_array);
}

void rt::ArrayValue::initialize(const gc::Local<gc::Array<Value>>& array) {
	m_array = array;

#ifndef NDEBUG
	for (std::size_t i = 0, n = array->length(); i < n; ++i) assert(!!(*array)[i]);
#endif
}

ss::StringLoc rt::ArrayValue::to_string(const gc::Local<ExecContext>& context) const {
	return array_to_string(context, m_array, 0, m_array->length());
}

bool rt::ArrayValue::iterate(InternalValueIterator& iterator) {
	for (std::size_t i = 0, n = m_array->length(); i < n; ++i) {
		if (!iterator.iterate(m_array->get(i))) return false;
	}
	return true;
}

gc::Local<rt::Value> rt::ArrayValue::get_array_element(const gc::Local<ExecContext>& context, std::size_t index) {
	return get_element_ref(index);
}

void rt::ArrayValue::set_array_element(
	const gc::Local<ExecContext>& context,
	std::size_t index,
	const gc::Local<Value>& value)
{
	get_element_ref(index) = value;
}

ss::StringLoc rt::ArrayValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("array");
}

const gc::Ref<gc::Array<rt::Value>>& rt::ArrayValue::get_array() const {
	return m_array;
}

gc::Ref<rt::Value>& rt::ArrayValue::get_element_ref(std::size_t index) {
	std::size_t length = m_array->length();
	if (index >= length) {
		throw RuntimeError(std::string("Array index out of bounds: ") + std::to_string(index)
			+ " >= " + std::to_string(length));
	}
	return m_array->get(index);
}

std::size_t rt::ArrayValue::get_sys_class_id() const {
	return API::get_class_id();
}

ss::ScriptIntegerType rt::ArrayValue::api_length(const gc::Local<ExecContext>& context) {
	return size_to_scriptint_ex(m_array->length());
}

void rt::ArrayValue::api_sort(const gc::Local<ExecContext>& context) {
	array_sort(context, m_array, 0, m_array->length());
}

//
//FunctionValue
//

void rt::FunctionValue::gc_enumerate_refs() {
	gc_ref(m_scope);
	gc_ref(m_expr);
}

void rt::FunctionValue::initialize(
	const gc::Local<ExecScope>& scope,
	const gc::Local<ast::FunctionExpression>& expr)
{
	m_scope = scope;
	m_expr = expr;
}

gc::Local<rt::Value> rt::FunctionValue::invoke(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	gc::Local<rt::Value>& exception)
{
	return m_expr->invoke(context, m_scope, arguments, exception);
}

ss::StringLoc rt::FunctionValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("function");
}

//
//ClassValue
//

void rt::ClassValue::gc_enumerate_refs() {
	gc_ref(m_scope);
	gc_ref(m_expr);
}

void rt::ClassValue::initialize(const gc::Local<ExecScope>& scope, const gc::Local<ast::ClassExpression>& expr) {
	m_scope = scope;
	m_expr = expr;
}

gc::Local<rt::Value> rt::ClassValue::instantiate(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	gc::Local<rt::Value>& exception)
{
	return m_expr->instantiate(context, m_scope, arguments, exception);
}

ss::StringLoc rt::ClassValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("class");
}

//
//ObjectValue
//

void rt::ObjectValue::gc_enumerate_refs() {
	gc_ref(m_expr);
	gc_ref(m_scope);
}

void rt::ObjectValue::initialize(
	const gc::Local<const ast::ClassExpression>& expr,
	const gc::Local<ExecScope>& outer_scope,
	const gc::Local<ScopeDescriptor>& scope_descriptor)
{
	m_expr = expr;
	m_scope = outer_scope->create_nested_scope(scope_descriptor, self(this));
}

gc::Local<rt::Value> rt::ObjectValue::get_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<ExecScope>& scope,
	const gc::Local<const ss::NameInfo>& name_info)
{
	return m_expr->get_object_member(m_scope, scope, name_info);
}

void rt::ObjectValue::set_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<ExecScope>& scope,
	const gc::Local<const ss::NameInfo>& name_info,
	const gc::Local<Value>& value)
{
	m_expr->set_object_member(m_scope, scope, name_info, value);
}

ss::StringLoc rt::ObjectValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("object");
}

gc::Local<rt::ExecScope> rt::ObjectValue::get_object_scope() {
	return m_scope;
}

//
//ExceptionValue::API
//

class rt::ExceptionValue::API : public SysAPI<ExceptionValue> {
	void init() override {
		bld->add_method("print", &ExceptionValue::api_print_0);
		bld->add_method("print", &ExceptionValue::api_print_1);
	}
};

//
//ExceptionValue
//

void rt::ExceptionValue::gc_enumerate_refs() {
	gc_ref(m_value);
	gc_ref(m_stack_trace);
}

void rt::ExceptionValue::initialize(
	const gc::Local<Value>& value,
	const gc::Local<gc::Array<rt::StackTraceElement>>& stack_trace)
{
	m_value = value;
	m_stack_trace = stack_trace;
}

ss::StringLoc rt::ExceptionValue::to_string(const gc::Local<ExecContext>& context) const {
	return m_value->to_string(context);
}

ss::StringLoc rt::ExceptionValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("exception");
}

void rt::ExceptionValue::print_stack_trace(const gc::Local<ExecContext>& context) const {
	print_stack_trace(context, std::cout);
}

void rt::ExceptionValue::print_stack_trace(const gc::Local<ExecContext>& context, std::ostream& out) const {
	if (!m_value) {
		out << "null" << std::endl;
	} else {
		StringLoc str = m_value->to_string(context);
		out << str << std::endl;

		for (std::size_t i = 0, n = m_stack_trace->length(); i < n; ++i) {
			out << "\tat " << m_stack_trace->get(i) << std::endl;
		}
	}
}

std::size_t rt::ExceptionValue::get_sys_class_id() const {
	return API::get_class_id();
}

void rt::ExceptionValue::api_print_0(const gc::Local<ExecContext>& context) {
	print_stack_trace(context);
}

void rt::ExceptionValue::api_print_1(const gc::Local<ExecContext>& context, const gc::Local<TextOutputValue>& out) {
	print_stack_trace(context, out->get_out());
}
