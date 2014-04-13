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

//BNF Grammar representation classes.

#ifndef SYN_CORE_BNF_H_INCLUDED
#define SYN_CORE_BNF_H_INCLUDED

#include <algorithm>
#include <iterator>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cntptr.h"
#include "commons.h"
#include "noncopyable.h"
#include "util.h"
#include "util_string.h"

namespace synbin {

	template<class Traits>
	class BnfGrammarBuilder;

	//
	//BnfTraits
	//

	template<class NtObjT, class TrObjT, class PrObjT>
	class BnfTraits {
		NONCOPYABLE(BnfTraits);

	public:
		typedef NtObjT NtObj;
		typedef TrObjT TrObj;
		typedef PrObjT PrObj;
	};

	//
	//BnfGrammar
	//

	//BNF Grammar. Consists of nonterminals, terminals and productions.
	template<class TraitsT>
	class BnfGrammar {
		NONCOPYABLE(BnfGrammar);

	public:
		typedef TraitsT Traits;
		typedef typename Traits::NtObj NtObj;
		typedef typename Traits::TrObj TrObj;
		typedef typename Traits::PrObj PrObj;

		friend class BnfGrammarBuilder<Traits>;

	public:
		class Nt;
		class Tr;
		class Pr;

		//
		//Obj
		//

		//Common base class for grammar objects (terminal, nonterminal, production).
		class Obj : public RefCnt {
			NONCOPYABLE(Obj);

		protected:
			Obj(){}
			virtual ~Obj(){}
		};

		//
		//Sym
		//

		//Grammar symbol (may be either terminal or nonterminal).
		class Sym : public Obj {
			NONCOPYABLE(Sym);

			const int m_sym_index;
			const util::String m_name;

		protected:
			Sym(int sym_index, const util::String& name) : m_sym_index(sym_index), m_name(name){}

		public:
			int get_sym_index() const { return m_sym_index; }
			const util::String& get_name() const { return m_name; }

			//as_tr() and as_nt() functions act like a dynamic_cast<>. But using virtual functions seems
			//to be a better solution, since it is easier to find all occurences of calls to these functions
			//in the code than all occurences of dynamic_cast<>s from Sym.
			virtual const Tr* as_tr() const { return nullptr; }
			virtual const Nt* as_nt() const { return nullptr; }
		};

		//
		//Tr
		//

		//Terminal symbol.
		class Tr : public Sym {
			NONCOPYABLE(Tr);

			friend class BnfGrammarBuilder<Traits>;

			const int m_tr_index;
			const TrObj m_tr_obj;

			Tr(int sym_index, int tr_index, const util::String& name, const TrObj tr_obj)
				: Sym(sym_index, name), m_tr_index(tr_index), m_tr_obj(tr_obj){}

		public:
			int get_tr_index() const { return m_tr_index; }
			TrObj get_tr_obj() const { return m_tr_obj; }
			const Tr* as_tr() const override { return this; }
		};

		//
		//Pr
		//

		//Production.
		class Pr : public Obj {
			NONCOPYABLE(Pr);

			friend class BnfGrammarBuilder<Traits>;

			const int m_pr_index;
			const Nt* m_nt;
			const PrObj m_pr_obj;
			const std::vector<const Sym*> m_elements;

			Pr(int pr_index, const Nt* nt, const PrObj pr_obj, const std::vector<const Sym*>& elements)
				: m_pr_index(pr_index), m_nt(nt), m_pr_obj(pr_obj), m_elements(elements){}

		public:
			int get_pr_index() const { return m_pr_index; }
			const Nt* get_nt() const { return m_nt; }
			PrObj get_pr_obj() const { return m_pr_obj; }
			const std::vector<const Sym*>& get_elements() const { return m_elements; }

			void print(std::ostream& out) const {
				const char* sep = "\t";
				for (const Sym* sym : m_elements) {
					out << sep << sym->get_name();
					sep = " ";
				}
				out << "\n";
			}
		};

		//
		//Nt
		//

		//Nonterminal symbol.
		class Nt : public Sym {
			NONCOPYABLE(Nt);

			friend class BnfGrammarBuilder<Traits>;

			const int m_nt_index;
			const NtObj m_nt_obj;
			std::vector<const Pr*> m_productions;

			void set_productions(const std::vector<const Pr*>& productions) { m_productions = productions; }

		public:
			Nt(int sym_index, int nt_index, const util::String& name, const NtObj nt_obj)
				: Sym(sym_index, name), m_nt_index(nt_index), m_nt_obj(nt_obj){}

			int get_nt_index() const { return m_nt_index; }
			NtObj get_nt_obj() const { return m_nt_obj; }
			const std::vector<const Pr*>& get_productions() const { return m_productions; }
			const Nt* as_nt() const override { return this; }

			void print(std::ostream& out) const {
				out << this->get_name() << " :\n";
				for (const Pr* pr : m_productions) pr->print(out);
			}
		};

	private:
		const std::unique_ptr<const std::vector<CntPtr<Sym>>> m_owned_symbols;
		const std::unique_ptr<const std::vector<CntPtr<Pr>>> m_owned_productions;
		std::vector<const Sym*> m_symbols;
		std::vector<const Pr*> m_productions;
		const std::vector<const Tr*> m_terminals;
		const std::vector<const Nt*> m_nonterminals;

		template<class V, class Op>
		static void check_indexes(const V& vector, Op op) {
			for (int i = 0, n = vector.size(); i < n; ++i) {
				int idx = op(vector[i]);
				if (idx != i) throw std::logic_error("Index missmatch!");
			}
		}

		BnfGrammar(
			std::unique_ptr<std::vector<CntPtr<Sym>>> owned_symbols,
			std::unique_ptr<std::vector<CntPtr<Pr>>> owned_productions,
			const std::vector<const Tr*>& terminals,
			const std::vector<const Nt*>& nonterminals)
			: m_owned_symbols(std::move(owned_symbols)),
			m_owned_productions(std::move(owned_productions)),
			m_terminals(terminals),
			m_nonterminals(nonterminals)
		{
			//Create unmodifiable vectors of symbols and productions.

			m_symbols.reserve(m_owned_symbols->size());
			for (const CntPtr<Sym>& sym : *m_owned_symbols) m_symbols.push_back(sym.get());

			m_productions.reserve(m_owned_productions->size());
			for (const CntPtr<Pr>& pr : *m_owned_productions) m_productions.push_back(pr.get());

			//Ensure that indicies are correct.
			check_indexes(m_symbols, std::mem_fun(&Sym::get_sym_index));
			check_indexes(m_terminals, std::mem_fun(&Tr::get_tr_index));
			check_indexes(m_nonterminals, std::mem_fun(&Nt::get_nt_index));
			check_indexes(m_productions, std::mem_fun(&Pr::get_pr_index));
		}

	public:
		const std::vector<const Sym*>& get_symbols() const { return m_symbols; }
		const std::vector<const Nt*>& get_nonterminals() const { return m_nonterminals; }
		const std::vector<const Tr*>& get_terminals() const { return m_terminals; }
		const std::vector<const Pr*>& get_productions() const { return m_productions; }

		//TODO Output user-supplied objects.
		void print(std::ostream& out) const {
			for (const Nt* nt : m_nonterminals) nt->print(out);
		}
	};

	//
	//BnfGrammarBuilder
	//

	//BNF Grammar builder. The BnfGrammar class is unmodifiable. BnfGrammarBuilder is used to configure an instance of BnfGrammar.
	template<class TraitsT>
	class BnfGrammarBuilder {
		NONCOPYABLE(BnfGrammarBuilder);

	public:
		typedef TraitsT Traits;
		typedef typename Traits::NtObj NtObj;
		typedef typename Traits::TrObj TrObj;
		typedef typename Traits::PrObj PrObj;

		typedef BnfGrammar<Traits> Grammar;
		typedef typename Grammar::Sym Sym;
		typedef typename Grammar::Nt Nt;
		typedef typename Grammar::Tr Tr;
		typedef typename Grammar::Pr Pr;

	private:
		//Temporary nonterminal - with a modifiable list of productions.
		class NtTemp : public RefCnt {
			Nt* const m_nt;
			std::vector<const Pr*> m_productions;

		public:
			NtTemp(Nt* nt) : m_nt(nt){}

			Nt* get_nt() const { return m_nt; }
			const std::vector<const Pr*>& get_productions() const { return m_productions; }

			void add_production(const Pr* pr) { m_productions.push_back(pr); }
		};

		//These objects are owned by this one.
		std::unique_ptr<std::vector<CntPtr<Sym>>> m_owned_symbols;
		std::unique_ptr<std::vector<CntPtr<Pr>>> m_owned_productions;
		std::vector<CntPtr<NtTemp>> m_nt_temps;

		//Vectors contain non-owning pointers.
		std::vector<const Pr*> m_const_productions;
		std::vector<const Tr*> m_terminals;
		std::vector<const Nt*> m_nonterminals;

		bool m_grammar_created;

		void assert_grammar_not_created() const {
			if (m_grammar_created) throw std::logic_error("Grammar has already been created");
		}

	public:
		BnfGrammarBuilder()
			: m_grammar_created(false),
			m_owned_symbols(make_unique1<std::vector<CntPtr<Sym>>>()),
			m_owned_productions(make_unique1<std::vector<CntPtr<Pr>>>())
		{}

		//Creates a new terminal symbol.
		const Tr* create_terminal(const util::String& name, const TrObj tr_obj) {
			assert_grammar_not_created();

			int sym_index = m_owned_symbols->size();
			int tr_index = m_terminals.size();

			CntPtr<Tr> tr_ptr = new Tr(sym_index, tr_index, name, tr_obj);
			m_owned_symbols->push_back(tr_ptr);
			m_terminals.push_back(tr_ptr.get());
			return tr_ptr.get();
		}

		//Creates a new nonterminal symbol.
		const Nt* create_nonterminal(const util::String& name, const NtObj nt_obj) {
			assert_grammar_not_created();

			int sym_index = m_owned_symbols->size();
			int nt_index = m_nonterminals.size();

			CntPtr<Nt> nt_ptr = new Nt(sym_index, nt_index, name, nt_obj);
			Nt* nt = nt_ptr.get();

			m_owned_symbols->push_back(nt_ptr);
			m_nonterminals.push_back(nt);
			
			CntPtr<NtTemp> nt_temp_ptr = new NtTemp(nt);
			m_nt_temps.push_back(nt_temp_ptr);

			return nt;
		}

		//Adds a new production to the current nonterminal.
		//
		//WARN Each element must be either a terminal or a nonterminal object created by create_terminal() or create_nonterminal() function,
		//respectively of THIS BnfGrammarBuilder object, i. e. the symbol must belong to this grammar. Otherwise, the behaviour is undefined.
		const Pr* add_production(const Nt* nt, const PrObj pr_obj, const std::vector<const Sym*>& elements) {
			assert_grammar_not_created();

			typename std::vector<Nt*>::size_type nt_index = nt->get_nt_index();
			if (nt_index >= m_nonterminals.size() || m_nonterminals[nt_index] != nt) {
				throw std::logic_error("Wrong nonterminal has been passed!");
			}

			int pr_index = m_owned_productions->size();
			CntPtr<Pr> pr_ptr = new Pr(pr_index, nt, pr_obj, elements);
			const Pr* pr = pr_ptr.get();
			m_owned_productions->push_back(pr_ptr);
			m_nt_temps[nt_index]->add_production(pr);

			return pr;
		}

		//Creates a BnfGrammar instance. After calling this method, this builder objects cannot be used any more.
		std::unique_ptr<const BnfGrammar<Traits>> create_grammar() {
			assert_grammar_not_created();
			
			//Create unmodifiable nonterminal symbols.
			for (const CntPtr<NtTemp>& nt_temp : m_nt_temps) {
				const std::vector<const Pr*>& productions = nt_temp->get_productions();
				if (productions.empty()) throw std::runtime_error("No productions are defined!");
				nt_temp->get_nt()->set_productions(productions);
			}

			//std::make_unique cannot be used, because the constructor is private.
			std::unique_ptr<Grammar> result(new Grammar(
				std::move(m_owned_symbols), std::move(m_owned_productions), m_terminals, m_nonterminals));
			m_grammar_created = true;

			return std::move(result);
		}
	};

}

#endif//SYN_CORE_BNF_H_INCLUDED
