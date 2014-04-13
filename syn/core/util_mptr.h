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

//MPtr - managed pointer.

#ifndef SYN_CORE_UTIL_MPTR_H_INCLUDED
#define SYN_CORE_UTIL_MPTR_H_INCLUDED

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "cntptr.h"
#include "commons.h"
#include "noncopyable.h"
#include "util.h"

namespace synbin { namespace util {

	template<class T>
	class MContainer;

	class MHeap;

	//
	//MPtr
	//

	//Managed pointer. Wraps a raw pointer.
	//Used when large collections of objects have to be deleted at once (like nodes of an Abstract Syntax Tree).
	//Any object pointed to by MPtr is owned by a container (represented by MHeap and MContainer classes).
	//When the container is destroyed, all its objects are deleted.
	template<class T>
	class MPtr {
		friend class MContainer<T>;
		friend class MHeap;
		
		T* m_value;
		
		//This constructor is private. It can be used directly only by friend M-classes.
		//Users should use unsafe_cast function.
		explicit MPtr(T* value) : m_value(value){}

	public:
		MPtr() : m_value(nullptr){}
		MPtr(std::nullptr_t value) : m_value(nullptr){}

		template<class P>
		MPtr(const MPtr<P>& ptr) : m_value(ptr.get()){}

		template<class P>
		MPtr<T>& operator=(const MPtr<P>& ptr) {
			m_value = ptr.get();
			return *this;
		}
		
		T& operator*() const { return *m_value; }
		T* operator->() const { return m_value; }
		T* get() const { return m_value; }
		bool operator!() const { return !m_value; }

		//Make an MPtr from a raw pointer. Calls of this function will be easier to find than calls of
		//a constructor (if it was public).
		static MPtr<T> unsafe_cast(T* value) { return MPtr<T>(value); }
	};

	//
	//MContainer
	//

	//Container of managed objects. Types of objects have to be derived from the type of template argument T.
	//When a container is destroyed, all its objects are deleted.
	template<class T>
	class MContainer {
		NONCOPYABLE(MContainer);

		friend class MHeap;

		static const int PAGE_SIZE = 256;

		struct MPage : public RefCnt {
			T* m_objects[PAGE_SIZE];

			void free(std::size_t cnt) {
				for (std::size_t i = 0; i < cnt; ++i) delete m_objects[i];
			}
		};

		bool m_in_heap;
		std::vector<CntPtr<MPage>> m_pages;
		std::size_t m_last_page_cnt;

	public:
		MContainer() : m_in_heap(false), m_last_page_cnt(PAGE_SIZE){}

		~MContainer() {
			if (!m_pages.empty()) {
				for (std::size_t i = 0, n = m_pages.size(); i < n - 1; ++i) m_pages[i]->free(PAGE_SIZE);
				m_pages.back()->free(m_last_page_cnt);
			}
		}

		//Adds the passed object to the container, returns MPtr to it.
		template<class P>
		MPtr<P> add(P* object) {
			T* base_object = object;

			if (m_last_page_cnt >= PAGE_SIZE) {
				m_pages.push_back(CntPtr<MPage>(new MPage()));
				m_last_page_cnt = 0;
			}
			m_pages.back()->m_objects[m_last_page_cnt++] = base_object;

			return MPtr<P>::unsafe_cast(object);
		}
	};

	//
	//MHeap
	//

	//Managed heap. A collection of arbitrary managed objects and containers.
	//When a heap is destroyed, all its objects are deleted.
	class MHeap {
		NONCOPYABLE(MHeap);

		typedef void (*Destroyer)(void*);

		template<class T>
		static void destroy(void* data) {
			T* value = static_cast<T*>(data);
			delete value;
		}

		struct MEntry {
			void* m_data;
			Destroyer m_destroyer;

			MEntry(void* data, Destroyer destroyer) : m_data(data), m_destroyer(destroyer){}

			void destroy() {
				m_destroyer(m_data);
				m_data = nullptr;
			}
		};

		std::vector<MEntry> m_entries;

		template<class T>
		MPtr<T> add_object0(std::unique_ptr<T> ptr) {
			Destroyer destroyer = &destroy<T>;
			T* object = ptr.get();
			assert(object);
			const void* object_ptr = object;

			//const_cast can be used here, because the pointer is got from an unique_ptr, meaning that
			//we own the object.
			m_entries.push_back(MEntry(const_cast<void*>(object_ptr), destroyer));
			ptr.release();
			return MPtr<T>(object);
		}

	public:
		MHeap(){}

		~MHeap() {
			for (MEntry& entry : m_entries) entry.destroy();
		}

		template<class T>
		MPtr<T> add_object(std::unique_ptr<T> ptr) {
			if (ptr.get()) return add_object0(std::move(ptr));
			return MPtr<T>();
		}

		template<class T>
		MPtr<T> add_object(T* object) {
			if (object) return add_object0(std::unique_ptr<T>(object));
			return MPtr<T>();
		}

		template<class T>
		MPtr<MContainer<T>> add_container(std::unique_ptr<MContainer<T>> container_ptr) {
			assert(!container_ptr->m_in_heap);
			MPtr<MContainer<T>> container = add_object0(std::move(container_ptr));
			container->m_in_heap = true;
			return container;
		}

		template<class T>
		MPtr<MContainer<T>> create_container() {
			return add_container(make_unique1<MContainer<T>>());
		}

		MPtr<MHeap> add_heap(std::unique_ptr<MHeap> heap) {
			assert(heap.get());
			return add_object0(std::move(heap));
		}
	};

	//
	//MRoot
	//

	//Managed root. Same as MHeap, but holds a reference to the 'root' object.
	//Useful for owning all nodes of an Abstract Syntax Tree, while holding an additional pointer pointer
	//to the root node.
	template<class S>
	class MRoot : public MHeap {
		NONCOPYABLE(MRoot);

		const MPtr<S> m_value;

	public:
		MRoot(MPtr<S> value) : m_value(value){}

		MPtr<S> ptr() const { return m_value; }
		S* get() const { return m_value.get(); }

		S& operator*() const { return *m_value; }
		S* operator->() const { return m_value.get(); }
	};

}}


#endif//SYN_CORE_UTIL_MPTR_H_INCLUDED
