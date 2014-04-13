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

//Basic value classes implementation.

#include <memory>
#include <sstream>

#include "api_basic.h"
#include "ast.h"
#include "ast_combined.h"
#include "common.h"
#include "gc.h"
#include "name.h"
#include "stringex.h"
#include "sysclass.h"
#include "sysclassbld.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;
namespace ast = ss::ast;

namespace {
	gc::Local<rt::Value> create_char_string(char c) {
		ss::StringLoc str = gc::create<ss::String>(&c, 1);
		return gc::create<rt::StringValue>(str);
	}
}

//
//Value
//

bool rt::Value::is_undefined() const {
	return false;
}

bool rt::Value::is_void() const {
	return false;
}

bool rt::Value::is_null() const {
	return false;
}

bool rt::Value::get_boolean() const {
	throw RuntimeError("Not a boolean value");
}

ss::ScriptIntegerType rt::Value::get_integer() const {
	throw RuntimeError("Not an integer number");
}

ss::ScriptFloatType rt::Value::get_float() const {
	throw RuntimeError("Not a floating-point number");
}

ss::StringLoc rt::Value::get_string() const {
	throw RuntimeError("Not a string");
}

ss::StringLoc rt::Value::to_string(const gc::Local<ExecContext>& context) const {
	throw RuntimeError("Not supported");
}

rt::OperandType rt::Value::get_operand_type() const {
	throw RuntimeError("Invalid operand type");
}

bool rt::Value::iterate(rt::InternalValueIterator& iterator) {
	throw RuntimeError("Not a collection");
}

gc::Local<rt::Value> rt::Value::get_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<ExecScope>& scope,
	const gc::Local<const ss::NameInfo>& name_info)
{
	throw RuntimeError("Not an object");
}

gc::Local<rt::Value> rt::Value::get_array_element(const gc::Local<ExecContext>& context, std::size_t index) {
	throw RuntimeError("Not an array");
}

void rt::Value::set_member(
	const gc::Local<ExecContext>& context,
	const gc::Local<ExecScope>& scope,
	const gc::Local<const ss::NameInfo>& name_info,
	const gc::Local<Value>& value)
{
	get_member(context, scope, name_info);
	throw RuntimeError("Cannot modify a member");
}

void rt::Value::set_array_element(
	const gc::Local<ExecContext>& context,
	std::size_t index,
	const gc::Local<Value>& value)
{
	get_array_element(context, index);
	throw RuntimeError("Cannot modify an element");
}

gc::Local<rt::Value> rt::Value::invoke(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	gc::Local<rt::Value>& exception)
{
	throw RuntimeError("Not a function");
}

gc::Local<rt::Value> rt::Value::instantiate(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& arguments,
	gc::Local<rt::Value>& exception)
{
	throw RuntimeError("Not a class");
}

ss::StringLoc rt::Value::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("unknown");
}

bool rt::Value::value_equals(const gc::Local<const Value>& value) const {
	throw RuntimeError("equals() is not supported");
}

std::size_t rt::Value::value_hash_code() const {
	throw RuntimeError("hash_code() is not supported");
}

int rt::Value::value_compare_to(const gc::Local<Value>& value) const {
	throw RuntimeError("compare_to() is not supported");
}

//
//ReferenceValue
//

rt::OperandType rt::ReferenceValue::get_operand_type() const {
	return rt::OperandType::REFERENCE;
}

//
//ValueFactory
//

rt::ValueFactory::ValueFactory(ss::NameRegistry& name_registry, const gc::Local<ss::StringArray>& arguments) {
	m_undefined_value = gc::create<UndefinedValue>();
	m_void_value = gc::create<VoidValue>();
	m_null_value = gc::create<NullValue>();
	m_false_value = gc::create<BooleanValue>(false);
	m_true_value = gc::create<BooleanValue>(true);

	m_empty_str = gc::create<String>("");
	m_null_str = gc::create<String>("null");
	m_false_str = gc::create<String>("false");
	m_true_str = gc::create<String>("true");

	m_arguments_value = create_arguments_value(arguments);
	m_int_cache = create_int_cache();
	m_float_cache = create_float_cache();;
	m_char_str_cache = create_char_str_cache();

	init_sys_classes(name_registry);
}

gc::Local<rt::Value> rt::ValueFactory::create_arguments_value(const gc::Local<ss::StringArray>& arguments) {
	const std::size_t n = arguments->length();
	gc::Local<ValueArray> array = ValueArray::create(n);
	for (std::size_t i = 0; i < n; ++i) {
		StringLoc arg = (*arguments)[i];
		gc::Local<Value> arg_value = gc::create<StringValue>(arg);
		(*array)[i] = arg_value;
	}
	return gc::create<ArrayValue>(array);
}

gc::Local<rt::ValueArray> rt::ValueFactory::create_int_cache() {
	gc::Local<ValueArray> cache = gc::Array<Value>::create(INT_CACHE_MAX - INT_CACHE_MIN + 1);
	for (int v = INT_CACHE_MIN; v <= INT_CACHE_MAX; ++v) {
		ScriptIntegerType sv = int_to_scriptint_ex(v);
		(*cache)[v - INT_CACHE_MIN] = gc::create<IntegerValue>(sv);
	}
	return cache;
}

gc::Local<rt::ValueArray> rt::ValueFactory::create_float_cache() {
	gc::Local<ValueArray> cache = ValueArray::create(FLOAT_CACHE_MAX - FLOAT_CACHE_MIN + 1);
	for (int v = FLOAT_CACHE_MIN; v <= FLOAT_CACHE_MAX; ++v) {
		ScriptFloatType sv = v;
		(*cache)[v - FLOAT_CACHE_MIN] = gc::create<FloatValue>(sv);
	}
	return cache;
}

gc::Local<rt::ValueArray> rt::ValueFactory::create_char_str_cache() {
	gc::Local<ValueArray> cache = ValueArray::create(CHAR_CACHE_SIZE);
	for (std::size_t v = 0; v < CHAR_CACHE_SIZE; ++v) {
		unsigned char uc = static_cast<unsigned char>(v);
		char c = reinterpret_cast<char&>(uc);
		(*cache)[v] = create_char_string(c);
	}
	return cache;
}

void rt::ValueFactory::init_sys_classes(ss::NameRegistry& name_registry) {
	const std::vector<const BasicSysClassInitializer*>& cls_inits =
		BasicSysClassInitializer::get_class_initializers();
	std::size_t cls_cnt = cls_inits.size();

	m_sys_classes = gc::Array<SysClass>::create(cls_cnt);

	for (const BasicSysClassInitializer* cls_init : cls_inits) {
		std::size_t id = cls_init->get_class_id();
		assert(id < cls_cnt);
		assert(!m_sys_classes->get(id));
		m_sys_classes->get(id) = cls_init->create_sys_class(name_registry);
	}

	for (std::size_t i = 0; i < cls_cnt; ++i) {
		assert(!!m_sys_classes->get(i));
	}
}

gc::Local<rt::Value> rt::ValueFactory::get_arguments_value() const {
	return m_arguments_value;
}

gc::Local<rt::Value> rt::ValueFactory::get_undefined_value() const {
	return m_undefined_value;
}

gc::Local<rt::Value> rt::ValueFactory::get_void_value() const {
	return m_void_value;
}

gc::Local<rt::Value> rt::ValueFactory::get_null_value() const {
	return m_null_value;
}

gc::Local<rt::Value> rt::ValueFactory::get_boolean_value(bool value) const {
	return value ? m_true_value : m_false_value;
}

gc::Local<rt::Value> rt::ValueFactory::get_integer_value(ss::ScriptIntegerType value) const {
	if (cmp_scriptint_int(value, INT_CACHE_MIN) >= 0 && cmp_scriptint_int(value, INT_CACHE_MAX) <= 0) {
		int ivalue = scriptint_to_int(value);
		int idx = ivalue - INT_CACHE_MIN;
		return (*m_int_cache)[idx];
	}
	return gc::create<IntegerValue>(value);
}

gc::Local<rt::Value> rt::ValueFactory::get_float_value(ss::ScriptFloatType value) const {
	if (value >= FLOAT_CACHE_MIN && value <= FLOAT_CACHE_MAX) {
		int ivalue = scriptfloat_to_int(value);
		if (ivalue == value) {
			int i = ivalue - FLOAT_CACHE_MIN;
			return (*m_float_cache)[i];
		}
	}
	return gc::create<FloatValue>(value);
}

gc::Local<rt::Value> rt::ValueFactory::get_string_value(const ss::StringLoc& value) const {
	return gc::create<StringValue>(value);
}

gc::Local<rt::Value> rt::ValueFactory::get_char_string_value(char c) const {
	unsigned char uc = reinterpret_cast<unsigned char&>(c);
	std::size_t v = uc;
	if (v < CHAR_CACHE_SIZE) return (*m_char_str_cache)[v];
	return create_char_string(c);	
}

ss::StringLoc rt::ValueFactory::get_empty_str() const {
	return m_empty_str;
}

ss::StringLoc rt::ValueFactory::get_null_str() const {
	return m_null_str;
}

ss::StringLoc rt::ValueFactory::get_false_str() const {
	return m_false_str;
}

ss::StringLoc rt::ValueFactory::get_true_str() const {
	return m_true_str;
}

gc::Local<rt::SysClass> rt::ValueFactory::get_sys_class(std::size_t class_id) const {
	assert(class_id < m_sys_classes->length());
	return m_sys_classes->get(class_id);
}
