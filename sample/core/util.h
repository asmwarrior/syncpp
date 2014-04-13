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

#ifndef SYNSAMPLE_CORE_UTIL_H_INCLUDED
#define SYNSAMPLE_CORE_UTIL_H_INCLUDED

#include <cassert>
#include <string>

namespace syn_script {
	namespace util {

		//
		//ScopeGuard
		//

		template<class T, void (T::*F)()>
		class ScopeGuard {
			NONCOPYABLE(ScopeGuard);

			T* const m_object;

		public:
			explicit ScopeGuard(T* object) : m_object(object){}
			~ScopeGuard(){ (m_object->*F)();  }
		};

		//
		//UnaryScopeGuard
		//

		template<class T, class A, void (T::*F)(A)>
		class UnaryScopeGuard {
			NONCOPYABLE(UnaryScopeGuard);

			T* const m_object;
			const A m_value;

		public:
			UnaryScopeGuard(T* object, const A& value) : m_object(object), m_value(value){}
			~UnaryScopeGuard(){ (m_object->*F)(m_value); }
		};

		//
		//BoolGuard
		//

		class BoolGuard {
			NONCOPYABLE(BoolGuard);

			bool* const m_variable;

		public:
			BoolGuard(bool* variable) : m_variable(variable){}
			~BoolGuard(){ *m_variable = false; }
		};

	}
}

#endif//SYNSAMPLE_CORE_UTIL_H_INCLUDED
