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

//GC Hashmap implementation.

#include <cassert>

#include "gc.h"
#include "gc_hashmap.h"

namespace ss = syn_script;
namespace gc = ss::gc;

//
//BasicHashMap::Entry
//

void gc::BasicHashMap::Entry::gc_enumerate_refs() {
	Object::gc_enumerate_refs();
	gc_ref(m_next);
	gc_ref(m_key);
	gc_ref(m_value);
}

void gc::BasicHashMap::Entry::initialize(
	const gc::Local<Entry>& next,
	const gc::Local<Object>& key,
	const gc::Local<Object>& value)
{
	m_next = next;
	m_key = key;
	m_value = value;
}

const gc::Ref<gc::BasicHashMap::Entry>& gc::BasicHashMap::Entry::get_next() const {
	return m_next;
}

const gc::Ref<gc::Object>& gc::BasicHashMap::Entry::get_key() const {
	return m_key;
}

const gc::Ref<gc::Object>& gc::BasicHashMap::Entry::get_value() const {
	return m_value;
}

void gc::BasicHashMap::Entry::set_next(const gc::Local<Entry>& next) {
	m_next = next;
}

void gc::BasicHashMap::Entry::set_value(const gc::Local<Object>& value) {
	m_value = value;
}

//
//BasicHashMap::BasicIterator
//

gc::BasicHashMap::BasicIterator::BasicIterator(const gc::Local<const BasicHashMap>& map)
: m_map(map)
{
	m_index = 0;
	next_index();
}

bool gc::BasicHashMap::BasicIterator::end() const {
	return !m_entry;
}

void gc::BasicHashMap::BasicIterator::next() {
	assert(!!m_entry);
	m_entry = m_entry->get_next();
	if (!m_entry) {
		++m_index;
		next_index();
	}
}

gc::Local<gc::Object> gc::BasicHashMap::BasicIterator::key() const {
	assert(!!m_entry);
	return m_entry->get_key();
}

gc::Local<gc::Object> gc::BasicHashMap::BasicIterator::value() const {
	assert(!!m_entry);
	return m_entry->get_value();
}

void gc::BasicHashMap::BasicIterator::next_index() {
	std::size_t capacity = m_map->m_table->length();
	while (m_index < capacity) {
		m_entry = (*m_map->m_table)[m_index];
		if (!!m_entry) break;
		++m_index;
	}
}

//
//BasicHashMap
//

const float gc::BasicHashMap::LOAD_FACTOR = 0.75f;

void gc::BasicHashMap::gc_enumerate_refs() {
	Object::gc_enumerate_refs();
	gc_ref(m_table);
}

void gc::BasicHashMap::initialize() {
	m_threshold = 16;
	m_table = gc::Array<Entry>::create(m_threshold);
	m_size = 0;
}

bool gc::BasicHashMap::is_empty() const {
	return 0 == m_size;
}

std::size_t gc::BasicHashMap::size() const {
	return m_size;
}

void gc::BasicHashMap::clear() {
	for (std::size_t i = 0, len = m_table->length(); i < len; ++i) (*m_table)[i] = nullptr;
	m_size = 0;
}

gc::BasicHashMap::BasicIterator gc::BasicHashMap::basic_iterator() const {
	return BasicIterator(self(this));
}

bool gc::BasicHashMap::contains_0(const gc::Local<Object>& key) const {
	gc::Local<Entry> prev;
	std::size_t index;
	gc::Local<Entry> entry = find_entry(key, prev, index);
	return !!entry;
}

gc::Local<gc::Object> gc::BasicHashMap::get_0(const gc::Local<Object>& key) const {
	gc::Local<Entry> prev;
	std::size_t index;
	gc::Local<Entry> entry = find_entry(key, prev, index);
	if (!entry) return nullptr;
	return entry->get_value();
}

gc::Local<gc::Object> gc::BasicHashMap::put_0(const gc::Local<Object>& key, const gc::Local<Object>& value) {
	gc::Local<Entry> prev;
	std::size_t index;
	gc::Local<Entry> entry = find_entry(key, prev, index);
	if (!!entry) {
		gc::Local<Object> old_value = entry->get_value();
		entry->set_value(value);
		return old_value;
	} else {
		gc::Ref<Entry>& table_entry_ref = (*m_table)[index];
		gc::Local<Entry> table_entry = table_entry_ref;
		table_entry_ref = gc::create<Entry>(table_entry, key, value);

		++m_size;
		if (m_size >= m_threshold) expand();

		return nullptr;
	}
}

gc::Local<gc::Object> gc::BasicHashMap::remove_0(const gc::Local<Object>& key) {
	gc::Local<Entry> prev;
	std::size_t index;
	gc::Local<Entry> entry = find_entry(key, prev, index);
	if (!entry) return nullptr;

	if (!!prev) {
		prev->set_next(entry->get_next());
	} else {
		(*m_table)[index] = entry->get_next();
	}
	--m_size;

	return entry->get_value();
}

void gc::BasicHashMap::expand() {
	std::size_t capacity = m_table->length();
	std::size_t new_capacity = capacity << 1;
	if (new_capacity <= capacity || new_capacity >= SIZE_MAX) return;

	gc::Local<gc::Array<Entry>> new_table = gc::Array<Entry>::create(new_capacity);
	for (std::size_t i = 0; i < capacity; ++i) {
		gc::Local<Entry> entry = (*m_table)[i];
		while (!!entry) {
			std::size_t hash = key_hash_code(entry->get_key());
			std::size_t new_index = hash & (new_capacity - 1);

			gc::Local<Entry> next = entry->get_next();
			gc::Ref<Entry>& new_table_entry = (*new_table)[new_index];
			entry->set_next(new_table_entry);
			new_table_entry = entry;
			entry = next;
		}
	}

	m_table = new_table;

	std::size_t new_threshold = (std::size_t)(LOAD_FACTOR * new_capacity);
	//Overflow check.
	if (new_threshold > m_threshold) m_threshold = new_threshold;
}

gc::Local<gc::BasicHashMap::Entry> gc::BasicHashMap::find_entry(
	const gc::Local<Object>& key,
	gc::Local<Entry>& prev_ref,
	std::size_t& index_ref) const
{
	std::size_t hash = key_hash_code(key);
	std::size_t capacity = m_table->length();
	std::size_t index = hash & (capacity - 1);
	index_ref = index;

	gc::Local<Entry> prev;
	gc::Local<Entry> entry = (*m_table)[index];

	while (!!entry) {
		if (key_equals(key, entry->get_key())) {
			prev_ref = prev;
			return entry;
		}
		prev = entry;
		entry = entry->get_next();
	}

	return nullptr;
}
