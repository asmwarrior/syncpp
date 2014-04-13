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

//GC String class implementation.

#include <cstring>
#include <string>

#include "common.h"
#include "stringex.h"

namespace ss = syn_script;
namespace gc = ss::gc;

typedef gc::PrimitiveArray<char> CharArray;

namespace {
	template<class A, class B>
	int compare_string_keys(const A& a, const B& b) {
		std::size_t len = a.length();
		std::size_t len2 = b.length();
		if (len < len2) {
			return -1;
		} else if (len > len2) {
			return 1;
		}

		for (std::size_t i = 0; i < len; ++i) {
			char c1 = a.char_at(i);
			char c2 = b.char_at(i);
			if (c1 < c2) {
				return -1;
			} else if (c1 > c2) {
				return 1;
			}
		}

		return 0;
	}
}

//
//StringIterator
//

const std::string ss::StringIterator::EMPTY;

//
//String
//

ss::String::String() : m_hash(0){}

void ss::String::gc_enumerate_refs() {
	gc_ref(m_value);
}

void ss::String::initialize(const char* str) {
	initialize(str, std::strlen(str));
}

void ss::String::initialize(const char* str, std::size_t len) {
	//TODO Do not create an array if the len is 0.
	m_value = CharArray::create(len);
	if (len) m_value->set(0, str, 0, len);
}

void ss::String::initialize(const std::string& str) {
	initialize(str, 0, str.length());
}

void ss::String::initialize(const std::string& str, std::size_t start_pos, std::size_t end_pos) {
	std::size_t len = end_pos - start_pos;
	m_value = CharArray::create(len);
	for (std::size_t i = 0; i < len; ++i) m_value->get(i) = str[start_pos + i];
}

void ss::String::initialize(const StringLoc& str, std::size_t start_pos, std::size_t end_pos) {
	assert(start_pos <= end_pos);
	std::size_t len = end_pos - start_pos;
	m_value = CharArray::create(len);
	if (start_pos < end_pos) m_value->set(0, str->m_value, start_pos, end_pos);
}

void ss::String::initialize(const StringLoc& a, const StringLoc& b) {
	std::size_t len1 = a->length();
	std::size_t len2 = b->length();

	m_value = CharArray::create(len1 + len2);
	m_value->set(0, a->m_value, 0, len1);
	m_value->set(len1, b->m_value, 0, len2);
}

bool ss::String::is_empty() const {
	return !m_value->length();
}

std::size_t ss::String::length() const {
	return m_value->length();
}

char ss::String::char_at(std::size_t index) const {
	assert(index < length());
	return m_value->get(index);
}

ss::StringLoc ss::String::substring(std::size_t start) const {
	return substring(start, length());
}

ss::StringLoc ss::String::substring(std::size_t start, std::size_t end) const {
	assert(start <= end);
	if (start == 0 && end == length()) return self(this);
	return gc::create<String>(self(this), start, end);
}

std::unique_ptr<char[]> ss::String::get_c_string() const {
	std::size_t len = length();
	const char* raw_array = m_value->raw_array();

	std::unique_ptr<char[]> data(new char[len + 1]);
	char* raw_data = data.get();
	for (std::size_t i = 0; i < len; ++i) raw_data[i] = raw_array[i];
	raw_data[len] = 0;

	return data;
}

std::string ss::String::get_std_string() const {
	std::size_t len = length();
	const char* raw_array = m_value->raw_array();
	return std::string(raw_array, len);
}

void ss::String::get_std_string(std::size_t start_ofs, std::size_t end_ofs, std::string& str) const {
	assert(start_ofs <= end_ofs);
	assert(end_ofs <= length());

	const char* raw_array = m_value->raw_array();
	str.assign(raw_array + start_ofs, end_ofs - start_ofs);
}

gc::Local<ss::ByteArray> ss::String::get_bytes() const {
	std::size_t len = length();
	//TODO Do not create an empty array.
	gc::Local<ByteArray> array = ByteArray::create(len);
	const char* src = m_value->raw_array();
	char* dst = array->raw_array();
	while (len--) *dst++ = *src++;
	return array;
}

const char* ss::String::get_raw_data() const {
	return m_value->raw_array();
}

int ss::String::compare_to(const StringLoc& str) const {
	return compare(self(this), str);
}

bool ss::String::equals(const gc::Local<const ObjectEx>& obj) const {
	const ObjectEx* const ptr = obj.get();
	if (this == ptr) return true;
	
	const String* const str = dynamic_cast<const String*>(ptr);
	if (!str) return false;

	const CharArray* const value1 = m_value.get();
	const CharArray* const value2 = str->m_value.get();

	const std::size_t len = value1->length();
	if (len != value2->length()) return false;

	const char* array1 = value1->raw_array();
	const char* array2 = value2->raw_array();
	for (std::size_t i = 0; i < len; ++i) {
		if (array1[i] != array2[i]) return false;
	}

	return true;
}

std::size_t ss::String::hash_code() const {
	std::size_t hash = m_hash.load(std::memory_order_relaxed);

	if (!hash) {
		const CharArray* const value = m_value.get();
		const std::size_t len = value->length();
		if (len) {
			for (std::size_t i = 0; i < len; ++i) {
				hash = hash * 31 + static_cast<std::size_t>(value->get(i));
			}
			m_hash.store(hash, std::memory_order_relaxed);
		}
	}

	return hash;
}

int ss::String::compare(const gc::Local<const String>& str1, const gc::Local<const String>& str2) {
	std::size_t len1 = str1->length();
	std::size_t len2 = str2->length();
	std::size_t ofs = 0;

	const char* array1 = str1->m_value->raw_array();
	const char* array2 = str2->m_value->raw_array();

	for (;;) {
		if (ofs >= len1) return ofs >= len2 ? 0 : -1;
		if (ofs >= len2) return 1;

		char c1 = array1[ofs];
		char c2 = array2[ofs];
		unsigned char uc1 = reinterpret_cast<unsigned char&>(c1);
		unsigned char uc2 = reinterpret_cast<unsigned char&>(c2);

		if (uc1 < uc2) return -1;
		if (uc1 > uc2) return 1;

		++ofs;
	}
}

//
//String : operators
//

ss::StringLoc ss::operator+(const StringLoc& a, const StringLoc& b) {
	if (b->is_empty()) return a;
	if (a->is_empty()) return b;
	return gc::create<String>(a, b);
}

std::ostream& ss::operator<<(std::ostream& out, const ss::StringLoc& str) {
	std::ostream::sentry s(out);
	const CharArray* value = str->m_value.get();
	const std::size_t len = value->length();
	const char* data = value->raw_array();
	out.rdbuf()->sputn(data, len);
	return out;
}

//
//StringKey
//

const ss::StringLoc& ss::StringKey::get_gc_string() const {
	throw std::logic_error("illegal state");
}

//
//StringKey : operators
//

bool ss::operator==(const StringKey& a, const StringKey& b) {
	return a.compare_to(b) == 0;
}

bool ss::operator<(const StringKey& a, const StringKey& b) {
	return a.compare_to(b) < 0;
}

//
//StdStringKey
//

ss::StdStringKey::StdStringKey(const std::string& str)
: m_str(&str),
m_start_ofs(0),
m_end_ofs(str.length())
{}

ss::StdStringKey::StdStringKey(const std::string& str, std::size_t start_ofs, std::size_t end_ofs)
: m_str(&str),
m_start_ofs(start_ofs),
m_end_ofs(end_ofs)
{
	assert(start_ofs <= end_ofs);
	assert(end_ofs <= str.length());
}

int ss::StdStringKey::compare_to_std(const StdStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::StdStringKey::compare_to_gc(const GCStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::StdStringKey::compare_to_gc_ptr(const GCPtrStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::StdStringKey::compare_to(const StringKey& key) const {
	return -key.compare_to_std(*this);
}

const ss::StringLoc& ss::StdStringKey::get_gc_string() const {
	assert(false);
	throw "Illegal state!";
}

//
//GCStringKey
//

ss::GCStringKey::GCStringKey(const StringLoc& str)
: GCStringKey(str, 0, str->length())
{}

ss::GCStringKey::GCStringKey(const StringLoc& str, std::size_t start_ofs, std::size_t end_ofs)
: m_str(str), m_start_ofs(start_ofs), m_end_ofs(end_ofs)
{
	assert(!!str);
	assert(start_ofs <= end_ofs);
	assert(end_ofs <= str->length());
}

int ss::GCStringKey::compare_to_std(const StdStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::GCStringKey::compare_to_gc(const GCStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::GCStringKey::compare_to_gc_ptr(const GCPtrStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::GCStringKey::compare_to(const StringKey& key) const {
	return -key.compare_to_gc(*this);
}

const ss::StringLoc& ss::GCStringKey::get_gc_string() const {
	if (m_start_ofs != 0 || m_end_ofs != m_str->length()) throw std::logic_error("illegal state");
	return m_str;
}

//
//GCPtrStringKey
//

ss::GCPtrStringKey::GCPtrStringKey(const String* str)
: GCPtrStringKey(str, 0, str->length())
{}

ss::GCPtrStringKey::GCPtrStringKey(const String* str, std::size_t start_ofs, std::size_t end_ofs)
: m_str(str), m_start_ofs(start_ofs), m_end_ofs(end_ofs)
{
	assert(str);
	assert(start_ofs <= end_ofs);
	assert(end_ofs <= str->length());
}

int ss::GCPtrStringKey::compare_to_std(const StdStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::GCPtrStringKey::compare_to_gc(const GCStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::GCPtrStringKey::compare_to_gc_ptr(const GCPtrStringKey& key) const {
	return compare_string_keys(*this, key);
}

int ss::GCPtrStringKey::compare_to(const StringKey& key) const {
	return -key.compare_to_gc_ptr(*this);
}

//
//StringKeyValue
//

ss::StringKeyValue::StringKeyValue(const StringKey& key) : m_key(&key){}

const ss::StringKey& ss::StringKeyValue::get_key() const {
	return *m_key;
}

bool ss::operator==(const StringKeyValue& a, const StringKeyValue& b) {
	return a.get_key() == b.get_key();
}

bool ss::operator<(const StringKeyValue& a, const StringKeyValue& b) {
	return a.get_key() < b.get_key();
}
