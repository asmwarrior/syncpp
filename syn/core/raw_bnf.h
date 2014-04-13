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

//Raw BNF - templates for building a BNF grammar from a simplified text notation.
//Used to define the BNF grammar of EBNF grammar in a C++ module.

#ifndef SYN_CORE_RAW_BNF_H_INCLUDED
#define SYN_CORE_RAW_BNF_H_INCLUDED

#include <algorithm>
#include <cctype>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "noncopyable.h"
#include "util_string.h"

//TODO Use Standard Library algorithms for container manipulations where possible.

namespace synbin {
	namespace raw_bnf {

		class NullObj {};
		typedef const NullObj* NullType;

		//
		//RawBnfParser
		//

		template<class TraitsT>
		class RawBnfParser {
			NONCOPYABLE(RawBnfParser);
			RawBnfParser() = delete;

		public:
			typedef TraitsT Traits;
			typedef typename Traits::NtObj NtObj;
			typedef typename Traits::TrObj TrObj;
			typedef typename Traits::PrObj PrObj;

		private:
			typedef synbin::BnfGrammar<Traits> BnfGrammar;
			typedef synbin::BnfGrammarBuilder<Traits> BnfBuilder;
			typedef typename BnfGrammar::Tr BnfTr;
			typedef typename BnfGrammar::Nt BnfNt;
			typedef typename BnfGrammar::Sym BnfSym;

			//
			//RawPr
			//
			
			struct RawPr {
				std::vector<util::String> m_elements;
				PrObj m_pr_obj;
			};

			//
			//RawNt
			//

			struct RawNt {
				util::String m_name;
				std::vector<RawPr> m_productions;
				const BnfNt* m_bnf_nt;
			};

		public:
			//
			//RawTr
			//

			struct RawTr {
				const char* const m_name;
				const TrObj m_tr_obj;
			};

			//
			//RawRule
			//

			struct RawRule {
				const char* const m_text;
				const PrObj m_pr_obj;
			};

		private:
			inline static bool is_raw_name_start(int c) { return std::isalpha(c) || '_' == c; }
			inline static bool is_raw_name_part(int c) { return std::isalnum(c) || '_' == c; }

			static bool is_raw_name(const std::string& str) {
				if (str.empty()) return false;

				char c = str[0];
				if (!is_raw_name_start(c)) return false;

				std::string::const_iterator it = std::find_if(
					str.begin(),
					str.end(),
					std::not1(std::ptr_fun(is_raw_name_part)));

				return str.end() == it;
			}

			static inline util::String iter_to_str(
				const std::istream_iterator<std::string> iter,
				const std::istream_iterator<std::string> end_iter)
			{
				if (iter == end_iter) return util::String();
				return util::String(*iter);
			}

			static inline util::String str_to_action(const std::string& str) {
				if (str.empty()) throw std::runtime_error("Syntax error: action expected");

				std::string::size_type len = str.size();
				if (str[0] != '{' || str[len - 1] != '}') {
					throw std::runtime_error("Syntax error: action expected");
				}
		
				std::string action_name = str.substr(1, len - 2);
				if (!is_raw_name(action_name)) throw std::runtime_error("Syntax error: action expected");

				return util::String(action_name);
			}

			static void parse_raw_production(
				const RawRule* raw_rule,
				const PrObj zero_pr_obj,
				RawNt& raw_nt)
			{
				const std::string& text_str = raw_rule->m_text;
				std::istringstream in(text_str);
				std::istream_iterator<std::string> iter(in);
				const std::istream_iterator<std::string> end_iter;

				std::vector<util::String> elements;
				while (iter != end_iter) {
					const std::string& str = *iter;
					if (!is_raw_name(str)) throw std::runtime_error("Structure error : name expected");
					elements.push_back(util::String(str));
					++iter;
				}

				//To avoid copying of vector in RawPr, default RawPr is first added,
				//and then initialized by reference.
				raw_nt.m_productions.push_back(RawPr());
				RawPr& raw_pr = raw_nt.m_productions.back();
				raw_pr.m_pr_obj = raw_rule->m_pr_obj;
				raw_pr.m_elements = elements;
			}

			static const RawRule* parse_raw_nonterminal(
				BnfBuilder& bnf_builder,
				const RawRule* raw_rule,
				const PrObj zero_pr_obj,
				std::map<const util::String, RawNt>& raw_nt_map,
				const std::map<const util::String, const BnfTr*>& bnf_tr_map)
			{
				if (zero_pr_obj != raw_rule->m_pr_obj) {
					throw std::runtime_error("Structure error : expected zero action");
				}
				if (!is_raw_name(raw_rule->m_text)) {
					throw std::runtime_error("Structure error : expected nonterminal name");
				}

				util::String nt_name(raw_rule->m_text);
				if (bnf_tr_map.count(nt_name)) {
					throw std::runtime_error("Nonterminal has the same name as a terminal!");
				}
				
				RawNt& raw_nt = raw_nt_map[nt_name];
				if (!raw_nt.m_name.empty()) throw std::runtime_error("Duplicated nonterminal!");
				raw_nt.m_name = nt_name;
				raw_nt.m_bnf_nt = bnf_builder.create_nonterminal(nt_name, 0);

				++raw_rule;
				while (raw_rule->m_text && zero_pr_obj != raw_rule->m_pr_obj) {
					parse_raw_production(raw_rule, zero_pr_obj, raw_nt);
					++raw_rule;
				}

				return raw_rule;
			}

			static void parse_raw_grammar(
				BnfBuilder& bnf_builder,
				const RawRule* raw_rules,
				const PrObj zero_pr_obj,
				std::map<const util::String, RawNt>& raw_nt_map,
				const std::map<const util::String, const BnfTr*>& bnf_tr_map)
			{
				const RawRule* current_rule = raw_rules;
				while (current_rule->m_text) {
					current_rule = parse_raw_nonterminal(
						bnf_builder, current_rule, zero_pr_obj, raw_nt_map, bnf_tr_map);
				}

			}

			static void create_bnf_production(
				BnfBuilder& bnf_builder,
				const std::map<const util::String, const BnfSym*>& bnf_sym_map,
				const RawNt& raw_nt,
				const RawPr& raw_pr)
			{
				std::vector<const BnfSym*> bnf_elements;

				for (const util::String& name : raw_pr.m_elements) {
					typename std::map<const util::String, const BnfSym*>::const_iterator bnf_sym_it =
						bnf_sym_map.find(name);

					if (bnf_sym_it == bnf_sym_map.end()) throw std::runtime_error("Unknown symbol");
					const BnfSym* bnf_sym = bnf_sym_it->second;
					bnf_elements.push_back(bnf_sym);
				}

				bnf_builder.add_production(raw_nt.m_bnf_nt, raw_pr.m_pr_obj, bnf_elements);
			}

			static void create_bnf_productions(
				BnfBuilder& bnf_builder,
				const std::map<const util::String, const BnfSym*>& bnf_sym_map,
				const RawNt& raw_nt)
			{
				for (const RawPr& raw_pr : raw_nt.m_productions) {
					create_bnf_production(bnf_builder, bnf_sym_map, raw_nt, raw_pr);
				}
			}

		public:
			static std::unique_ptr<const BnfGrammar> raw_grammar_to_bnf(
				const RawTr* raw_tokens,
				const RawRule* raw_rules,
				const PrObj zero_pr_obj)
			{
				BnfBuilder bnf_builder;

				//Create BNF terminals.
				std::map<const util::String, const BnfTr*> bnf_tr_map;
				for (const RawTr* raw_token = raw_tokens; raw_token->m_name; ++raw_token) {
					util::String name(raw_token->m_name);
					const BnfTr*& bnf_tr = bnf_tr_map[name];
					if (bnf_tr) throw std::runtime_error("Duplicated token!");
					bnf_tr = bnf_builder.create_terminal(name, raw_token->m_tr_obj);
				}

				//Parse RAW grammar, create BNF nonterminals.
				std::map<const util::String, RawNt> raw_nt_map;
				parse_raw_grammar(bnf_builder, raw_rules, zero_pr_obj, raw_nt_map, bnf_tr_map);

				//Create BNF symbol map.
				std::map<const util::String, const BnfSym*> bnf_sym_map;
				for (const typename std::map<const util::String, const BnfTr*>::value_type& value : bnf_tr_map) {
					bnf_sym_map[value.first] = value.second;
				}

				typedef typename std::map<const util::String, RawNt>::value_type nt_map_value_type;
				for (const nt_map_value_type& value : raw_nt_map) {
					bnf_sym_map[value.first] = value.second.m_bnf_nt;
				}

				//Create BNF productions.
				for (const nt_map_value_type& value : raw_nt_map) {
					create_bnf_productions(bnf_builder, bnf_sym_map, value.second);
				}

				//Create BNF grammar.
				return bnf_builder.create_grammar();
			}
		};
	
	}
}

#endif//SYN_CORE_RAW_BNF_H_INCLUDED
