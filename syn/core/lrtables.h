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

//LR Tables templates.

#ifndef SYN_CORE_LRTABLES_H_INCLUDED
#define SYN_CORE_LRTABLES_H_INCLUDED

#include <iostream>
#include <ostream>
#include <map>
#include <set>
#include <vector>

#include "bnf.h"
#include "cntptr.h"
#include "noncopyable.h"
#include "util.h"

namespace synbin {

	template<class TraitsT>
	class LRGenerator;

	//
	//LRTables
	//

	//LR parser control tables, template.
	//The template parameter, TraitsT, must be a type containing tree types:
	//   NtObj - type of a custom nonterminal symbol descriptor.
	//   TrObj - type of a custom terminal symbol descriptor.
	//   PrObj - type of a custom production descriptor.
	template<class TraitsT>
	class LRTables {
		NONCOPYABLE(LRTables);

	public:
		typedef TraitsT Traits;
		typedef typename Traits::NtObj NtObj;
		typedef typename Traits::TrObj TrObj;
		typedef typename Traits::PrObj PrObj;

		friend class LRGenerator<Traits>;

		typedef BnfGrammar<Traits> Bnf;
		typedef typename Bnf::Sym Sym;
		typedef typename Bnf::Nt Nt;
		typedef typename Bnf::Tr Tr;
		typedef typename Bnf::Pr Pr;

		class State;
		class Shift;
		class Goto;

		//
		//Shift
		//

		class Shift {
			friend class LRGenerator<Traits>;

			const Tr* m_tr;
			const State* m_state;

			Shift(const Tr* tr, const State* state) : m_tr(tr), m_state(state){}

		public:
			const Tr* get_tr() const { return m_tr; }
			const State* get_state() const { return m_state; }
		};

		//
		//Goto
		//

		class Goto {
			friend class LRGenerator<Traits>;

			const Nt* m_nt;
			const State* m_state;

			Goto(const Nt* nt, const State* state) : m_nt(nt), m_state(state){}

		public:
			const Nt* get_nt() const { return m_nt; }
			const State* get_state() const { return m_state; }
		};

		//
		//State
		//

		class State : public RefCnt {
			NONCOPYABLE(State);

			friend class LRGenerator<Traits>;

			const int m_index;
			const Sym* m_sym;
			std::vector<Shift> m_shifts;
			std::vector<Goto> m_gotos;

			//May contain 0. 0 means 'accept'.
			std::vector<const Pr*> m_reduces;

			State(int index, const Sym* sym) : m_index(index), m_sym(sym){}

			void init(
				const std::vector<Shift>& shifts,
				const std::vector<Goto>& gotos,
				const std::vector<const Pr*>& reduces)
			{
				m_shifts = shifts;
				m_gotos = gotos;
				m_reduces = reduces;
			}

		public:
			const Sym* get_sym() const { return m_sym; }
			int get_index() const { return m_index; }
			const std::vector<Shift>& get_shifts() const { return m_shifts; }
			const std::vector<Goto>& get_gotos() const { return m_gotos; }
			const std::vector<const Pr*>& get_reduces() const { return m_reduces; }
		};

	private:
		const std::unique_ptr<const std::vector<CntPtr<State>>> m_owned_states;
		std::vector<const State*> m_states;
		const std::vector<std::pair<const Nt*, const State*>> m_start_states;

		LRTables(
			std::unique_ptr<std::vector<CntPtr<State>>>& owned_states,
			const std::vector<std::pair<const Nt*, const State*>>& start_states)
			: m_owned_states(std::move(owned_states)),
			m_start_states(start_states)
		{
			m_states.reserve(m_owned_states->size());
			for (const CntPtr<State>& state : *m_owned_states) m_states.push_back(state.get());
		}

	public:
		const std::vector<const State*>& get_states() const { return m_states; }
		const std::vector<std::pair<const Nt*, const State*>>& get_start_states() const { return m_start_states; }
	};

	//
	//LRGenerator
	//

	//LR tables generator. Creates LR tables for the given BNF grammar.
	template<class TraitsT>
	class LRGenerator {
		NONCOPYABLE(LRGenerator);

	public:
		typedef TraitsT Traits;
		typedef typename Traits::NtObj NtObj;
		typedef typename Traits::TrObj TrObj;
		typedef typename Traits::PrObj PrObj;

	private:
		typedef BnfGrammar<Traits> Bnf;
		typedef typename Bnf::Sym Sym;
		typedef typename Bnf::Nt Nt;
		typedef typename Bnf::Tr Tr;
		typedef typename Bnf::Pr Pr;
		
		//A derived class is used instead of typedef to avoid "decorated name length exceeded..." VC++ warning.
		//class ExtTraits : public BnfTraits<const Nt*, const Tr*, const Pr*>{};
		typedef BnfTraits<const Nt*, const Tr*, const Pr*> ExtTraits;

		typedef BnfGrammar<ExtTraits> ExtBnf;
		typedef typename ExtBnf::Sym ExtSym;
		typedef typename ExtBnf::Nt ExtNt;
		typedef typename ExtBnf::Tr ExtTr;
		typedef typename ExtBnf::Pr ExtPr;

		typedef LRTables<Traits> Tables;
		typedef typename Tables::Shift Shift;
		typedef typename Tables::Goto Goto;
		typedef typename Tables::State State;

		class LRItem;
		class LRSet;

		//
		//LRItem
		//

		class LRItem : public RefCnt {
			NONCOPYABLE(LRItem);

			int m_index;

		public:
			const int m_pos;
			const LRItem* m_next;
			const ExtSym* m_sym;
			const ExtPr* m_pr;

			LRItem(int pos, const LRItem* next, const ExtSym* sym, const ExtPr* pr)
				: m_pos(pos), m_next(next), m_sym(sym), m_pr(pr){}

			void set_index(int index) { m_index = index; }
			int get_index() const { return m_index; }
		};

		//
		//LRSet
		//

		class LRSet : public RefCnt {
			NONCOPYABLE(LRSet);

			State* const m_state;
			const std::vector<const LRItem*> m_items;

		public:
			LRSet(State* state, const std::vector<const LRItem*>& items)
				: m_state(state), m_items(items) {}

			State* get_state() const { return m_state; }
			const std::vector<const LRItem*>& get_items() const { return m_items; }
		};

		static bool compare_item_sym(const CntPtr<LRItem>& a, const CntPtr<LRItem>& b) {
			int idx_a = a->m_sym ? a->m_sym->get_sym_index() : 0;
			int idx_b = b->m_sym ? b->m_sym->get_sym_index() : 0;
			return idx_a < idx_b;
		};

		static bool compare_item_index(const LRItem* a, const LRItem* b) {
			return a->get_index() < b->get_index();
		};

		static bool compare_ext_nt_index(const ExtNt* a, const ExtNt* b) {
			return a->get_nt_index() < b->get_nt_index();
		};

		static bool compare_vectors_of_items(
			const std::vector<const LRItem*>* a,
			const std::vector<const LRItem*>* b)
		{
			typedef typename std::vector<const LRItem*>::size_type size_type;
			size_type a_size = a->size();
			size_type b_size = b->size();
			if (a_size < b_size) return true;
			if (a_size > b_size) return false;

			return std::lexicographical_compare(
				a->begin(), a->end(), b->begin(), b->end(), std::ptr_fun(&compare_item_index));
		}

		//
		//ListSet
		//
		
		//A list and a set in one - an ordered set.
		template<class T, class Compare>
		class ListSet {
			NONCOPYABLE(ListSet);

			std::vector<T> m_vector;
			std::set<T, Compare> m_set;

		public:
			explicit ListSet(const Compare& comp) : m_set(comp){}

			void add(const T& value) {
				std::pair<typename std::set<T, Compare>::iterator, bool> ins = m_set.insert(value);
				if (ins.second) m_vector.push_back(value);
			}

			void clear() {
				m_vector.clear();
				m_set.clear();
			}
		
			std::size_t size() const { return m_vector.size(); }
			const T& get(std::size_t index) const { return m_vector[index]; }
			const std::vector<T>& vector() const { return m_vector; }
		};

		//
		//ExtNtListSet
		//

		typedef std::pointer_to_binary_function<const ExtNt*, const ExtNt*, bool> ExtNtCompare;
		class ExtNtListSet : public ListSet<const ExtNt*, ExtNtCompare> {
		public:
			ExtNtListSet() : ListSet<const ExtNt*, ExtNtCompare>(std::ptr_fun(&compare_ext_nt_index)) {}
		};

		typedef std::pointer_to_binary_function<
			const std::vector<const LRItem*>*,
			const std::vector<const LRItem*>*,
			bool>
		LRItemVectorCompare;

		//Copies productions from the nonterminal 'nt' to the nonterminal 'ext_nt', translating grammar symbols
		//by the 'ext_syms_table' table.
		static void create_ext_productions(
			BnfGrammarBuilder<ExtTraits>& ext_bnf_builder,
			const Nt* nt,
			const ExtNt* ext_nt,
			const std::vector<const ExtSym*>& ext_syms_table)
		{
			for (const Pr* pr : nt->get_productions()) {
				const std::vector<const Sym*>& elements = pr->get_elements();

				//Translate production elements.
				std::vector<const ExtSym*> ext_elements;
				for (const Sym* sym : elements) {
					const ExtSym* ext_sym = ext_syms_table[sym->get_sym_index()];
					ext_elements.push_back(ext_sym);
				}

				//Add production.
				ext_bnf_builder.add_production(ext_nt, pr, ext_elements);
			}
		}

		//Creates an extended BNF grammar for the given BNF grammar.
		//For each start nonterminal N of the source grammar, a nonterminal N1 : N is
		//added to the extended grammar.
		//Extended nonterminals are ones with nt_obj == nullptr.
		static std::unique_ptr<const ExtBnf> create_ext_grammar(
			const Bnf& bnf_grammar,
			const std::vector<const Nt*>& start_nts)
		{
			BnfGrammarBuilder<ExtTraits> ext_bnf_builder;

			std::vector<const ExtSym*> ext_syms(bnf_grammar.get_symbols().size());
			std::vector<const ExtNt*> ext_nts;

			//Create terminals.
			const std::vector<const Tr*>& trs = bnf_grammar.get_terminals();
			for (const Tr* tr : trs) {
				const ExtTr* ext_tr = ext_bnf_builder.create_terminal(tr->get_name(), tr);
				ext_syms[tr->get_sym_index()] = ext_tr;
			}

			//Create nonterminals.
			const std::vector<const Nt*>& nts = bnf_grammar.get_nonterminals();
			for (const Nt* nt : nts) {
				const ExtNt* ext_nt = ext_bnf_builder.create_nonterminal(nt->get_name(), nt);
				ext_syms[nt->get_sym_index()] = ext_nt;
				ext_nts.push_back(ext_nt);
			}

			//Create productions.
			for (const ExtNt* ext_nt : ext_nts) {
				const Nt* nt = ext_nt->get_nt_obj();
				create_ext_productions(ext_bnf_builder, nt, ext_nt, ext_syms);
			}

			//Create extended start nonterminals.
			for (const Nt* nt : start_nts) {
				util::String name(nt->get_name().str() + "'");
				const ExtNt* ext_nt = ext_bnf_builder.create_nonterminal(name, nullptr);
				
				std::vector<const ExtSym*> ext_elements;
				ext_elements.push_back(ext_syms[nt->get_sym_index()]);
				ext_bnf_builder.add_production(ext_nt, nullptr, ext_elements);
			}
			
			//Create and return a grammar object.
			return ext_bnf_builder.create_grammar();
		}

		const Bnf& m_bnf_grammar;
		std::vector<CntPtr<LRItem>> m_all_items;
		std::vector<CntPtr<LRSet>> m_set_list;
		std::vector<std::vector<const LRItem*>> m_sym_to_items;
		std::vector<std::pair<const Nt*, const State*>> m_start_states;

		std::unique_ptr<std::vector<CntPtr<State>>> m_owned_states;

		typedef std::map<const std::vector<const LRItem*>*, const LRSet*, LRItemVectorCompare> set_map_type;
		typedef typename set_map_type::iterator set_map_iterator;
		set_map_type m_set_map;

		LRGenerator(const Bnf& bnf_grammar)
			: m_bnf_grammar(bnf_grammar),
			m_set_map(std::ptr_fun(&compare_vectors_of_items)),
			m_owned_states(make_unique1<std::vector<CntPtr<State>>>())
		{}

		//Creates LR item for each position in the given production. Returns the item for the 0-th position.
		const LRItem* create_LR_items_for_production(const ExtPr* ext_pr) {
			LRItem* next = nullptr;
			const std::vector<const ExtSym*>& ext_elements = ext_pr->get_elements();
			typedef typename std::vector<const ExtSym*>::const_reverse_iterator sym_iterator;

			for (std::size_t i = 0, n = ext_elements.size(); i < n + 1; ++i) {
				int pos = n - i;
				const ExtSym* sym = i == 0 ? nullptr : ext_elements[n - i];
				
				CntPtr<LRItem> next_ptr = new LRItem(pos, next, sym, ext_pr);
				m_all_items.push_back(next_ptr);
				next = next_ptr.get();
			}

			return next;
		}

		//Creates LR items for each production of the given nonterminal.
		void create_LR_items_for_nonterminal(const ExtNt* ext_nt) {
			std::vector<const LRItem*> nt_items;

			const std::vector<const ExtPr*>& ext_prs = ext_nt->get_productions();
			for (const ExtPr* ext_pr : ext_prs) {
				const LRItem* first = create_LR_items_for_production(ext_pr);
				nt_items.push_back(first);
			}

			std::size_t sym_index = ext_nt->get_sym_index();
			m_sym_to_items[sym_index] = nt_items;
		}

		//Creates LR items for the given BNF grammar.
		void create_LR_items(const ExtBnf& ext_bnf_grammar) {
			m_sym_to_items.resize(ext_bnf_grammar.get_symbols().size());
			
			const std::vector<const ExtNt*>& ext_nts = ext_bnf_grammar.get_nonterminals();
			for (const ExtNt* ext_nt : ext_nts) create_LR_items_for_nonterminal(ext_nt);

			//Items must be sorted and indexed according to their current symbol. Thus, the items with the same
			//current symbol will be consequent. However, the order must depend not only on symbols:
			//the order of the items with the same current symbols must be definite, i. e. the items
			//in a set must not be sorted by their current symbol only. For this reason additional
			//index is defined for each item, such that items with the same current symbols have consequent
			//indexes. Sets of items can then be sorted by these indexes instead of the symbols.
			std::sort(m_all_items.begin(), m_all_items.end(), std::ptr_fun(&compare_item_sym));
			for (typename std::vector<LRItem*>::size_type i = 0, n = m_all_items.size(); i < n; ++i) {
				m_all_items[i]->set_index(i);
			}
		}

		//Temporary set used by the items_closure() function.
		ExtNtListSet m_closure_nt_set;

		//Calculates the closure of the given set of LR items.
		void items_closure(std::vector<const LRItem*>& items_list) {

			//Find all nonterminals that must already be in the set.
			for (std::size_t i = 0, n = items_list.size(); i < n; ++i) {
				const LRItem* item = items_list[i];
				if (!item->m_pos) {
					const ExtNt* nt = item->m_pr->get_nt();
					m_closure_nt_set.add(item->m_pr->get_nt());
				}
			}

			std::size_t nt_pos = m_closure_nt_set.size();
			std::size_t item_pos = 0;
			while (item_pos < items_list.size()) {

				//Add all nonterminals pointed by the items to the set.
				for (std::size_t n = items_list.size(); item_pos < n; ++item_pos) {
					const LRItem* item = items_list[item_pos];
					const ExtSym* sym = item->m_sym;
					if (sym) {
						const ExtNt* nt = sym->as_nt();
						if (nt) m_closure_nt_set.add(nt);
					}
				}

				//Add all items of new nonterminals to the items list.
				//Since the nonterminals are new, the items will also be new in the list,
				//so it is not necessary to check if an item is already in the list.
				for (std::size_t n = m_closure_nt_set.size(); nt_pos < n; ++nt_pos) {
					const ExtNt* nt = m_closure_nt_set.get(nt_pos);
					const std::vector<const LRItem*>& nt_items = m_sym_to_items[nt->get_sym_index()];
					for (const LRItem* item : nt_items) items_list.push_back(item);
				}
			}

			m_closure_nt_set.clear();
		}

		//Creates and returns a new LR item set, or returns an existing one, if any.
		const LRSet* add_lr_set(const std::vector<const LRItem*>& items_list, const Sym* sym) {
			set_map_iterator it = m_set_map.find(&items_list);
			if (it != m_set_map.end()) return it->second;

			int index = m_owned_states->size();
			CntPtr<State> state_ptr = new State(index, sym);
			m_owned_states->push_back(state_ptr);

			CntPtr<LRSet> lr_set_ptr = new LRSet(state_ptr.get(), items_list);
			LRSet* lr_set = lr_set_ptr.get();

			m_set_list.push_back(lr_set);
			m_set_map.insert(std::make_pair(&lr_set->get_items(), lr_set));
			return lr_set;
		}

		//Calculates the closure of the given LR item set, and adds the resulting set of LR items
		//to the list of LR sets.
		const LRSet* calc_closure_and_add(std::vector<const LRItem*>& items_list, const Sym* sym) {
			items_closure(items_list);
			std::sort(items_list.begin(), items_list.end(), &compare_item_index);
			const LRSet* lr_set = add_lr_set(items_list, sym);
			return lr_set;
		}

		std::vector<const LRItem*> m_derived_items_list;
		std::vector<Shift> m_derived_shifts;
		std::vector<Goto> m_derived_gotos;
		std::vector<const Pr*> m_derived_reduces;

		//Creates LR item sets derived from the given set. 'Derived' means obtained by a transition.
		void create_derived_sets(LRSet* lr_set) {

			const std::vector<const LRItem*>& items = lr_set->get_items();
			std::size_t size = items.size();
			std::size_t pos = 0;

			while (pos < size) {
				const LRItem* item = items[pos];
				const ExtSym* ext_sym = item->m_sym;

				if (ext_sym) {
					//Not the ending position.

					//It must be safe to compare pointers, since one instance per item is created.
					while (pos < size && ext_sym == items[pos]->m_sym) {
						if (const LRItem* next_item = items[pos]->m_next) {
							m_derived_items_list.push_back(next_item);
						}
						++pos;
					}
					
					if (const ExtNt* ext_nt = ext_sym->as_nt()) {
						const Sym* sym = ext_nt->get_nt_obj();
						const LRSet* dest_set = calc_closure_and_add(m_derived_items_list, sym);
						m_derived_gotos.push_back(Goto(ext_nt->get_nt_obj(), dest_set->get_state()));
					} else if (const ExtTr* ext_tr = ext_sym->as_tr()) {
						const Sym* sym = ext_tr->get_tr_obj();
						const LRSet* dest_set = calc_closure_and_add(m_derived_items_list, sym);
						m_derived_shifts.push_back(Shift(ext_tr->get_tr_obj(), dest_set->get_state()));
					} else {
						//impossible. ignore.
					}

					m_derived_items_list.clear();
				} else {
					//Ending position. Add a reduce.
					m_derived_reduces.push_back(item->m_pr->get_pr_obj());
					++pos;
				}
			}

			lr_set->get_state()->init(m_derived_shifts, m_derived_gotos, m_derived_reduces);

			m_derived_shifts.clear();
			m_derived_gotos.clear();
			m_derived_reduces.clear();
		}

		//Print an LR item.
		void print_item(std::ostream& out, const LRItem* item) const {
			out << item->m_pr->get_nt()->get_name();
			out << " :";
			for (int i = 0; i < item->m_pos; ++i) {
				out << " " << item->m_pr->get_elements()[i]->get_name(); 
			}
			out << " *";
			for (int i = item->m_pos, n = item->m_pr->get_elements().size(); i < n; ++i) {
				out << " " << item->m_pr->get_elements()[i]->get_name(); 
			}
			out << "\n";
		}

		//Prints LR sets.
		void print_sets(std::ostream& out) const {
			for (std::size_t i = 0, n = m_set_list.size(); i < n; ++i) {
				const CntPtr<LRSet>& lrset = m_set_list[i];
				out << "=== " << i << " ===\n";
				const std::vector<const LRItem*>& items = lrset->get_items();
				for (std::size_t item_i = 0, item_n = items.size(); item_i < item_n; ++item_i) {
					print_item(out, items[item_i]);
				}
			}
		}

		std::vector<const LRItem*> m_create_tables_items_list;

		//Creates LR tables.
		std::unique_ptr<const LRTables<Traits>> create_LR_tables_0(
			const std::vector<const typename BnfGrammar<Traits>::Nt*>& start_nonterminals,
			bool print)
		{
			std::unique_ptr<const ExtBnf> ext_bnf_grammar = create_ext_grammar(m_bnf_grammar, start_nonterminals);
			create_LR_items(*ext_bnf_grammar.get());

			typedef std::vector<const ExtNt*> ext_nts_type;
			typedef std::vector<const LRItem*> items_type;

			//Create initial sets of items for extended start nonterminals.
			const ext_nts_type& ext_nts = ext_bnf_grammar->get_nonterminals();
			for (const ExtNt* ext_nt : ext_nts) {
				if (!ext_nt->get_nt_obj()) {
					//Extended start nonterminal.

					const items_type& items = m_sym_to_items[ext_nt->get_sym_index()];

					//There must be only one item for an extended start nonterminal.
					const LRItem* item = items[0];
					m_create_tables_items_list.push_back(item);
					const LRSet* lrset = calc_closure_and_add(m_create_tables_items_list, 0);
					m_create_tables_items_list.clear();

					//The item must point to the actual start nonterminal.
					const Nt* start_nt = item->m_sym->as_nt()->get_nt_obj();
					m_start_states.push_back(std::make_pair(start_nt, lrset->get_state()));
				}
			}

			//Create all derived sets.
			std::size_t cur = 0;
			while (cur < m_set_list.size()) {
				CntPtr<LRSet> lr_set = m_set_list[cur];
				create_derived_sets(lr_set.get());
				++cur;
			}

			//Create tables.
			std::unique_ptr<const Tables> tables(new Tables(m_owned_states, m_start_states));
			//(cannot use std::make_unique() - private member).

			if (print) {
				std::cout << "*** LR STATES ***\n";
				std::cout << '\n';
				print_sets(std::cout);
			}

			return tables;
		}

	public:
		//Creates LR tables for the given BNF grammar.
		static std::unique_ptr<const LRTables<Traits>> create_LR_tables(
			const BnfGrammar<Traits>& bnf_grammar,
			const std::vector<const typename BnfGrammar<Traits>::Nt*>& start_nonterminals,
			bool print)
		{
			LRGenerator<Traits> lr_generator(bnf_grammar);
			return lr_generator.create_LR_tables_0(start_nonterminals, print);
		}
	};

	template<class Traits>
	std::unique_ptr<const LRTables<Traits>> create_LR_tables(
		const BnfGrammar<Traits>& bnf_grammar,
		const std::vector<const typename BnfGrammar<Traits>::Nt*>& start_nonterminals,
		bool print)
	{
		return LRGenerator<Traits>::create_LR_tables(bnf_grammar, start_nonterminals, print);
	}

}

#endif//SYN_CORE_LRTABLES_H_INCLUDED
