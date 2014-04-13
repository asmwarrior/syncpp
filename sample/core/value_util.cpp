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

//Value-related utility functions.

#include <algorithm>
#include <cassert>
#include <iterator>
#include <sstream>

#include "basetype.h"
#include "common.h"
#include "gc.h"
#include "stringex.h"
#include "value.h"
#include "value_util.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;

//
//integer_to_string()
//

ss::StringLoc rt::integer_to_string(
	const gc::Local<ExecContext>& context,
	ss::ScriptIntegerType v)
{
	std::ostringstream s;
	int sign = scriptint_sign(v);
	if (sign < 0) {
		s << '-';
		v = scriptint_neg(v);
	}
	s << v;

	return gc::create<String>(s.str());
}

//
//float_to_string()
//

ss::StringLoc rt::float_to_string(
	const gc::Local<ExecContext>& context,
	ss::ScriptFloatType v)
{
	std::ostringstream s;
	s << v;
	return gc::create<String>(s.str());
}

//
//array_to_string()
//

ss::StringLoc rt::array_to_string(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& array,
	std::size_t start,
	std::size_t end)
{
	std::ostringstream out;
	out << "[";
	const char* sep = "";
	for (std::size_t i = start; i < end; ++i) {
		out << sep;
		//TODO Optimize: pass output stream to to_string() of elements.
		out << (*array)[i]->to_string(context);
		sep = ", ";
	}
	out << "]";
	return gc::create<String>(out.str());
}

//
//array_to_sort()
//

namespace {

	//
	//ArrayIterator
	//

	class ArrayIterator : public std::iterator<
		std::random_access_iterator_tag,
		gc::Local<rt::Value>,
		std::ptrdiff_t,
		gc::Ref<rt::Value>*,
		gc::Ref<rt::Value>&>
	{
		gc::Array<rt::Value>* m_array;
		std::size_t m_offset;

	public:
		ArrayIterator(gc::Array<rt::Value>* array, std::size_t offset)
			: m_array(array), m_offset(offset)
		{}

		bool operator==(const ArrayIterator& iter) const {
			assert(m_array == iter.m_array);
			return m_offset == iter.m_offset;
		}

		bool operator!=(const ArrayIterator& iter) const {
			return !operator==(iter);
		}

		bool operator<(const ArrayIterator& iter) const {
			assert(m_array == iter.m_array);
			return m_offset < iter.m_offset;
		}

		bool operator>(const ArrayIterator& iter) const {
			assert(m_array == iter.m_array);
			return m_offset > iter.m_offset;
		}

		bool operator<=(const ArrayIterator& iter) const {
			assert(m_array == iter.m_array);
			return m_offset <= iter.m_offset;
		}

		bool operator>=(const ArrayIterator& iter) const {
			assert(m_array == iter.m_array);
			return m_offset >= iter.m_offset;
		}

		gc::Ref<rt::Value>& operator*() const {
			assert(m_offset < m_array->length());
			return (*m_array)[m_offset];
		}

		gc::Ref<rt::Value>& operator[](std::size_t index) const {
			assert(index < m_array->length() - m_offset);
			return (*m_array)[m_offset + index];
		}

		ArrayIterator& operator++() {
			assert(m_offset < m_array->length());
			++m_offset;
			return *this;
		}

		ArrayIterator& operator--() {
			assert(m_offset > 0);
			--m_offset;
			return *this;
		}

		ArrayIterator operator++(int) {
			assert(m_offset < m_array->length());
			ArrayIterator iter(m_array, m_offset);
			++m_offset;
			return iter;
		}

		ArrayIterator operator--(int) {
			assert(m_offset > 0);
			ArrayIterator iter(m_array, m_offset);
			--m_offset;
			return iter;
		}

		ArrayIterator operator+(std::ptrdiff_t n) const {
			//TODO Check bounds.
			return ArrayIterator(m_array, m_offset + n);
		}

		ArrayIterator operator-(std::ptrdiff_t n) const {
			//TODO Check bounds.
			return ArrayIterator(m_array, m_offset - n);
		}

		std::ptrdiff_t operator-(const ArrayIterator& iter) const {
			assert(m_array == iter.m_array);
			//TODO Check bounds.
			return m_offset - iter.m_offset;
		}

		const ArrayIterator& operator+=(std::ptrdiff_t n) {
			//TODO Check bounds.
			m_offset += n;
			return *this;
		}

		const ArrayIterator& operator-=(std::ptrdiff_t n) {
			//TODO Check bounds.
			m_offset -= n;
			return *this;
		}
	};

	ArrayIterator operator+(std::ptrdiff_t n, const ArrayIterator& iter) {
		return iter + n;
	}

	bool compare_values(const gc::Local<rt::Value>& a, const gc::Local<rt::Value>& b) {
		int d = a->value_compare_to(b);
		return d < 0;
	}
}

void rt::array_sort(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<Value>>& array,
	std::size_t start,
	std::size_t end)
{
	ArrayIterator start_iter(array.get(), start);
	ArrayIterator end_iter(array.get(), end);
	std::sort(start_iter, end_iter, compare_values);
}
