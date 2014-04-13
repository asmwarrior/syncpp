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

//Basic API values.

#ifndef SYNSAMPLE_CORE_API_BASIC_H_INCLUDED
#define SYNSAMPLE_CORE_API_BASIC_H_INCLUDED

#include "api.h"
#include "gc.h"
#include "scope__dec.h"
#include "sysclassbld.h"
#include "sysvalue.h"
#include "value__dec.h"

//
//StringValue
//

class syn_script::rt::StringValue : public SysObjectValue {
	NONCOPYABLE(StringValue);

	StringRef m_value;

public:
	StringValue(){}
	void gc_enumerate_refs() override;
	void initialize(const StringLoc& value);

	OperandType get_operand_type() const override;
	StringLoc get_string() const override;
	StringLoc to_string(const gc::Local<ExecContext>& context) const override;

	gc::Local<Value> get_array_element(const gc::Local<ExecContext>& context, std::size_t index) override;

	StringLoc typeof(const gc::Local<ExecContext>& context) const override;

	bool value_equals(const gc::Local<const Value>& value) const override;
	std::size_t value_hash_code() const override;
	int value_compare_to(const gc::Local<Value>& value) const override;

protected:
	std::size_t get_sys_class_id() const override;

private:
	bool api_is_empty(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_length(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_char_at(const gc::Local<ExecContext>& context, ScriptIntegerType index);
	ScriptIntegerType api_index_of_1(const gc::Local<ExecContext>& context, ScriptIntegerType ch);

	ScriptIntegerType api_index_of_2(
		const gc::Local<ExecContext>& context,
		ScriptIntegerType ch,
		ScriptIntegerType index);

	StringLoc api_substring_1(
		const gc::Local<ExecContext>& context,
		ScriptIntegerType start_index);

	StringLoc api_substring_2(
		const gc::Local<ExecContext>& context,
		ScriptIntegerType start_index,
		ScriptIntegerType end_index);

	gc::Local<ByteArrayValue> api_get_bytes(const gc::Local<ExecContext>& context);
	gc::Local<ValueArray> api_get_lines(const gc::Local<ExecContext>& context);
	bool api_equals(const gc::Local<ExecContext>& context, const gc::Local<const Value>& value);
	ScriptIntegerType api_compare_to(const gc::Local<ExecContext>& context, const gc::Local<StringValue>& value);
	static gc::Local<Value> api_char(const gc::Local<ExecContext>& context, ScriptIntegerType code);

public:
	class API;
};

//
//ByteArrayValue
//

class syn_script::rt::ByteArrayValue : public SysObjectValue {
	NONCOPYABLE(ByteArrayValue);

	gc::Ref<ByteArray> m_array;

public:
	ByteArrayValue(){}
	void gc_enumerate_refs() override;
	void initialize(const gc::Local<ByteArray>& array);

	gc::Local<Value> get_array_element(const gc::Local<ExecContext>& context, std::size_t index) override;

	void set_array_element(
		const gc::Local<ExecContext>& context,
		std::size_t index,
		const gc::Local<Value>& value) override;

	gc::Local<ByteArray> get_array() const;

protected:
	std::size_t get_sys_class_id() const override;

private:
	static gc::Local<ByteArrayValue> create(const gc::Local<ExecContext>& context, ScriptIntegerType length);

private:
	ScriptIntegerType api_length(const gc::Local<ExecContext>& context);
	
	StringLoc api_to_string_0(const gc::Local<ExecContext>& context);

	StringLoc api_to_string_2(
		const gc::Local<ExecContext>& context,
		ScriptIntegerType start_ofs,
		ScriptIntegerType end_ofs);

public:
	class API;
};

#endif//SYNSAMPLE_CORE_API_BASIC_H_INCLUDED
