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

//Garbage Collector.

#ifndef SYNSAMPLE_CORE_GC_H_INCLUDED
#define SYNSAMPLE_CORE_GC_H_INCLUDED

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include "noncopyable.h"

/*
GUIDELINE

1. Every GC class MUST be derived from gc::Object.
2. Destructors of GC classes MUST NOT throw exceptions or use any GC functionality.
	It is recommended to not define destructors in GC classes.
3. Objects of GC classes MUST be referenced only by gc::Local or gc::Ref.
4. GC class MUST have only a default public constructor.
5. A constructor of a GC class MUST NOT use any GC functionality.
6. Objects of GC classes, except arrays, MUST be created only via gc::create().
7. Every reference from one GC class to another GC class MUST be implemented by gc::Ref.
8. A gc::Local object MUST be destroyed in the same thread where it was constructed.
9. Every GC class which has references to other GC classes MUST override function gc::Object::gc_enumerate_refs().
10. Every function which directly or indirectly overrides gc::Object::gc_enumerate_refs()
	MUST call the same function of the parent class at the beginning.
11. Function gc::startup() MUST be called once before using any GC functionality (inc. gc::enable()).
12. Function gc::enable() MUST be called before using any GC functionality in a thread.
13. Function gc::disable() MUST be called before long operations that do not involve GC functionality
	(such operations as network I/O, sleeping, etc).
14. A thread MUST NOT use any GC fucntionality in the timespan between gc::disable() and gc::enable() calls.
15. Every thread with enabled GC functionality MUST call gc:synchronize() periodically.
16. Arrays of GC objects may be implemented by gc::Array.
17. Primitive arrays may be implemented by gc::PrimitiveArray.
*/

namespace syn_script {
	namespace gc {

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// Declarations

		class Object;
		template<class T> class Local;
		template<class T> class Ref;
		template<class T> class BasicArray;
		template<class T> class Array;
		template<class T> class PrimitiveArray;

		namespace internal {
			extern const std::size_t MAX_SIZE;

			class GlobalState;
			class ThreadState;
			class ObjectListElement;
			class InternalLocal;
			class InternalRef;
			class AllocObserver;

			void* new_allocate(std::size_t size);
			void new_finish(Object* object, std::size_t size);
			void new_fail(void* ptr, std::size_t size);

			template<class T, class Fn, class... Args>
			Local<T> create_internal(std::size_t size, Fn init, Args... args);

			std::size_t calc_physical_block_size(std::size_t logical_size);
		}

		class out_of_memory;

		//Starts GC functionality for the entire process.
		void startup(std::size_t heap_size, internal::AllocObserver* observer = nullptr);

		//Stops GC functionality.
		void shutdown();

		//Enables GC functionality for the current thread.
		void enable();

		//Disables GC functionality for the thread. Then the thread cannot involve any GC functions,
		//while GC will not try to suspend the thread during garbage collection.
		void disable();

		//Collect the garbage now.
		void collect();

		//If garbage collection request is pending, this thread is suspended, allowing GC to proceed.
		void synchronize();

		template<class T, class... Args> Local<T> create(Args...);
		template<class T> void swap(gc::Ref<T>& a, gc::Ref<T>& b);

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// Definitions

		//
		//Object
		//

		class Object {
			NONCOPYABLE(Object);

			friend class internal::GlobalState;
			friend class internal::ThreadState;
			friend class internal::InternalLocal;
			friend class internal::InternalRef;
			template<class T> friend class Ref;
			template<class T> friend class BasicArray;

			template<class P, class Fn, class... Args>
			friend Local<P> internal::create_internal(std::size_t, Fn, Args...);

		private:
			std::size_t m_size_and_flags;
			Object* m_list_prev;
			Object* m_list_next;

		protected:
			Object();

		private:
			Object(bool gcstate);
			void initialize();

		protected:
			virtual ~Object();

			virtual void gc_enumerate_refs();
			template<class T> inline void gc_ref(Ref<T>& ref);
			template<class T> inline Local<T> self(T* this_ptr);
			template<class T> inline Local<const T> self(const T* this_ptr) const;

		private:
			void manage(std::size_t size);
			std::size_t get_size() const;
			bool is_mock() const;

			void gc_ref_internal(internal::InternalRef& ref);

			void list_add_to(Object& list_head);
			void list_remove_from();
			bool list_is_empty() const;
			void list_clear();
		};

		//
		//startup_guard
		//

		class startup_guard {
			NONCOPYABLE(startup_guard);

		public:
			startup_guard(
				std::size_t heap_size,
				internal::AllocObserver* observer = nullptr)
			{
				gc::startup(heap_size, observer);
			}

			~startup_guard() { gc::shutdown(); }
		};

		//
		//manage_thread_guard
		//

		class manage_thread_guard {
			NONCOPYABLE(manage_thread_guard);

			internal::ThreadState* m_thread_state;

		public:
			manage_thread_guard(const std::string& thread_name = std::string());
			~manage_thread_guard();
		};

		//
		//enable_guard
		//

		class enable_guard {
			NONCOPYABLE(enable_guard);

		public:
			enable_guard() { gc::enable(); }
			~enable_guard() { gc::disable(); }
		};

		//
		//disable_guard
		//

		class disable_guard {
			NONCOPYABLE(disable_guard);

		public:
			disable_guard() { gc::disable(); }
			~disable_guard() { gc::enable(); }
		};

		//
		//out_of_memory
		//

		class out_of_memory : public std::runtime_error {
		public:
			out_of_memory() : runtime_error("Out of memory"){}
		};

		//
		//ObjectListElement
		//

		class internal::ObjectListElement {
			NONCOPYABLE(ObjectListElement);

			friend class GlobalState;
			friend class ThreadState;
			friend class InternalLocal;

		private:
			ObjectListElement* m_prev;
			ObjectListElement* m_next;
			Object* m_object;

			ObjectListElement(Object* object);
			ObjectListElement(Object* object, ObjectListElement* list_head);

			void list_add_to(ObjectListElement* list_head);
			void list_remove_from();
			void list_clear();

			Object* get_object() const;
			void set_object(Object* object);
		};

		//
		//InternalLocal
		//

		class internal::InternalLocal {
			NONCOPYABLE(InternalLocal);

			template<class T> friend class gc::Local;

		private:
			ObjectListElement m_list_element;

			InternalLocal(const Object* object);

		protected:
			~InternalLocal();

			Object* internal_get_object() const;
			void internal_set_object(const Object* object);
		};

		//
		//InternalRef
		//

		class internal::InternalRef {
			NONCOPYABLE(InternalRef);

			template<class T> friend class gc::Ref;
			friend class GlobalState;
			friend class ThreadState;
			friend class gc::Object;

		private:
			gc::Object* m_object;

			InternalRef();

			Object* internal_get_object() const;
			void internal_set_object(const Object* object);
		};

		//
		//Local
		//

		template<class T> class Local : internal::InternalLocal {
			friend class gc::Object;
			template<class P> friend class Local;
			template<class P> friend class Ref;

			template<class P, class Fn, class... Args>
			friend Local<P> internal::create_internal(std::size_t, Fn, Args...);

			void set_object(T* object) { internal_set_object(object); }
			T* get_object() const { return static_cast<T*>(internal_get_object()); }
			inline T* check_type(T* object) { return object; }

		private:
			explicit Local(T* object) : InternalLocal(object){}

		public:
			Local() : InternalLocal(nullptr){}
			Local(std::nullptr_t) : Local(){}
			Local(const Local<T>& local) : InternalLocal(local.get_object()){}
			template<class P> Local(const Local<P>& local) : InternalLocal(check_type(local.get_object())){}
			template<class P> Local(const Ref<P>& ref) : InternalLocal(check_type(ref.get_object())){}

			template<class P> Local<P> cast() const {
				T* obj = get_object();
				if (!obj) return Local<P>(nullptr);
				P* p = dynamic_cast<P*>(obj);
				if (!p) throw std::runtime_error("Class cast error");
				return Local<P>(p);
			}

			template<class P> Local<P> cast_opt() const {
				T* obj = get_object();
				if (!obj) return Local<P>(nullptr);
				return Local<P>(dynamic_cast<P*>(obj));
			}

			template<class P> Local<P> stat_cast() const { return Local<P>(static_cast<P*>(get_object())); }

			template<class P> Local<P> stat_cast_ex() const {
				return Local<P>(const_cast<P*>(static_cast<const P*>(get_object())));
			}

		public:
			Local<T>& operator=(std::nullptr_t) {
				set_object(nullptr);
				return *this;
			}

			Local<T>& operator=(const Local<T>& local) {
				set_object(local.get_object());
				return *this;
			}
			
			template<class P> Local<T>& operator=(const Local<P>& local) {
				set_object(local.get_object());
				return *this;
			}

			template<class P> Local<T>& operator=(const Ref<P>& ref) {
				set_object(ref.get_object());
				return *this;
			}

			T& operator*() const { return *get_object(); }
			T* operator->() const { return get_object(); }
			//operator bool() const { return !!internal_get_object(); }
			bool operator!() const { return !internal_get_object(); }
			T* get() const { return get_object(); }

			template<class P> bool operator==(const Local<P>& local) {
				return get_object() == local.get_object();
			}

			template<class P> bool operator!=(const Local<P>& local) {
				return !operator==(local);
			}

			template<class P> bool operator==(const Ref<P>& ref) {
				return get_object() == ref.get_object();
			}

			template<class P> bool operator!=(const Ref<P>& ref) {
				return !operator==(ref);
			}
		};

		//
		//Ref
		//

		template<class T> class Ref : internal::InternalRef {
			Ref(const Ref<T>&) = delete;
			Ref(Ref<T>&&) = delete;

			friend class Object;
			template<class P> friend class Local;
			template<class P> friend class Ref;

		private:
			void set_object(T* object) { internal_set_object(object); }
			T* get_object() const { return static_cast<T*>(internal_get_object()); }

		public:
			Ref(){}

			Ref<T>& operator=(std::nullptr_t) {
				internal_set_object(nullptr);
				return *this;
			}

			Ref<T>& operator=(const Ref<T>& ref) {
				internal_set_object(ref.get_object());
				return *this;
			}

			template<class P> Ref<T>& operator=(const Ref<P>& ref) {
				set_object(ref.get_object());
				return *this;
			}

			template<class P> Ref<T>& operator=(const Local<P>& local) {
				set_object(local.get_object());
				return *this;
			}

			T& operator*() const { return *get_object(); }
			T* operator->() const { return get_object(); }
			//operator bool() const { return !!internal_get_object(); }
			bool operator!() const { return !internal_get_object(); }
			T* get() const { return get_object(); }

			template<class P> bool operator==(const Local<P>& local) {
				return get_object() == local.get_object();
			}

			template<class P> bool operator!=(const Local<P>& local) {
				return !operator==(local);
			}

			template<class P> bool operator==(const Ref<P>& ref) {
				return get_object() == ref.get_object();
			}

			template<class P> bool operator!=(const Ref<P>& ref) {
				return !operator==(ref);
			}

			template<class P> Local<P> cast() const {
				T* obj = get_object();
				if (!obj) return Local<P>(nullptr);
				P* p = dynamic_cast<P*>(obj);
				if (!p) throw std::exception("Class cast error");
				return Local<P>(p);
			}

			template<class P> Local<P> cast_opt() const {
				T* obj = get_object();
				if (!obj) return Local<P>(nullptr);
				return Local<P>(dynamic_cast<P*>(obj));
			}

			template<class P> Local<P> stat_cast() const { return Local<P>(static_cast<P*>(get_object())); }

			Local<T> local() const { return *this; }
		};

		//
		//BasicArray
		//

		template<class T> class BasicArray : public Object {
			NONCOPYABLE(BasicArray);

			friend class PrimitiveArray<T>;
			template<class P> friend class Array;

			static const std::size_t ELEMENT_SIZE = sizeof(T);
			static const std::size_t PTR_SIZE = sizeof(void*);
			static const std::size_t HEADER_SIZE_EXTRA;
			static const std::size_t HEADER_SIZE;

			template<class A, class E>
			class generic_iterator {
				friend class BasicArray<T>;

				gc::Local<A> m_array;
				std::size_t m_pos;
				std::size_t m_length;

				generic_iterator(const gc::Local<A>& array, std::size_t pos, std::size_t length);

			public:
				generic_iterator& operator++();
				bool operator==(const generic_iterator& iter) const;
				bool operator!=(const generic_iterator& iter) const;
				E& operator*() const;
				E* operator->() const;
			};

		public:
			typedef generic_iterator<BasicArray<T>, T> iterator;
			typedef generic_iterator<const BasicArray<T>, const T> const_iterator;

		private:
			BasicArray(std::size_t length);

			static std::size_t calc_size(std::size_t length);
			const char* get_array_ptr_const() const;
			char* get_array_ptr();

		public:
			inline std::size_t length() const;
			inline std::size_t size() const;
			inline T& get(std::size_t index);
			inline const T& get(std::size_t index) const;
			inline T& operator[](std::size_t index);
			inline const T& operator[](std::size_t index) const;
			inline T* raw_array(std::size_t index = 0);
			inline const T* raw_array(std::size_t index = 0) const;

			iterator begin();
			iterator end();
			const_iterator begin() const;
			const_iterator end() const;
		};

		//
		//Array
		//

		template<class T> class Array : public BasicArray<Ref<T>> {
			NONCOPYABLE(Array);

			Array(std::size_t size) : BasicArray<Ref<T>>(size){}
			void gc_enumerate_refs() override;

		public:
			static Local<Array<T>> create(std::size_t size);
		};

		//
		//PrimitiveArray
		//

		template<class T> class PrimitiveArray : public BasicArray<T> {
			NONCOPYABLE(PrimitiveArray);

			PrimitiveArray(std::size_t size) : BasicArray<T>(size){}

		public:
			static Local<PrimitiveArray<T>> create(std::size_t size);

			void set(std::size_t pos, const T* array, std::size_t array_start, std::size_t array_end);

			void set(
				std::size_t pos,
				const gc::Local<PrimitiveArray<T>> array,
				std::size_t array_start,
				std::size_t array_end);
		};

		//
		//AllocObserver
		//

		class internal::AllocObserver {
			NONCOPYABLE(AllocObserver);

		protected:
			AllocObserver(){}

		public:
			virtual void memory_allocated(void* ptr, std::size_t size) = 0;
			virtual void memory_deleted(void* ptr, std::size_t size) = 0;
		};

		//
		//(Misc.)
		//

		template<class T> void Object::gc_ref(Ref<T>& ref) {
			gc_ref_internal(ref);
		}

		template<class T> Local<T> Object::self(T* this_ptr) {
			assert(this == this_ptr);
			return Local<T>(static_cast<T*>(this));
		}

		template<class T> Local<const T> Object::self(const T* this_ptr) const {
			assert(this == this_ptr);
			return Local<const T>(static_cast<const T*>(this));
		}
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		// Implementation of gc::create().

		template<class T, class Fn, class... Args>
		Local<T> internal::create_internal(std::size_t size, Fn init, Args... args) {
			//std::cout << typeid(T).name() << ' '  << size << '\n';
			void* const ptr = internal::new_allocate(size);
			T* object;
			try {
				object = init(ptr);
			} catch (...) {
				internal::new_fail(ptr, size);
				throw;
			}
			internal::new_finish(object, size);
			Local<T> local(object);
			object->initialize(args...);
			return local;
		}

		template<class T, class... Args> Local<T> create(Args... args) {
			return internal::create_internal<T>(
				sizeof(T),
				[](void* ptr){ return new(ptr)T(); },
				args...);
		}

		template<class T>
		void swap(gc::Ref<T>& a, gc::Ref<T>& b) {
			gc::Local<T> t = a;
			a = b;
			b = t;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// Implementation of arrays.

		//
		//BasicArray
		//

		template<class T> const std::size_t BasicArray<T>::HEADER_SIZE_EXTRA =
			sizeof(BasicArray<T>) + PTR_SIZE - 1;
		template<class T> const std::size_t BasicArray<T>::HEADER_SIZE =
			HEADER_SIZE_EXTRA - HEADER_SIZE_EXTRA % PTR_SIZE;

		template<class T> BasicArray<T>::BasicArray(std::size_t size) {
			char* ptr = static_cast<char*>(static_cast<void*>(this)) + HEADER_SIZE;
			for (std::size_t i = 0; i < size; ++i, ptr += ELEMENT_SIZE) {
				new (ptr)T();
			}
		}

		template<class T> std::size_t BasicArray<T>::calc_size(std::size_t length) {
			if (length >= internal::MAX_SIZE / ELEMENT_SIZE
				|| internal::MAX_SIZE - length * ELEMENT_SIZE < HEADER_SIZE)
			{
				throw gc::out_of_memory();
			}

			const std::size_t array_size = length * ELEMENT_SIZE;
			const std::size_t size = HEADER_SIZE + array_size;
			return size;
		}

		template<class T> const char* BasicArray<T>::get_array_ptr_const() const {
			return static_cast<const char*>(static_cast<const void*>(this)) + HEADER_SIZE;
		}

		template<class T> char* BasicArray<T>::get_array_ptr() {
			return const_cast<char*>(get_array_ptr_const());
		}

		template<class T> std::size_t BasicArray<T>::length() const {
			const std::size_t size = get_size();
			const std::size_t array_size = size - HEADER_SIZE;
			return array_size / ELEMENT_SIZE;
		}

		template<class T> std::size_t BasicArray<T>::size() const {
			return length();
		}

		template<class T> T& BasicArray<T>::get(std::size_t index) {
			assert(index < length());
			return *raw_array(index);
		}

		template<class T> const T& BasicArray<T>::get(std::size_t index) const {
			assert(index < length());
			return *raw_array(index);
		}

		template<class T> T& BasicArray<T>::operator[](std::size_t index) {
			return get(index);
		}

		template<class T> const T& BasicArray<T>::operator[](std::size_t index) const {
			return get(index);
		}

		template<class T> T* BasicArray<T>::raw_array(std::size_t index) {
			assert(index <= length());
			char* array_char_ptr = get_array_ptr();
			char* element_char_ptr = array_char_ptr + index * ELEMENT_SIZE;
			return static_cast<T*>(static_cast<void*>(element_char_ptr));
		}

		template<class T> const T* BasicArray<T>::raw_array(std::size_t index) const {
			assert(index <= length());
			const char* array_char_ptr = get_array_ptr_const();
			const char* element_char_ptr = array_char_ptr + index * ELEMENT_SIZE;
			return static_cast<const T*>(static_cast<const void*>(element_char_ptr));
		}

		template<class T> typename BasicArray<T>::iterator BasicArray<T>::begin() {
			return iterator(self(this), 0, length());
		}

		template<class T> typename BasicArray<T>::iterator BasicArray<T>::end() {
			std::size_t len = length();
			return iterator(self(this), len, len);
		}

		template<class T> typename BasicArray<T>::const_iterator BasicArray<T>::begin() const {
			return const_iterator(self(this), 0, length());
		}

		template<class T> typename BasicArray<T>::const_iterator BasicArray<T>::end() const {
			std::size_t len = length();
			return const_iterator(self(this), len, len);
		}

		//
		//BasicArray::generic_iterator
		//

		template<class T> template<class A, class E>
		BasicArray<T>::generic_iterator<A, E>::generic_iterator(
			const gc::Local<A>& array,
			std::size_t pos,
			std::size_t length)
			: m_array(array),
			m_pos(pos),
			m_length(length)
		{}
		
		template<class T> template<class A, class E>
#ifdef _MSC_VER
		typename
#endif
		BasicArray<T>::generic_iterator<A, E>& BasicArray<T>::generic_iterator<A, E>::operator++() {
			assert(m_pos < m_length);
			++m_pos;
			return *this;
		}

		template<class T> template<class A, class E>
		bool BasicArray<T>::generic_iterator<A, E>::operator==(const generic_iterator& iter) const {
			assert(m_array.get() == iter.m_array.get());
			return m_pos == iter.m_pos;
		}

		template<class T> template<class A, class E>
		bool BasicArray<T>::generic_iterator<A, E>::operator!=(const generic_iterator& iter) const {
			assert(m_array.get() == iter.m_array.get());
			return m_pos != iter.m_pos;
		}

		template<class T> template<class A, class E>
		E& BasicArray<T>::generic_iterator<A, E>::operator*() const {
			return (*m_array)[m_pos];
		}

		template<class T> template<class A, class E>
		E* BasicArray<T>::generic_iterator<A, E>::operator->() const {
			return &(*m_array)[m_pos];
		}

		//
		//Array
		//

		template<class T> Local<Array<T>> Array<T>::create(std::size_t length) {
			std::size_t size = Array<T>::calc_size(length);
			return internal::create_internal<Array<T>>(size, [length](void* ptr){ return new(ptr)Array<T>(length); });
		}

		template<class T> void Array<T>::gc_enumerate_refs() {
			char* ptr = this->get_array_ptr();
			for (std::size_t i = 0, n = this->length(); i < n; ++i) {
				Ref<T>* ref_ptr = static_cast<Ref<T>*>(static_cast<void*>(ptr));
				this->gc_ref(*ref_ptr);
				ptr += sizeof(Ref<T>);
			}
		}

		//
		//PrimitiveArray
		//

		template<class T> Local<PrimitiveArray<T>> PrimitiveArray<T>::create(std::size_t length) {
			std::size_t size = PrimitiveArray<T>::calc_size(length);
			return internal::create_internal<PrimitiveArray<T>>(size, [length](void* ptr){
				return new(ptr)PrimitiveArray<T>(length); });
		}

		template<class T> void PrimitiveArray<T>::set(
			std::size_t pos,
			const T* array,
			std::size_t array_start,
			std::size_t array_end)
		{
			assert(array_start < array_end);
			std::size_t array_len = array_end - array_start;
			assert(array_len <= this->length());
			assert(pos <= this->length() - array_len);

			T* dest = static_cast<T*>(this->get_array_ptr());
			for (std::size_t i = 0; i < array_len; ++i) dest[pos + i] = array[array_start + i];
		}

		template<class T> void PrimitiveArray<T>::set(
			std::size_t pos,
			const gc::Local<PrimitiveArray<T>> array,
			std::size_t array_start,
			std::size_t array_end)
		{
			assert(array_start <= array_end);
			assert(array_end <= array->length());
			set(pos, array->raw_array(), array_start, array_end);
		}

	}
}

#endif//SYNSAMPLE_CORE_GC_H_INCLUDED
