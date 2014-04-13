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

//Collections API implementation.

#include <algorithm>
#include <memory>
#include <sstream>

#include "api.h"
#include "api_collection.h"
#include "common.h"
#include "gc.h"
#include "gc_hashmap.h"
#include "platform_file.h"
#include "scope.h"
#include "stringex.h"
#include "sysclass.h"
#include "sysclassbld.h"
#include "sysclassbld__imp.h"
#include "value.h"
#include "value_core.h"
#include "value_util.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace rt = ss::rt;

namespace syn_script {
	namespace rt {
		class ArrayListValue;
		class HashSetValue;
	}
}

//
//ArrayListValue : definition
//

class rt::ArrayListValue : public SysObjectValue {
	NONCOPYABLE(ArrayListValue);

	gc::Ref<ValueArray> m_array;
	std::size_t m_size;

public:
	ArrayListValue(){}
	void gc_enumerate_refs() override;
	void initialize(std::size_t initial_capacity = 0);

	StringLoc to_string(const gc::Local<ExecContext>& context) const override;
	bool iterate(InternalValueIterator& iterator) override;
	gc::Local<Value> get_array_element(const gc::Local<ExecContext>& context, std::size_t index) override;

	void set_array_element(
		const gc::Local<ExecContext>& context,
		std::size_t index,
		const gc::Local<Value>& value) override;

protected:
	std::size_t get_sys_class_id() const override;

private:
	static gc::Local<ArrayListValue> api_create_0(const gc::Local<ExecContext>& context);

	static gc::Local<ArrayListValue> api_create_1(
		const gc::Local<ExecContext>& context,
		ScriptIntegerType initial_capacity);

	bool api_is_empty(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_size(const gc::Local<ExecContext>& context);
	void api_clear(const gc::Local<ExecContext>& context);
	bool api_contains(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
	ScriptIntegerType api_index_of(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
	gc::Local<Value> api_get(const gc::Local<ExecContext>& context, ScriptIntegerType index);
	void api_remove(const gc::Local<ExecContext>& context, ScriptIntegerType index);
	gc::Local<ValueArray> api_to_array(const gc::Local<ExecContext>& context);
	void api_add(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
	void api_sort(const gc::Local<ExecContext>& context);

public:
	class API;
};

//
//ArrayListValue::API
//

class rt::ArrayListValue::API : public SysAPI<ArrayListValue> {
	void init() override {
		bld->add_constructor(api_create_0);
		bld->add_constructor(api_create_1);
		bld->add_method("is_empty", &ArrayListValue::api_is_empty);
		bld->add_method("size", &ArrayListValue::api_size);
		bld->add_method("clear", &ArrayListValue::api_clear);
		bld->add_method("contains", &ArrayListValue::api_contains);
		bld->add_method("index_of", &ArrayListValue::api_index_of);
		bld->add_method("get", &ArrayListValue::api_get);
		bld->add_method("add", &ArrayListValue::api_add);
		bld->add_method("remove", &ArrayListValue::api_remove);
		bld->add_method("to_array", &ArrayListValue::api_to_array);
		bld->add_method("sort", &ArrayListValue::api_sort);
	}
};

//
//ArrayListValue : implementation
//

void rt::ArrayListValue::gc_enumerate_refs() {
	SysObjectValue::gc_enumerate_refs();
	gc_ref(m_array);
}

void rt::ArrayListValue::initialize(std::size_t initial_capacity) {
	std::size_t capacity = std::max((std::size_t)16, initial_capacity);
	m_array = ValueArray::create(capacity);
	m_size = 0;
}

ss::StringLoc rt::ArrayListValue::to_string(const gc::Local<ExecContext>& context) const {
	return array_to_string(context, m_array, 0, m_size);
}

bool rt::ArrayListValue::iterate(InternalValueIterator& iterator) {
	for (std::size_t i = 0; i < m_size; ++i) {
		if (!iterator.iterate((*m_array)[i])) return false;
	}
	return true;
}

gc::Local<rt::Value> rt::ArrayListValue::get_array_element(
	const gc::Local<ExecContext>& context,
	std::size_t index)
{
	if (index >= m_size) throw RuntimeError("index out of bounds");
	return (*m_array)[index];
}

void rt::ArrayListValue::set_array_element(
	const gc::Local<ExecContext>& context,
	std::size_t index,
	const gc::Local<Value>& value)
{
	assert(!!value);
	if (index >= m_size) throw RuntimeError("index out of bounds");
	(*m_array)[index] = value;
}

std::size_t rt::ArrayListValue::get_sys_class_id() const {
	return API::get_class_id();
}

gc::Local<rt::ArrayListValue> rt::ArrayListValue::api_create_0(const gc::Local<ExecContext>& context) {
	return gc::create<ArrayListValue>();
}

gc::Local<rt::ArrayListValue> rt::ArrayListValue::api_create_1(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType initial_capacity)
{
	std::size_t s = scriptint_to_size_ex(initial_capacity);
	return gc::create<ArrayListValue>(s);
}

bool rt::ArrayListValue::api_is_empty(const gc::Local<ExecContext>& context) {
	return !m_size;
}

ss::ScriptIntegerType rt::ArrayListValue::api_size(const gc::Local<ExecContext>& context) {
	return size_to_scriptint_ex(m_size);
}

void rt::ArrayListValue::api_clear(const gc::Local<ExecContext>& context) {
	for (std::size_t i = 0; i < m_size; ++i) (*m_array)[i] = nullptr;
	m_size = 0;
}

bool rt::ArrayListValue::api_contains(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	assert(!!value);
	for (std::size_t i = 0; i < m_size; ++i) {
		if (value->value_equals((*m_array)[i])) return true;
	}
	return false;
}

ss::ScriptIntegerType rt::ArrayListValue::api_index_of(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	assert(!!value);
	for (std::size_t i = 0; i < m_size; ++i) {
		if (value->value_equals((*m_array)[i])) return size_to_scriptint_ex(i);
	}
	return int_to_scriptint(-1);
}

gc::Local<rt::Value> rt::ArrayListValue::api_get(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType index)
{
	std::size_t sindex = scriptint_to_size_ex(index);
	return get_array_element(context, sindex);
}

void rt::ArrayListValue::api_remove(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType index)
{
	if (index >= m_size) throw RuntimeError("index out of bounds");
	std::size_t idx = scriptint_to_size(index);
	for (std::size_t i = idx; i < m_size - 1; ++i) {
		(*m_array)[i] = (*m_array)[i + 1];
	}
	--m_size;
	(*m_array)[m_size] = nullptr;
}

gc::Local<gc::Array<rt::Value>> rt::ArrayListValue::api_to_array(const gc::Local<ExecContext>& context) {
	//TODO Cache empty arrays.
	gc::Local<ValueArray> array = ValueArray::create(m_size);
	for (std::size_t i = 0; i < m_size; ++i) (*array)[i] = (*m_array)[i];
	return array;
}

void rt::ArrayListValue::api_add(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	std::size_t capacity = m_array->length();
	if (m_size >= capacity) {
		std::size_t delta_capacity = capacity >> 1;
		std::size_t new_capacity = (capacity <= SIZE_MAX - delta_capacity) ? capacity + delta_capacity : SIZE_MAX;
		if (new_capacity <= capacity) throw RuntimeError("Size limit exceeded");

		gc::Local<ValueArray> new_array = ValueArray::create(new_capacity);
		for (std::size_t i = 0; i < m_size; ++i) (*new_array)[i] = (*m_array)[i];
		m_array = new_array;
	}
	(*m_array)[m_size++] = value;
}

void rt::ArrayListValue::api_sort(const gc::Local<ExecContext>& context) {
	array_sort(context, m_array, 0, m_size);
}

//
//HashSetValue : definition
//

class rt::HashSetValue : public SysObjectValue {
	NONCOPYABLE(HashSetValue);

	gc::Ref<ValueHashMap> m_map;

public:
	HashSetValue(){}
	void gc_enumerate_refs() override;
	void initialize();

	StringLoc to_string(const gc::Local<ExecContext>& context) const override;
	bool iterate(InternalValueIterator& iterator) override;

protected:
	std::size_t get_sys_class_id() const override;

private:
	static gc::Local<HashSetValue> api_create(const gc::Local<ExecContext>& context);

	bool api_is_empty(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_size(const gc::Local<ExecContext>& context);
	void api_clear(const gc::Local<ExecContext>& context);
	bool api_contains(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
	bool api_add(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
	bool api_remove(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
	gc::Local<gc::Array<Value>> api_to_array(const gc::Local<ExecContext>& context);

public:
	class API;
};

//
//HashSetValue::API
//

class rt::HashSetValue::API : public SysAPI<HashSetValue> {
	void init() override {
		bld->add_constructor(api_create);
		bld->add_method("is_empty", &HashSetValue::api_is_empty);
		bld->add_method("size", &HashSetValue::api_size);
		bld->add_method("clear", &HashSetValue::api_clear);
		bld->add_method("contains", &HashSetValue::api_contains);
		bld->add_method("add", &HashSetValue::api_add);
		bld->add_method("remove", &HashSetValue::api_remove);
		bld->add_method("to_array", &HashSetValue::api_to_array);
	}
};

//
//HashSetValue : implementation
//

void rt::HashSetValue::gc_enumerate_refs() {
	SysObjectValue::gc_enumerate_refs();
	gc_ref(m_map);
}

void rt::HashSetValue::initialize() {
	m_map = gc::create<ValueHashMap>();
}

ss::StringLoc rt::HashSetValue::to_string(const gc::Local<ExecContext>& context) const {
	std::ostringstream out;
	out << "[";
	const char* sep = "";
	ValueHashMap::Iterator iter = m_map->iterator();
	while (!iter.end()) {
		out << sep << iter.key()->to_string(context);
		sep = ", ";
		iter.next();
	}
	out << "]";
	return gc::create<String>(out.str());
}

bool rt::HashSetValue::iterate(InternalValueIterator& iterator) {
	ValueHashMap::Iterator iter = m_map->iterator();
	while (!iter.end()) {
		if (!iterator.iterate(iter.key())) return false;
		iter.next();
	}
	return true;
}

std::size_t rt::HashSetValue::get_sys_class_id() const {
	return API::get_class_id();
}

gc::Local<rt::HashSetValue> rt::HashSetValue::api_create(const gc::Local<ExecContext>& context) {
	return gc::create<HashSetValue>();
}

bool rt::HashSetValue::api_is_empty(const gc::Local<ExecContext>& context) {
	return m_map->is_empty();
}

ss::ScriptIntegerType rt::HashSetValue::api_size(const gc::Local<ExecContext>& context) {
	return size_to_scriptint_ex(m_map->size());
}

void rt::HashSetValue::api_clear(const gc::Local<ExecContext>& context) {
	m_map->clear();
}

bool rt::HashSetValue::api_contains(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	assert(!!value);
	return m_map->contains(value);
}

bool rt::HashSetValue::api_add(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	assert(!!value);
	gc::Local<Value> old_value = m_map->put(value, value);
	return !old_value;
}

bool rt::HashSetValue::api_remove(const gc::Local<ExecContext>& context, const gc::Local<Value>& value) {
	gc::Local<Value> old_value = m_map->remove(value);
	return !!old_value;
}

gc::Local<gc::Array<rt::Value>> rt::HashSetValue::api_to_array(const gc::Local<ExecContext>& context) {
	return m_map->keys();
}

//
//HashMapValue::API
//

class rt::HashMapValue::API : public SysAPI<HashMapValue> {
	void init() override {
		bld->add_constructor(api_create);
		bld->add_method("is_empty", &HashMapValue::api_is_empty);
		bld->add_method("size", &HashMapValue::api_size);
		bld->add_method("clear", &HashMapValue::api_clear);
		bld->add_method("contains", &HashMapValue::api_contains);
		bld->add_method("get", &HashMapValue::api_get);
		bld->add_method("put", &HashMapValue::api_put);
		bld->add_method("remove", &HashMapValue::api_remove);
		bld->add_method("keys", &HashMapValue::api_keys);
		bld->add_method("values", &HashMapValue::api_values);
	}
};

//
//HashMapValue
//

void rt::HashMapValue::gc_enumerate_refs() {
	SysObjectValue::gc_enumerate_refs();
	gc_ref(m_map);
}

void rt::HashMapValue::initialize() {
	m_map = gc::create<ValueHashMap>();
}

const gc::Ref<rt::ValueHashMap>& rt::HashMapValue::get_map() const {
	return m_map;
}

std::size_t rt::HashMapValue::get_sys_class_id() const {
	return API::get_class_id();
}

void rt::HashMapValue::check_key(const gc::Local<Value>& key) const {
	if (!key) throw RuntimeError("key == null");
}

gc::Local<rt::Value> rt::HashMapValue::external_value(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& value) const
{
	return !!value ? value : context->get_value_factory()->get_null_value();
}

gc::Local<rt::HashMapValue> rt::HashMapValue::api_create(const gc::Local<ExecContext>& context) {
	return gc::create<HashMapValue>();
}

bool rt::HashMapValue::api_is_empty(const gc::Local<ExecContext>& context) {
	return m_map->is_empty();
}

ss::ScriptIntegerType rt::HashMapValue::api_size(const gc::Local<ExecContext>& context) {
	return size_to_scriptint_ex(m_map->size());
}

void rt::HashMapValue::api_clear(const gc::Local<ExecContext>& context) {
	m_map->clear();
}

bool rt::HashMapValue::api_contains(const gc::Local<ExecContext>& context, const gc::Local<Value>& key) {
	check_key(key);
	return m_map->contains(key);
}

gc::Local<rt::Value> rt::HashMapValue::api_get(const gc::Local<ExecContext>& context, const gc::Local<Value>& key) {
	check_key(key);
	return external_value(context, m_map->get(key));
}

gc::Local<rt::Value> rt::HashMapValue::api_remove(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& key)
{
	check_key(key);
	return external_value(context, m_map->remove(key));
}

gc::Local<gc::Array<rt::Value>> rt::HashMapValue::api_keys(const gc::Local<ExecContext>& context) {
	return m_map->keys();
}

gc::Local<gc::Array<rt::Value>> rt::HashMapValue::api_values(const gc::Local<ExecContext>& context) {
	return m_map->values();
}

gc::Local<rt::Value> rt::HashMapValue::api_put(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& key,
	const gc::Local<Value>& value)
{
	check_key(key);
	return external_value(context, m_map->put(key, value));
}

//
//(Functions)
//

namespace {
	rt::SysNamespaceInitializer s_sys_namespace_initializer([](rt::SysClassBuilder<rt::SysNamespaceValue>& bld){
		bld.add_class<rt::ArrayListValue>("ArrayList");
		bld.add_class<rt::HashSetValue>("HashSet");
		bld.add_class<rt::HashMapValue>("HashMap");
	});
}

void link__api_collections(){}
