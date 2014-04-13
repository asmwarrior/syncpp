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

//Basic API implementations.

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <memory>
#include <sstream>

#include "api.h"
#include "api_basic.h"
#include "api_collection.h"
#include "common.h"
#include "gc.h"
#include "platform.h"
#include "scope.h"
#include "sysclass.h"
#include "sysclassbld__imp.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace rt = ss::rt;
namespace pf = ss::platform;

//
//StringValue::API
//

class rt::StringValue::API : public rt::SysAPI<StringValue> {
	void init() override {
		bld->add_method("is_empty", &StringValue::api_is_empty);
		bld->add_method("length", &StringValue::api_length);
		bld->add_method("char_at", &StringValue::api_char_at);
		bld->add_method("index_of", &StringValue::api_index_of_1);
		bld->add_method("index_of", &StringValue::api_index_of_2);
		bld->add_method("substring", &StringValue::api_substring_1);
		bld->add_method("substring", &StringValue::api_substring_2);
		bld->add_method("get_bytes", &StringValue::api_get_bytes);
		bld->add_method("get_lines", &StringValue::api_get_lines);
		bld->add_method("equals", &StringValue::api_equals);
		bld->add_method("compare_to", &StringValue::api_compare_to);
		bld->add_static_method("char", &StringValue::api_char);
	}
};

//
//StringValue
//

void rt::StringValue::gc_enumerate_refs() {
	Value::gc_enumerate_refs();
	gc_ref(m_value);
}

void rt::StringValue::initialize(const ss::StringLoc& value) {
	assert(!!value);
	m_value = value;
}

rt::OperandType rt::StringValue::get_operand_type() const {
	return rt::OperandType::STRING;
}

ss::StringLoc rt::StringValue::get_string() const {
	return m_value;
}

ss::StringLoc rt::StringValue::to_string(const gc::Local<ExecContext>& context) const {
	return m_value;
}

gc::Local<rt::Value> rt::StringValue::get_array_element(const gc::Local<ExecContext>& context, std::size_t index) {
	if (index >= m_value->length()) throw RuntimeError("Index out of bounds");
	return context->get_value_factory()->get_integer_value(m_value->char_at(index));
}

ss::StringLoc rt::StringValue::typeof(const gc::Local<ExecContext>& context) const {
	return gc::create<String>("string");
}

bool rt::StringValue::value_equals(const gc::Local<const Value>& value) const {
	const StringValue* v = dynamic_cast<const StringValue*>(value.get());
	return v && m_value->equals(v->m_value);
}

std::size_t rt::StringValue::value_hash_code() const {
	return m_value->hash_code();
}

int rt::StringValue::value_compare_to(const gc::Local<Value>& value) const {
	const StringValue* v = dynamic_cast<const StringValue*>(value.get());
	if (!v) throw RuntimeError("wrong type");
	return m_value->compare_to(v->m_value);
}

std::size_t rt::StringValue::get_sys_class_id() const {
	return API::get_class_id();
}

bool rt::StringValue::api_is_empty(const gc::Local<ExecContext>& context) {
	return m_value->is_empty();
}

ss::ScriptIntegerType rt::StringValue::api_length(const gc::Local<ExecContext>& context) {
	return m_value->length();
}

ss::ScriptIntegerType rt::StringValue::api_char_at(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType index)
{
	std::size_t idx = scriptint_to_size_ex(index);
	char ch = m_value->char_at(idx);
	return ch;
}

ss::ScriptIntegerType rt::StringValue::api_index_of_1(const gc::Local<ExecContext>& context, ss::ScriptIntegerType ch) {
	return api_index_of_2(context, ch, 0);
}

ss::ScriptIntegerType rt::StringValue::api_index_of_2(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType ch,
	ss::ScriptIntegerType index)
{
	std::size_t len = m_value->length();
	std::size_t i = scriptint_to_size_ex(index);
	char c = scriptint_to_char_ex(ch);
	while (i < len) {
		if (m_value->char_at(i) == c) return size_to_scriptint_ex(i);
		++i;
	}

	return int_to_scriptint(-1);
}

ss::StringLoc rt::StringValue::api_substring_1(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType start_index)
{
	if (start_index > m_value->length()) throw RuntimeError("Index out of bounds");
	std::size_t start_idx = scriptint_to_size(start_index);
	return m_value->substring(start_idx);
}

ss::StringLoc rt::StringValue::api_substring_2(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType start_index,
	ss::ScriptIntegerType end_index)
{
	std::size_t len = m_value->length();
	if (start_index > end_index || end_index > len) throw RuntimeError("Index out of bounds");

	std::size_t start_idx = scriptint_to_size(start_index);
	std::size_t end_idx = scriptint_to_size(end_index);
	return m_value->substring(start_idx, end_idx);
}

gc::Local<rt::ByteArrayValue> rt::StringValue::api_get_bytes(const gc::Local<ExecContext>& context) {
	gc::Local<ByteArray> array = m_value->get_bytes();
	return gc::create<ByteArrayValue>(array);
}

gc::Local<rt::ValueArray> rt::StringValue::api_get_lines(const gc::Local<ExecContext>& context) {
	const char* data = m_value->get_raw_data();
	const std::size_t len = m_value->length();
	std::size_t line_count = 1;
	for (std::size_t i = 0; i < len; ++i) {
		if (data[i] == '\n') ++line_count;
	}

	gc::Local<ValueArray> array = ValueArray::create(line_count);
	std::size_t idx = 0;
	std::size_t start_pos = 0;
	std::size_t pos = 0;
	while (pos < len) {
		char c = data[pos];

		if (c == '\r' || c == '\n') {
			StringLoc line = m_value->substring(start_pos, pos);
			assert(idx < line_count);
			(*array)[idx++] = context->get_value_factory()->get_string_value(line);
			++pos;
			if (c == '\r' && pos < len && data[pos] == '\n') ++pos;
			start_pos = pos;
		} else {
			++pos;
		}
	}

	StringLoc line = m_value->substring(start_pos, pos);
	assert(idx < line_count);
	(*array)[idx++] = context->get_value_factory()->get_string_value(line);
	assert(idx == line_count);

	return array;
}

bool rt::StringValue::api_equals(const gc::Local<ExecContext>& context, const gc::Local<const Value>& value) {
	const Value* other_value = value.get();
	const StringValue* other_str_value = dynamic_cast<const StringValue*>(other_value);
	bool result = other_str_value ? m_value->equals(other_str_value->m_value) : false;
	return result;
}

ss::ScriptIntegerType rt::StringValue::api_compare_to(
	const gc::Local<ExecContext>& context,
	const gc::Local<StringValue>& value)
{
	if (!value) throw RuntimeError("null pointer error");
	int d = m_value->compare_to(value->m_value);

	//In order to avoid possible range problems with integer conversion, let the return value be one of (-1, 0, 1)
	//instead of an arbitrary negative or positive value.
	if (d < 0) {
		d = -1;
	} else if (d > 0) {
		d = 1;
	}

	return int_to_scriptint(d);
}

gc::Local<rt::Value> rt::StringValue::api_char(const gc::Local<ExecContext>& context, ss::ScriptIntegerType code) {
	char c = scriptint_to_char_ex(code);
	return context->get_value_factory()->get_char_string_value(c);
}

//
//ByteArrayValue::API
//

class rt::ByteArrayValue::API : public SysAPI<ByteArrayValue> {
	void init() override {
		bld->add_constructor(create);
		bld->add_field("length", &ByteArrayValue::api_length);
		bld->add_method("to_string", &ByteArrayValue::api_to_string_0);
		bld->add_method("to_string", &ByteArrayValue::api_to_string_2);
	}
};

//
//ByteArrayValue
//

void rt::ByteArrayValue::gc_enumerate_refs() {
	gc_ref(m_array);
}

void rt::ByteArrayValue::initialize(const gc::Local<ss::ByteArray>& array) {
	m_array = array;
}

gc::Local<rt::Value> rt::ByteArrayValue::get_array_element(
	const gc::Local<ExecContext>& context,
	std::size_t index)
{
	std::size_t length = m_array->length();
	if (index >= length) throw RuntimeError("Array index out of bounds");
	
	char c = m_array->get(index);
	ScriptIntegerType v = char_to_scriptint_ex(c);
	return context->get_value_factory()->get_integer_value(v);
}

void rt::ByteArrayValue::set_array_element(
	const gc::Local<ExecContext>& context,
	std::size_t index,
	const gc::Local<Value>& value)
{
	std::size_t length = m_array->length();
	if (index >= length) throw RuntimeError("Array index out of bounds");

	ScriptIntegerType v = value->get_integer();
	if (v < 0 || v > std::numeric_limits<unsigned char>::max()) {
		throw RuntimeError(std::string("Value out of bounds: ") + std::to_string(v));
	}

	char c = scriptint_to_char_ex(v);
	m_array->get(index) = c;
}

gc::Local<ss::ByteArray> rt::ByteArrayValue::get_array() const {
	return m_array;
}

std::size_t rt::ByteArrayValue::get_sys_class_id() const {
	return API::get_class_id();
}

gc::Local<rt::ByteArrayValue> rt::ByteArrayValue::create(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType length)
{
	std::size_t s = scriptint_to_size_ex(length);
	gc::Local<ByteArray> array = ByteArray::create(s);
	return gc::create<ByteArrayValue>(array);
}

ss::ScriptIntegerType rt::ByteArrayValue::api_length(const gc::Local<ExecContext>& context) {
	return size_to_scriptint_ex(m_array->length());
}

ss::StringLoc rt::ByteArrayValue::api_to_string_0(const gc::Local<ExecContext>& context) {
	std::size_t len = m_array->length();
	if (!len) return context->get_value_factory()->get_empty_str();
	return gc::create<String>(m_array->raw_array(), len);
}

ss::StringLoc rt::ByteArrayValue::api_to_string_2(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType start_offset,
	ss::ScriptIntegerType end_offset)
{
	std::size_t array_len = m_array->length();
	if (start_offset > end_offset || end_offset > array_len) throw RuntimeError("Index out of bounds");

	std::size_t start_ofs = scriptint_to_size(start_offset);
	std::size_t end_ofs = scriptint_to_size(end_offset);
	std::size_t len = end_ofs - start_ofs;

	if (!len) return context->get_value_factory()->get_empty_str();
	return gc::create<String>(m_array->raw_array() + start_ofs, len);
}

//
//StringBufferValue : declaration
//

namespace syn_script {
	namespace rt {
		class StringBufferValue;
	}
}

//
//StringBufferValue : definition
//

class rt::StringBufferValue : public SysObjectValue {
	NONCOPYABLE(StringBufferValue);

	typedef gc::PrimitiveArray<char> CharArray;

	std::size_t m_capacity;
	std::size_t m_size;
	gc::Ref<CharArray> m_array;

public:
	StringBufferValue(){}
	void gc_enumerate_refs() override;
	void initialize();

	StringLoc to_string(const gc::Local<ExecContext>& context) const override;

	gc::Local<Value> get_array_element(const gc::Local<ExecContext>& context, std::size_t index) override;

protected:
	std::size_t get_sys_class_id() const override;

private:
	void expand(std::size_t min_capacity);

	static gc::Local<StringBufferValue> api_create(const gc::Local<ExecContext>& context);

	bool api_is_empty(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_length(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_char_at(const gc::Local<ExecContext>& context, ScriptIntegerType index);
	StringLoc api_to_string(const gc::Local<ExecContext>& context);
	void api_append_char(const gc::Local<ExecContext>& context, ScriptIntegerType value);
	void api_append(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
	void api_clear(const gc::Local<ExecContext>& context);

public:
	class API;
};

//
//StringBufferValue::API
//

class rt::StringBufferValue::API : public rt::SysAPI<StringBufferValue> {
	void init() override {
		bld->add_constructor(api_create);
		bld->add_method("is_empty", &StringBufferValue::api_is_empty);
		bld->add_method("length", &StringBufferValue::api_length);
		bld->add_method("charAt", &StringBufferValue::api_char_at);
		bld->add_method("to_string", &StringBufferValue::api_to_string);
		bld->add_method("append_char", &StringBufferValue::api_append_char);
		bld->add_method("append", &StringBufferValue::api_append);
		bld->add_method("clear", &StringBufferValue::api_clear);
	}
};

//
//StringBufferValue : implementation
//

void rt::StringBufferValue::gc_enumerate_refs() {
	SysObjectValue::gc_enumerate_refs();
	gc_ref(m_array);
}

void rt::StringBufferValue::initialize() {
	m_capacity = 16;
	m_array = CharArray::create(m_capacity);
	m_size = 0;
}

ss::StringLoc rt::StringBufferValue::to_string(const gc::Local<ExecContext>& context) const {
	const char* data = m_array->raw_array();
	//TODO Cache empty strings.
	return gc::create<String>(data, m_size);
}

gc::Local<rt::Value> rt::StringBufferValue::get_array_element(
	const gc::Local<ExecContext>& context,
	std::size_t index)
{
	ScriptIntegerType v = api_char_at(context, index);
	return context->get_value_factory()->get_integer_value(v);
}

std::size_t rt::StringBufferValue::get_sys_class_id() const {
	return API::get_class_id();
}

void rt::StringBufferValue::expand(std::size_t min_capacity) {
	std::size_t new_capacity = m_capacity * 3 / 2;
	if (new_capacity < min_capacity) new_capacity = min_capacity;

	gc::Local<CharArray> new_array = CharArray::create(new_capacity);
	const char* old_data = m_array->raw_array();
	char* new_data = new_array->raw_array();
	for (std::size_t i = 0; i < m_size; ++i) new_data[i] = old_data[i];
	m_array = new_array;
	m_capacity = new_capacity;
}

gc::Local<rt::StringBufferValue> rt::StringBufferValue::api_create(const gc::Local<ExecContext>& context) {
	return gc::create<StringBufferValue>();
}

bool rt::StringBufferValue::api_is_empty(const gc::Local<ExecContext>& context) {
	return !m_size;
}

ss::ScriptIntegerType rt::StringBufferValue::api_length(const gc::Local<ExecContext>& context) {
	return size_to_scriptint_ex(m_size);
}

ss::ScriptIntegerType rt::StringBufferValue::api_char_at(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType index)
{
	if (index >= m_size) throw RuntimeError("index out of range");
	std::size_t s = scriptint_to_size(index);
	return (*m_array)[s];
}

ss::StringLoc rt::StringBufferValue::api_to_string(const gc::Local<ExecContext>& context) {
	return to_string(context);
}

void rt::StringBufferValue::api_append_char(const gc::Local<ExecContext>& context, ss::ScriptIntegerType value) {
	if (m_size >= m_capacity) expand(m_size + 1);
	char c = scriptint_to_char_ex(value);
	(*m_array)[m_size++] = c;
}

void rt::StringBufferValue::api_append(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	StringLoc str = value->to_string(context);
	std::size_t len = str->length();
	if (!len) return;

	if (len > m_capacity || m_capacity - len < m_size) expand(m_size + len);

	char* dst_data = m_array->raw_array() + m_size;
	const char* src_data = str->get_raw_data();
	for (std::size_t i = 0; i < len; ++i) dst_data[i] = src_data[i];

	m_size += len;
}

void rt::StringBufferValue::api_clear(const gc::Local<ExecContext>& context) {
	m_size = 0;
}


//
//(Functions)
//

namespace syn_script {
	namespace rt {
		ScriptIntegerType api_current_time_millis(const gc::Local<ExecContext>& context);
		StringLoc api_current_time_str(const gc::Local<ExecContext>& context);
		ScriptIntegerType api_str_to_int(const gc::Local<ExecContext>& context, const StringLoc& str);
		bool api_windows(const gc::Local<ExecContext>& context);
		gc::Local<Value> api_args(const gc::Local<ExecContext>& context);
	}
}

ss::ScriptIntegerType rt::api_current_time_millis(const gc::Local<ExecContext>& context) {
	auto now = std::chrono::system_clock::now();
	//TODO Take into account that epoch may differ for different C++ implementations.
	auto time = now.time_since_epoch();

	typedef std::chrono::milliseconds Ms;
	static_assert(sizeof(Ms::rep) <= sizeof(ScriptIntegerType), "Wrong type size");
	Ms::rep cnt = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
	return cnt;
}

namespace {
	void print_integer(std::ostream& out, int w, int value) {
		out << std::setfill('0') << std::setw(w) << value;
	}
}

ss::StringLoc rt::api_current_time_str(const gc::Local<ExecContext>& context) {
	pf::DateTime date_time;
	pf::get_current_time(date_time);

	std::ostringstream out;
	print_integer(out, 4, date_time.m_year);
	out << '-';
	print_integer(out, 2, date_time.m_month + 1);
	out << '-';
	print_integer(out, 2, date_time.m_day);
	out << ' ';
	print_integer(out, 2, date_time.m_hour);
	out << ':';
	print_integer(out, 2, date_time.m_minute);
	out << ':';
	print_integer(out, 2, date_time.m_second);

	return gc::create<String>(out.str());
}

ss::ScriptIntegerType rt::api_str_to_int(const gc::Local<ExecContext>& context, const ss::StringLoc& str) {
	const std::size_t len = str->length();
	if (!len) throw RuntimeError("String is empty");

	ScriptIntegerType v = 0;
	for (std::size_t i = 0; i < len; ++i) {
		char c = str->char_at(i);
		//TODO Use a more reliable approach to check whether the character is a digit.
		if (c < '0' || c > '9') throw RuntimeError("Invalid digit");

		//TODO Check overflow.
		v = v * 10 + (c - '0');
	}
	return v;
}

bool rt::api_windows(const gc::Local<ExecContext>& context) {
	return pf::IS_WINDOWS;
}

gc::Local<rt::Value> rt::api_args(const gc::Local<ExecContext>& context) {
	return context->get_value_factory()->get_arguments_value();
}

//
//(Misc.)
//

namespace {
	rt::SysNamespaceInitializer s_sys_namespace_initializer([](rt::SysClassBuilder<rt::SysNamespaceValue>& bld){
		bld.add_class<rt::StringValue>("String");
		bld.add_class<rt::ByteArrayValue>("Bytes");
		bld.add_class<rt::StringBufferValue>("StringBuffer");
		bld.add_static_method("current_time_millis", &rt::api_current_time_millis);
		bld.add_static_method("current_time_str", &rt::api_current_time_str);
		bld.add_static_method("str_to_int", &rt::api_str_to_int);
		bld.add_static_field("windows", &rt::api_windows);
		bld.add_static_field("args", &rt::api_args);
	});
}

void link__api_basic(){}
