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

//Double-linked list templates.

#ifndef SYNSAMPLE_CORE_DBLLIST_H_INCLUDED
#define SYNSAMPLE_CORE_DBLLIST_H_INCLUDED

namespace syn_script {

	template<class T, T* T::*Prev, T* T::*Next, class E = T> class DoubleLinkedList;

	//
	//DoubleLinkedListIterator
	//

	template<class T, T* T::*Prev, T* T::*Next, class E>
	class DoubleLinkedListIterator {
		friend class DoubleLinkedList<T, Prev, Next, E>;

	private:
		typedef DoubleLinkedListIterator<T, Prev, Next, E> DListIterator;

		E* m_element;

	public:
		DoubleLinkedListIterator(E* element) : m_element(element){}

		DListIterator& operator++() {
			m_element = m_element->*Next;
			return *this;
		}

		bool operator==(const DListIterator& iter) const {
			return m_element == iter.m_element;
		}

		bool operator!=(const DListIterator& iter) const {
			return m_element != iter.m_element;
		}

		E* operator*() const {
			return m_element;
		}
	};

	//
	//DoubleLinkedList
	//

	template<class T, T* T::*Prev, T* T::*Next, class E>
	class DoubleLinkedList {
		DoubleLinkedList() = delete;

	private:
		typedef DoubleLinkedList<T, Prev, Next, E> DList;
		typedef DoubleLinkedListIterator<T, Prev, Next, E> DListIterator;

		E* m_head;

	public:
		DoubleLinkedList(E* head) : m_head(head){}

		static void init(T* element) {
			element->*Prev = element;
			element->*Next = element;
		}

		static void clear(T* head) {
			head->*Prev = head;
			head->*Next = head;
		}

		static void add(T* head, T* element) {
			element->*Next = head;
			T* prev = head->*Prev;
			element->*Prev = prev;
			prev->*Next = element;
			head->*Prev = element;
		}

		static void remove(T* element) {
			T* prev = element->*Prev;
			T* next = element->*Next;
			prev->*Next = next;
			next->*Prev = prev;
		}

		static bool is_empty(const E* head) {
			return head->*Next == head;
		}

		static void move_replace(T* src_head, T* dst_head) {
			if (is_empty(src_head)) {
				clear(dst_head);
			} else {
				T* first = src_head->*Next;
				T* last = src_head->*Prev;

				dst_head->*Next = first;
				dst_head->*Prev = last;
				first->*Prev = dst_head;
				last->*Next = dst_head;

				clear(src_head);
			}
		}

		static void move_add(T* src_head, T* dst_head) {
			if (is_empty(src_head)) return;

			T* src_first = src_head->*Next;
			T* src_last = src_head->*Prev;
			T* dst_last = dst_head->*Prev;

			dst_last->*Next = src_first;
			src_first->*Prev = dst_last;
			dst_head->*Prev = src_last;
			src_last->*Next = dst_head;

			clear(src_head);
		}

		static DListIterator begin(E* head) {
			return DListIterator(head->*Next);
		}

		static DListIterator end(E* head) {
			return DListIterator(head);
		}

		static DoubleLinkedList<T, Prev, Next, const E> const_list(const E* head) {
			return DoubleLinkedList<T, Prev, Next, const E>(head);
		}

		DListIterator begin() const {
			return begin(m_head);
		}

		DListIterator end() const {
			return end(m_head);
		}
	};

}

#endif//SYNSAMPLE_CORE_DBLLIST_H_INCLUDED
