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

//GC Vector.

#ifndef SYNSAMPLE_CORE_GC_VECTOR_H_INCLUDED
#define SYNSAMPLE_CORE_GC_VECTOR_H_INCLUDED

#include "common.h"
#include "gc.h"

namespace syn_script {
	namespace gc {
		namespace internal {
			class InternalVector;
		}
	}

	namespace gc {
		template<class T> class Vector;
	}
}

//
//InternalVector
//

class syn_script::gc::internal::InternalVector : public gc::Object {
	NONCOPYABLE(InternalVector);

	gc::Ref<gc::Array<gc::Object>> m_array;
	std::size_t m_size;

protected:
	InternalVector(){}
	void gc_enumerate_refs() override;
	void initialize();

public:
	std::size_t size() const;

protected:
	void internal_add(const gc::Local<gc::Object>& object);
	gc::Local<gc::Object> internal_get(std::size_t index) const;
};

//
//Vector
//

template<class T>
class syn_script::gc::Vector : public internal::InternalVector {
	NONCOPYABLE(Vector);

public:
	Vector(){}
	void initialize() { InternalVector::initialize(); }

	void add(const gc::Local<T>& object) {
		internal_add(object.template stat_cast_ex<gc::Object>());
	}

	gc::Local<T> get(std::size_t index) const {
		return internal_get(index).template stat_cast<T>();
	}
};

#endif//SYNSAMPLE_CORE_GC_VECTOR_H_INCLUDED
