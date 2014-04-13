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

//Fast and simple reference-counting smart pointer.

#ifndef SYN_CORE_CNTPTR_H_INCLUDED
#define SYN_CORE_CNTPTR_H_INCLUDED

#include <cstddef>
#include <iostream>

#include "noncopyable.h"

namespace synbin {

	template<class T>
	class CntPtr;

	//
	//RefCnt
	//

	//Contains the reference counter. Any class must derive from RefCnt in order to be used with CntPtr.
	class RefCnt {
		NONCOPYABLE(RefCnt);

		template<class T> friend class CntPtr;

		mutable std::size_t m_ref_cnt;

	protected:
		RefCnt() : m_ref_cnt(0){}
	};

	//
	//CntPtr
	//

	//Reference-counting pointer.
	//1. Any class used with this pointer must be derived from RefCnt.
	//2. Not thread-safe. To be used in a single thread code.
	template<class T>
	class CntPtr {
		template<class P> friend class CntPtr;

		T* m_object;

		inline const RefCnt* chktype(){ return m_object; }

		inline void add_object() { if (m_object) ++m_object->m_ref_cnt;	}

		inline void remove_object() {
			if (m_object) {
				if (!--m_object->m_ref_cnt) delete m_object;
			}
		}

		inline void change_object(T* object) {
			remove_object();
			m_object = object;
			add_object();
		}

	public:
		CntPtr(const CntPtr& ptr) : m_object(ptr.m_object){ chktype(); add_object(); }
		CntPtr(CntPtr&& ptr) : m_object(ptr.m_object){ ptr.m_object = nullptr; }

		CntPtr() : m_object(nullptr){}
		CntPtr(T* object) : m_object(object){ chktype(); add_object(); }
		template<class P> CntPtr(const CntPtr<P>& ptr) : m_object(ptr.m_object){ chktype(); add_object(); }
		template<class P> CntPtr(CntPtr<P>&& ptr) : m_object(ptr.m_object){ ptr.m_object = nullptr; }

		~CntPtr(){ remove_object(); }

		CntPtr& operator=(const CntPtr& ptr) { change_object(ptr.m_object); return *this; }
		CntPtr& operator=(CntPtr&& ptr) { remove_object(); m_object = ptr.m_object; ptr.m_object = nullptr; return *this; }

		CntPtr& operator=(T* object) { change_object(object); return *this; }
		template<class P> CntPtr& operator=(const CntPtr<P>& ptr) { change_object(ptr.m_object); return *this; }
		template<class P> CntPtr& operator=(CntPtr<P>&& ptr) { m_object = ptr.m_object;  ptr.m_object = nullptr; return *this; }
		
		T* get() const { return m_object; }
		T* operator->() const { return m_object; }
		T& operator*() const { return *m_object; }
		bool operator!() const { return !m_object; }
	};

}

#endif//SYN_CORE_CNTPTR_H_INCLUDED
