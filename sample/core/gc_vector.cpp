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

//GC Vector implementation.

#include <cassert>

#include "gc.h"
#include "gc_vector.h"

namespace ss = syn_script;
namespace gc = ss::gc;

//
//InternalVector
//

void gc::internal::InternalVector::gc_enumerate_refs() {
	Object::gc_enumerate_refs();
	gc_ref(m_array);
}

void gc::internal::InternalVector::initialize() {
	m_array = gc::Array<gc::Object>::create(10);
	m_size = 0;
}

std::size_t gc::internal::InternalVector::size() const {
	return m_size;
}

void gc::internal::InternalVector::internal_add(const gc::Local<gc::Object>& object) {
	std::size_t length = m_array->length();
	if (m_size >= length) {
		std::size_t delta_length = length >> 1;
		std::size_t new_length = (length <= SIZE_MAX - delta_length) ? length + delta_length : SIZE_MAX;
		if (new_length <= length) throw std::runtime_error("size limit reached");

		gc::Local<gc::Array<gc::Object>> new_array = gc::Array<gc::Object>::create(new_length);
		for (std::size_t i = 0; i < m_size; ++i) (*new_array)[i] = (*m_array)[i];
		m_array = new_array;
	}
	(*m_array)[m_size++] = object;
}

gc::Local<gc::Object> gc::internal::InternalVector::internal_get(std::size_t index) const {
	if (index >= m_size) throw std::runtime_error("index out of range");
	return m_array->get(index);
}
