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

//SYN run-time library definitions.

#ifndef SYN_RT_SYN_H_INCLUDED
#define SYN_RT_SYN_H_INCLUDED

#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace syn {

	typedef int const_int;
	typedef bool const_bool;
	typedef std::string const_str;

	//
	//SynError
	//

	//Exception thrown by the parser.
	class SynError {
	public:
		virtual ~SynError(){}
	};

	class SynLexicalError : public SynError {};
	class SynSyntaxError : public SynError {};

	//
	//Pool
	//

	//Pool of values. Allocates values of a particular type and returns pointer to those values.
	//When a pool is destroyed, all its values are deleted.
	//Values are allocated by pages, and are not copied when the pool expands.
	template<class T>
	class Pool {
		Pool(const Pool&) = delete;
		Pool(Pool&&) = delete;
		Pool& operator=(const Pool&) = delete;
		Pool& operator=(Pool&&) = delete;

	private:
		struct Page {
			Page* const m_next;
			T* const m_data;
			T* m_end;

			Page(Page* next, std::size_t size)
				: m_next(next),
				m_data(new T[size]),
				m_end(m_data + size)
			{}

			~Page() {
				delete[] m_data;
			}

			T* allocate() {
				if (m_data == m_end) return nullptr;
				return --m_end;
			}
		};

		const std::size_t m_pagesize;
		Page* m_page;

	public:
		explicit Pool(std::size_t pagesize = 512) : m_pagesize(pagesize) {
			assert(pagesize);
			m_page = new Page(nullptr, pagesize);
		}

		~Pool() {
			Page* page = m_page;
			while (page) {
				Page* next = page->m_next;
				delete page;
				page = next;
			}
		}

		inline T* allocate(const T& value) {
			T* ptr = m_page->allocate();
			if (!ptr) {
				Page* new_page = new Page(m_page, m_pagesize);
				m_page = new_page;
				ptr = new_page->allocate();
			}
			assert(ptr);
			*ptr = value;
			return ptr;
		}
	};

	//
	//(Definitions)
	//

	typedef int InternalTk;
	typedef int InternalNt;
	typedef std::size_t InternalAction;

	const std::size_t ACCEPT_ACTION = SIZE_MAX;
	const std::size_t NULL_ACTION = SIZE_MAX - 1;

	struct Shift;
	struct Goto;
	struct Reduce;
	struct State;

	class StackElement;
	class StackElement_Value;
	class StackElement_Nt;

	class ConcreteCoreParser;
	class StacksList;
	class StackElementPool;

	//
	//Shift, Goto, Reduce, State
	//

	//Members of these structures (Shift, Goto, Reduce, State) should be const,
	//but they cannot be, because otherwise it will be impossible to create the states
	//'on the fly', since the structure is cyclic (state -> shift -> state -> goto -> state, ...).

	struct Shift {
		const State* m_state;
		InternalTk m_token;

		void assign(const State* state, InternalTk token);
	};

	struct Goto {
		const State* m_state;
		InternalNt m_nt;

		void assign(const State* state, InternalNt nt);
	};

	struct Reduce {
		int m_length;
		InternalNt m_nt;
		InternalAction m_action;

		void assign(int length, InternalNt nt, InternalAction action);
	};

	struct State {
		enum SymType {
			sym_none,
			sym_tk_value,
			sym_nt
		};

		int m_index;
		const Shift* m_shifts;
		const Goto* m_gotos;
		const Reduce* m_reduces;
		SymType m_sym_type;

		void assign(int index, const Shift* shifts, const Goto* gotos, const Reduce* reduces, SymType sym_type);
	};

	//
	//StackElType
	//

	enum StackElType {
		STACKEL_NONE,
		STACKEL_VALUE,
		STACKEL_NT
	};

	//
	//StackElement
	//

	//GLR parser stack element.
	class StackElement {
		StackElement(const StackElement&) = delete;
		StackElement(StackElement&&) = delete;
		StackElement& operator=(const StackElement&) = delete;
		StackElement& operator=(StackElement&&) = delete;

		friend class StacksList;
		friend class StackElementPool;

#ifndef NDEBUG
		const StackElType m_type;
#endif

		StackElement* m_prev;
		const State* m_state;
		StackElement* m_list;
		std::size_t m_ref_count;

#ifdef NDEBUG
		StackElement(){}
#else
		StackElement() : m_type(STACKEL_NONE){}
#endif

	protected:
#ifdef NDEBUG
		explicit StackElement(StackElType type){}
#else
		explicit StackElement(StackElType type) : m_type(type){}
#endif

		//Stack elements are pooled, they can be reused. For that reason, StackElement is initialized by
		//the init() function, not by a constructor.
		void init(StackElement* prev, const State* state);

	public:
		const State* state() const { return m_state; }
		StackElement* prev() { return m_prev; }

		inline const StackElement_Nt* as_nt() const;
		inline const StackElement_Value* as_value() const;
	};

	//
	//StackElement_Value
	//

	//Stack element which corresponds to a primitive value, produced by a token.
	class StackElement_Value : public StackElement {
		StackElement_Value(const StackElement_Value&) = delete;
		StackElement_Value(StackElement_Value&&) = delete;
		StackElement_Value& operator=(const StackElement_Value&) = delete;
		StackElement_Value& operator=(StackElement_Value&&) = delete;

		friend class StacksList;
		friend class StackElementPool;

		//A pointer to value is stored in stack element instead of the value itself
		//for efficiency reasons. More than one stack node can be created for one
		//input token (when different shifts exist) causing the parser to copy the value
		//in each node. This operation can be expensive for some types, like std::string.
		const void* m_value_ptr;
		
		StackElement_Value() : StackElement(STACKEL_VALUE){}

		void init(StackElement* prev, const State* state, const void* value_ptr);

	public:
		const void* value() const { return m_value_ptr; }
	};

	//
	//StackElement_Nt
	//

	//Stack element corresponding to a nonterminal.
	class StackElement_Nt : public StackElement {
		StackElement_Nt(const StackElement_Nt&) = delete;
		StackElement_Nt(StackElement_Nt&&) = delete;
		StackElement_Nt& operator=(const StackElement_Nt&) = delete;
		StackElement_Nt& operator=(StackElement_Nt&&) = delete;
			
		friend class StacksList;
		friend class StackElementPool;

		//It is possible to remove 'm_reduce'. Since the nonterminal and the subnodes are known, the production
		//can be determined definitely (except if there are two equal productions, but this is an ambiguity, and
		//it had to be detected during reduce, so now just the first production can be chosen).
		const Reduce* m_reduce;
		StackElement* m_sub_elements;

		StackElement_Nt() : StackElement(STACKEL_NT){}
		
		void init(
			StackElement* prev,
			const State* state,
			const Reduce* reduce,
			StackElement* sub_elements);

	public:
		const Reduce* reduce() const { return m_reduce; }
		const StackElement* sub_elements() const { return m_sub_elements; }
		InternalAction action() const { return m_reduce->m_action; }
		void get_sub_elements(std::vector<const StackElement*>& v) const;
	};

	const StackElement_Nt* StackElement::as_nt() const {
#ifndef NDEBUG
		assert(STACKEL_NT == m_type);
#endif
		return static_cast<const StackElement_Nt*>(this);
	}

	const StackElement_Value* StackElement::as_value() const {
#ifndef NDEBUG
		assert(STACKEL_VALUE == m_type);
#endif
		return static_cast<const StackElement_Value*>(this);
	}

	//
	//ScannerInterface
	//

	class ScannerInterface {
	protected:
		ScannerInterface(){}

	public:
		virtual std::pair<InternalTk, const void*> scan() = 0;
	};

	//
	//ParserInterface
	//

	class ParserInterface {
	protected:
		ParserInterface(){}

	public:
		virtual StackElement_Nt* parse(const State* start_state, ScannerInterface& scanner, InternalTk tk_eof) = 0;
		static std::unique_ptr<ParserInterface> create();
	};

	//
	//SynScannerCore
	//

	template<class Scanner, class ValuePool, class TokenValue>
	class SynScannerCore : public ScannerInterface {
		SynScannerCore(const SynScannerCore&) = delete;
		SynScannerCore(SynScannerCore&&) = delete;
		SynScannerCore& operator=(const SynScannerCore&) = delete;
		SynScannerCore& operator=(SynScannerCore&&) = delete;

		TokenValue m_token_value;
		Scanner& m_scanner;
		ValuePool m_value_pool;

	public:
		explicit SynScannerCore(Scanner& scanner) : m_scanner(scanner){}

		std::pair<InternalTk, const void*> scan() override {
			InternalTk token = m_scanner.scan(m_token_value);
			const void* value = m_value_pool.allocate_value(token, m_token_value);
			return std::make_pair(token, value);
		}

		ValuePool& get_value_pool() {
			return m_value_pool;
		}
	};

	//
	//BasicSynParser
	//

	template<class Scanner, class ValuePool, class TokenValue, InternalTk eof_token>
	class BasicSynParser {
		BasicSynParser(const BasicSynParser&) = delete;
		BasicSynParser(BasicSynParser&&) = delete;
		BasicSynParser& operator=(const BasicSynParser&) = delete;
		BasicSynParser& operator=(BasicSynParser&&) = delete;

		syn::SynScannerCore<Scanner, ValuePool, TokenValue> m_scanner_core;

		//The parser must be a member of this class, because it owns returned stack elements,
		//and thus has to exist longer than a parse() method call execution.
		std::unique_ptr<ParserInterface> m_parser;

	public:
		explicit BasicSynParser(Scanner& scanner)
			: m_scanner_core(scanner),
			m_parser(ParserInterface::create())
		{}

		StackElement_Nt* parse(const State* start) {
			return m_parser->parse(start, m_scanner_core, eof_token);
		}
	};

	template<class Ch>
	inline char default_char_convertor(Ch ch) {
		return ch;
	}

	//
	//TokenDescriptor
	//

	struct TokenDescriptor {
		const std::string name;
		const std::string str;
	};

	//
	//ProductionStack
	//

	class ProductionStack {
		ProductionStack(const ProductionStack&) = delete;
		ProductionStack(ProductionStack&&) = delete;
		ProductionStack& operator=(const ProductionStack&) = delete;
		ProductionStack& operator=(ProductionStack&&) = delete;

		std::vector<const StackElement*>& m_vector;
		const std::size_t m_size;
		const StackElement_Nt* const m_nt;

	public:
		ProductionStack(std::vector<const StackElement*>& vector, const StackElement* node);
		~ProductionStack();

		std::size_t size() const;
		const StackElement_Nt* get_nt() const;
		const StackElement* operator[](std::size_t index) const;
	};

	std::exception illegal_state();
	bool is_production(const ProductionStack& stack, InternalAction pr, std::size_t len);
	void check_production(const ProductionStack& stack, InternalAction pr, std::size_t len);

	//
	//InternalAllocator
	//

	struct InternalAllocator {
		template<class T> using ListPtr = std::unique_ptr<std::vector<T>>;

		template<class T>
		static ListPtr<T> list_first(const T& elem) {
			ListPtr<T> list(new std::vector<T>());
			list->push_back(elem);
			return list;
		}

		template<class T>
		static void list_next(const ListPtr<T>& list, const T& elem) {
			list->push_back(elem);
		}

		template<class T>
		static ListPtr<T> list_null() {
			return ListPtr<T>(new std::vector<T>());
		}

		template<class T>
		static std::unique_ptr<T> list_move(std::unique_ptr<T>& ptr) {
			return std::move(ptr);
		}
	};

	//
	//ExternalAllocator
	//
	
	struct ExternalAllocator {
		template<class T> using Ptr = std::shared_ptr<T>;
		template<class T> using List = std::vector<T>;
		template<class T> using NodeList = std::vector<Ptr<T>>;

		template<class T>
		static Ptr<T> create() {
			return Ptr<T>(new T());
		}

		template<class T>
		static Ptr<List<T>> list(std::unique_ptr<std::vector<T>> v) {
			return Ptr<List<T>>(v.release());
		}

		template<class T>
		static Ptr<List<Ptr<T>>> node_list(std::unique_ptr<std::vector<Ptr<T>>> v) {
			return Ptr<NodeList<T>>(v.release());
		}
	};
}

#endif//SYN_RT_SYN_H_INCLUDED
