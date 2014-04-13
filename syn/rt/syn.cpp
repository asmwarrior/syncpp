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

//SYN run-time library implementation.

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "syn.h"

using syn::InternalTk;
using syn::InternalNt;
using syn::InternalAction;
using syn::Shift;
using syn::Goto;
using syn::Reduce;
using syn::State;

//
//StackElement
//

void syn::StackElement::init(StackElement* prev, const State* state) {
	m_prev = prev;
	m_state = state;
	m_list = 0;
	m_ref_count = 0;
}

//
//StackElement_Value
//

void syn::StackElement_Value::init(StackElement* prev, const State* state, const void* value_ptr) {
	StackElement::init(prev, state);
	m_value_ptr = value_ptr;
}

//
//StackElement_Nt
//

void syn::StackElement_Nt::init(
	StackElement* prev,
	const State* state,
	const Reduce* reduce,
	StackElement* sub_elements)
{
	StackElement::init(prev, state);
	m_reduce = reduce;
	m_sub_elements = sub_elements;
}

void syn::StackElement_Nt::get_sub_elements(std::vector<const StackElement*>& v) const {
	std::size_t ofs = v.size();
	std::size_t len = m_reduce->m_length;
	v.resize(ofs + len);

	StackElement* element = m_sub_elements;
	while (len) {
		assert(element);
		--len;
		v[ofs + len] = element;
		element = element->prev();
	}
}

//
//Shift
//

void Shift::assign(const State* state, InternalTk token) {
	m_state = state;
	m_token = token;
}

//
//Goto
//

void Goto::assign(const State* state, InternalNt nt) {
	m_state = state;
	m_nt = nt;
}

//
//Reduce
//

void Reduce::assign(int length, InternalNt nt, InternalAction action) {
	m_length = length;
	m_nt = nt;
	m_action = action;
}

//
//State
//

void State::assign(int index, const Shift* shifts, const Goto* gotos, const Reduce* reduces, SymType sym_type) {
	m_index = index;
	m_shifts = shifts;
	m_gotos = gotos;
	m_reduces = reduces;
	m_sym_type = sym_type;
}

//
//ProductionStack
//

syn::ProductionStack::ProductionStack(std::vector<const StackElement*>& vector, const StackElement* node)
: m_vector(vector),
m_size(vector.size()),
m_nt(node->as_nt())
{
	m_nt->get_sub_elements(vector);
}

syn::ProductionStack::~ProductionStack() {
	m_vector.resize(m_size);
}

std::size_t syn::ProductionStack::size() const {
	return m_vector.size() - m_size;
}

const syn::StackElement_Nt* syn::ProductionStack::get_nt() const {
	return m_nt;
}

const syn::StackElement* syn::ProductionStack::operator[](std::size_t index) const {
	return m_vector[m_size + index];
}

//
//(Functions)
//

std::exception syn::illegal_state() {
	throw std::logic_error("illegal state");
}

bool syn::is_production(const ProductionStack& stack, InternalAction pr, std::size_t len) {
	if (stack.get_nt()->action() != pr) return false;
	if (stack.size() != len) throw illegal_state();
	return true;
}

void syn::check_production(const ProductionStack& stack, InternalAction pr, std::size_t len) {
	if (!is_production(stack, pr, len)) throw illegal_state();
}

//
//StacksList : definition
//

namespace syn {
	class StacksList {
		StacksList(const StacksList&) = delete;
		StacksList(StacksList&&) = delete;
		StacksList& operator=(const StacksList&) = delete;
		StacksList& operator=(StacksList&&) = delete;

		StackElementPool& m_element_pool;
		StackElement* m_begin;

		StackElement* delete_unreferenced_element(StackElement* element, StackElement* to_delete_queue);
		void delete_reference(StackElement* element);
		void clear();
			
	public:
		explicit StacksList(StackElementPool& element_pool)
			: m_begin(0),
			m_element_pool(element_pool)
		{}

		~StacksList() {
			clear();
		}

		class iterator {
			friend class StacksList;

			StackElement* m_element;

		public:
			explicit iterator(StackElement* stack_el) : m_element(stack_el){}

			bool operator==(iterator iter) const { return m_element == iter.m_element; }
			bool operator!=(iterator iter) const { return m_element != iter.m_element; }
			StackElement* operator*() const { return m_element; }
			StackElement* element() const { return m_element; }

			iterator& operator++() {
				//WARN the iterator must not point to the end of list (i. e. m_element != 0).
				m_element = m_element->m_list;
				return *this;
			}
		};

		iterator begin() const { return iterator(m_begin); }
		iterator end() const { return iterator(nullptr); }
		bool empty() const { return !m_begin; }

		void push_front_start_state(const State* start_state);
		void push_front_tk(StackElement* prev, const State* state, const void* value_ptr);

		void push_front_nt(
			StackElement* prev,
			const State* state,
			const Reduce* reduce,
			StackElement* sub_elements);

		void move(StacksList& source_list);
		void print(std::ostream& out);
	};
}

//
//StackElementPool : definition
//

namespace syn {
	class StackElementPool {
		StackElementPool(const StackElementPool&) = delete;
		StackElementPool(StackElementPool&&) = delete;
		StackElementPool& operator=(const StackElementPool&) = delete;
		StackElementPool& operator=(StackElementPool&&) = delete;

		template<class T>
		class SimplePool {
			SimplePool(const SimplePool&) = delete;
			SimplePool(SimplePool&&) = delete;
			SimplePool& operator=(const SimplePool&) = delete;
			SimplePool& operator=(SimplePool&&) = delete;

			T* m_list;
			std::size_t m_size;
			std::size_t m_allocations_count;

		public:
			SimplePool() : m_list(nullptr) {
				m_size = 0;
				m_allocations_count = 0;
			}

			~SimplePool() {
				T* el = m_list;
				while (el) {
					T* next = static_cast<T*>(el->m_list);
					delete el;
					el = next;
				}
				m_list = nullptr;
			}

			T* allocate() {
				T* result = m_list;
				if (result) {
					m_list = static_cast<T*>(result->m_list);
					--m_size;
				}
				++m_allocations_count;
				return result;
			}

			void release(T* element) {
				element->m_list = m_list;
				m_list = element;
				++m_size;
			}

			std::size_t size() const {
				return m_size;
			}

			std::size_t allocations_count() const {
				return m_allocations_count;
			}
		};

		SimplePool<StackElement> m_element_pool;
		SimplePool<StackElement_Value> m_element_value_pool;
		SimplePool<StackElement_Nt> m_element_nt_pool;

		template<class T>
		T* allocate_el(SimplePool<T>& pool) {
			T* el = pool.allocate();
			if (!el) {
				el = new T();
			}
			return el;
		}

		template<class T>
		void release_el(SimplePool<T>& pool, T* el) {
			pool.release(el);
		}

	public:
		StackElementPool(){}

		StackElement* allocate_element(StackElement* prev, const State* state);
		StackElement* allocate_element_value(StackElement* prev, const State* state, const void* value_ptr);
			
		StackElement* allocate_element_nt(
			StackElement* prev,
			const State* state,
			const Reduce* reduce,
			StackElement* sub_elements);

		void release_element(StackElement* element);
		void release_element_value(StackElement_Value* element_value);
		void release_element_nt(StackElement_Nt* element_nt);
	};
}

//
//StacksList : implementation
//

syn::StackElement* syn::StacksList::delete_unreferenced_element(
	syn::StackElement* element,
	syn::StackElement* to_delete_queue)
{
	StackElement* prev = element->prev();
	if (prev && !--prev->m_ref_count) {
		prev->m_list = to_delete_queue;
		to_delete_queue = prev;
	}

	//Determine the type of the element and release it.
	const State* state = element->state();
	State::SymType sym_type = state->m_sym_type;
	if (State::sym_nt == sym_type) {
		//Check for NT is first, because NT is the most frequent type of nodes.
		StackElement_Nt* element_nt = static_cast<StackElement_Nt*>(element);
		StackElement* sub = element_nt->m_sub_elements;
		if (sub && !--sub->m_ref_count) {
			sub->m_list = to_delete_queue;
			to_delete_queue = sub;
		}
		m_element_pool.release_element_nt(element_nt);
	} else if (State::sym_tk_value == sym_type) {
		m_element_pool.release_element_value(static_cast<StackElement_Value*>(element));
	} else {
		//Assuming sym_none.
		m_element_pool.release_element(element);
	}

	return to_delete_queue;
}

void syn::StacksList::delete_reference(syn::StackElement* element) {
	if (!--element->m_ref_count){
		//Since the element cannot be used after having been passed to this function,
		//we can use the element's link field to make the linked list of elements to be
		//released. The list is necessary, because Nt-elements may reference two
		//chains of elements (previous and sub-elements).

		element->m_list = nullptr;
		StackElement* to_delete = element;
		while (to_delete) to_delete = delete_unreferenced_element(to_delete, to_delete->m_list);
	}
}

void syn::StacksList::clear() {
	StackElement* element = m_begin;
	while (element) {
		StackElement* next = element->m_list;
		delete_reference(element);
		element = next;
	}
	m_begin = nullptr;
}

void syn::StacksList::push_front_start_state(const State* start_state) {
	StackElement* element = m_element_pool.allocate_element(nullptr, start_state);
	element->m_list = m_begin;
	element->m_ref_count = 1;
	m_begin = element;
}

void syn::StacksList::push_front_tk(syn::StackElement* prev, const State* state, const void* value_ptr) {
	StackElement* element;
	if (value_ptr) {
		element = m_element_pool.allocate_element_value(prev, state, value_ptr);
	} else {
		element = m_element_pool.allocate_element(prev, state);
	}
	element->m_list = m_begin;
	element->m_ref_count = 1;
	if (prev) {
		++prev->m_ref_count;
	}
	m_begin = element;
}

void syn::StacksList::push_front_nt(
	syn::StackElement* prev,
	const State* state,
	const Reduce* reduce,
	syn::StackElement* sub_elements)
{
	StackElement* element = m_element_pool.allocate_element_nt(prev, state, reduce, sub_elements);
	element->m_list = m_begin;
	element->m_ref_count = 1;
	if (prev) {
		++prev->m_ref_count;
	}
	if (sub_elements) {
		//Only the reference to the last element is increased, since all the other elements
		//are referenced by the last one.
		++sub_elements->m_ref_count;
	}
	m_begin = element;
}

void syn::StacksList::move(StacksList& source_list) {
	clear();
	m_begin = source_list.m_begin;
	source_list.m_begin = nullptr;
}

void syn::StacksList::print(std::ostream& out) {
	const char* sep = "";
	for (StackElement* element : *this) {
		out << sep << element->m_state->m_index;
		sep = " ";
	}
}

//
//StackElementPool : implementation
//

syn::StackElement* syn::StackElementPool::allocate_element(syn::StackElement* prev, const State* state) {
	StackElement* element = allocate_el(m_element_pool);
	element->init(prev, state);
	return element;
}

syn::StackElement* syn::StackElementPool::allocate_element_value(
	syn::StackElement* prev,
	const State* state,
	const void* value_ptr)
{
	StackElement_Value* element_value = allocate_el(m_element_value_pool);
	element_value->init(prev, state, value_ptr);
	return element_value;
}

syn::StackElement* syn::StackElementPool::allocate_element_nt(
	syn::StackElement* prev,
	const State* state,
	const Reduce* reduce,
	syn::StackElement* sub_elements)
{
	StackElement_Nt* element_nt = allocate_el(m_element_nt_pool);
	element_nt->init(prev, state, reduce, sub_elements);
	return element_nt;
}

void syn::StackElementPool::release_element(syn::StackElement* element) {
	release_el(m_element_pool, element);
}

void syn::StackElementPool::release_element_value(syn::StackElement_Value* element_value) {
	release_el(m_element_value_pool, element_value);
}

void syn::StackElementPool::release_element_nt(syn::StackElement_Nt* element_nt) {
	release_el(m_element_nt_pool, element_nt);
}

//
//CoreParser
//

namespace syn {
	class CoreParser : public ParserInterface {
		CoreParser(const CoreParser&) = delete;
		CoreParser(CoreParser&&) = delete;
		CoreParser& operator=(const CoreParser&) = delete;
		CoreParser& operator=(CoreParser&&) = delete;

		StackElementPool m_element_pool;
		StacksList m_stacks_list;

		void reduce_and_goto(StacksList::iterator stack, const Reduce* reduce);
		void reduce_one_stack(StacksList::iterator stack, StackElement_Nt** result_element, bool* accept);
		void reduce_stacks(StackElement_Nt** result_element, bool* accept);
		void shift_stacks(InternalTk token, const void* value_ptr);

	public:
		CoreParser();
		StackElement_Nt* parse(const State* start_state, ScannerInterface& scanner, InternalTk tk_eof) override;
	};
}

syn::CoreParser::CoreParser() : m_stacks_list(m_element_pool){}

void syn::CoreParser::reduce_and_goto(syn::StacksList::iterator stack, const Reduce* reduce) {
	StackElement* stack_el = stack.element();
	StackElement* origin = stack_el;
	for (int i = reduce->m_length; i; --i) {
		assert(origin);
		origin = origin->prev();
	}

	const InternalNt nt = reduce->m_nt;

	const Goto* pgoto = origin->state()->m_gotos;
	if (!pgoto) return;

	while (pgoto->m_state) {
		if (nt == pgoto->m_nt) {
			m_stacks_list.push_front_nt(origin, pgoto->m_state, reduce, stack_el);
			break;
		}
		++pgoto;
	}
}

void syn::CoreParser::reduce_one_stack(
	syn::StacksList::iterator stack,
	syn::StackElement_Nt** result_element,
	bool* accept)
{
	StackElement* stack_el = stack.element();
	const Reduce* reduce = stack_el->state()->m_reduces;
	if (!reduce) return;

	while (reduce->m_action != NULL_ACTION) {
		if (reduce->m_action == ACCEPT_ACTION) {
			StackElement_Nt* element_nt = static_cast<StackElement_Nt*>(stack_el);
			*result_element = element_nt;
			*accept = true;
		} else {
			reduce_and_goto(stack, reduce);
		}
		++reduce;
	}
}

void syn::CoreParser::reduce_stacks(syn::StackElement_Nt** result_element, bool* accept) {
	StacksList::iterator end = m_stacks_list.end();
	StacksList::iterator start = m_stacks_list.begin();
	while (start != end) {
		for (StacksList::iterator cur = start; cur != end; ++cur) {
			reduce_one_stack(cur, result_element, accept);
			//TODO Check for stack duplication, i. e. ambiguity.
		}

		end = start;
		start = m_stacks_list.begin();
	}
}

void syn::CoreParser::shift_stacks(InternalTk token, const void* value_ptr) {
	StacksList next_stacks_list(m_element_pool);
	for (StackElement* stack_el : m_stacks_list) {
		if (const Shift* shift = stack_el->state()->m_shifts) {
			while (shift->m_state) {
				if (token == shift->m_token) {
					next_stacks_list.push_front_tk(stack_el, shift->m_state, value_ptr);
				}
				++shift;
			}
		}
	}
	m_stacks_list.move(next_stacks_list);
}

syn::StackElement_Nt* syn::CoreParser::parse(
	const State* start_state,
	syn::ScannerInterface& scanner,
	const InternalTk tk_eof)
{
	m_stacks_list.push_front_start_state(start_state);
	StackElement_Nt* result_element;
	bool accept;

	for (;;) {
		//1. Reduce.
		result_element = nullptr;
		accept = false;
		reduce_stacks(&result_element, &accept);

		//2. Shift.
		std::pair<InternalTk, const void*> scan_result = scanner.scan();
		InternalTk token = scan_result.first;
		const void* value_ptr = scan_result.second;
		shift_stacks(token, value_ptr);
				
		//3. Check.
		if (m_stacks_list.empty()) {
			//Optimization trick: since there cannot be a shift with token=EOF, on EOF we will get here.
			//No additional check for EOF is needed in the main loop of shift_stack() in this case.
			if (tk_eof == token) {
				if (!accept) throw SynSyntaxError();
				return result_element;
			}
			//Syntax error.
			throw SynSyntaxError();
		}
	}
}

//
//ParserInterface
//

std::unique_ptr<syn::ParserInterface> syn::ParserInterface::create() {
	return std::unique_ptr<ParserInterface>(new CoreParser());
}
