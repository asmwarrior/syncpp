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

//Unit tests for the Raw BNF functionality.

#include "core/bnf.h"
#include "core/raw_bnf.h"

#include "unittest.h"

namespace ns = synbin;
namespace raw = ns::raw_bnf;

namespace {

	enum Tokens {
		NUMBER,
		PLUS,
		MINUS,
		OPAREN,
		CPAREN
	};

	enum Actions {
		ACT_NONE,
		ACT_PLUS,
		ACT_MINUS,
		ACT_COPY,
		ACT_NUMBER,
		ACT_PARENTHESES
	};

	class RawTraits : public ns::BnfTraits<raw::NullType, Tokens, Actions>{};
	typedef raw::RawBnfParser<RawTraits> RawPrs;

	typedef ns::BnfGrammar<RawTraits> BnfGrm;

	const RawPrs::RawTr g_raw_tokens[] = {
		{ "NUMBER", NUMBER },
		{ "PLUS", PLUS },
		{ "MINUS", MINUS },
		{ "OPAREN", OPAREN },
		{ "CPAREN", CPAREN },
		{ 0, Tokens(0) }
	};

}

namespace {//anonymous

TEST(undefined_symbol) {
	static const RawPrs::RawRule g_raw_rules[] =
	{
		{ "Expr", ACT_NONE },
		{ "UnknownSymbol", ACT_NUMBER },
		{ 0, ACT_NONE }
	};

	try {
		std::unique_ptr<const BnfGrm> bnf_grammar = RawPrs::raw_grammar_to_bnf(g_raw_tokens, g_raw_rules, ACT_NONE);
		fail();
	} catch (std::exception& e) {
		assertEquals("Unknown symbol", e.what());
	}
}

TEST(terminals) {

	static const RawPrs::RawRule g_raw_rules[] =
	{
		{ "Expr", ACT_NONE },
		{ "NUMBER", ACT_NUMBER },
		{ 0, ACT_NONE }
	};

	std::unique_ptr<const BnfGrm> bnf_grammar = RawPrs::raw_grammar_to_bnf(g_raw_tokens, g_raw_rules, ACT_NONE);
	assertNotNull(bnf_grammar.get());

	const std::vector<const BnfGrm::Tr*>& trs = bnf_grammar->get_terminals();
	assertEquals(5, trs.size());
	assertTrue(NUMBER == trs[0]->get_tr_obj());
	assertTrue(PLUS == trs[1]->get_tr_obj());
	assertTrue(MINUS == trs[2]->get_tr_obj());
	assertTrue(OPAREN == trs[3]->get_tr_obj());
	assertTrue(CPAREN == trs[4]->get_tr_obj());
}

TEST(nonterminals) {

	static const RawPrs::RawRule g_raw_rules[] =
	{
		{ "Expr", ACT_NONE },
		{ "Expr PLUS Term", ACT_PLUS },
		{ "Term", ACT_COPY },
		
		{ "Term", ACT_NONE },
		{ "NUMBER", ACT_NUMBER },
		{ "OPAREN Expr CPAREN", ACT_PARENTHESES },
		{ "MINUS Term", ACT_MINUS },
		
		{ 0, ACT_NONE }
	};

	std::unique_ptr<const BnfGrm> bnf_grammar = RawPrs::raw_grammar_to_bnf(g_raw_tokens, g_raw_rules, ACT_NONE);
	assertNotNull(bnf_grammar.get());

	const std::vector<const BnfGrm::Tr*>& trs = bnf_grammar->get_terminals();
	assertEquals(5, trs.size());
	const BnfGrm::Tr* const number = trs[0];
	const BnfGrm::Tr* const plus = trs[1];
	const BnfGrm::Tr* const minus = trs[2];
	const BnfGrm::Tr* const oparen = trs[3];
	const BnfGrm::Tr* const cparen = trs[4];

	const std::vector<const BnfGrm::Nt*>& nts = bnf_grammar->get_nonterminals();
	assertEquals(2, nts.size());

	const BnfGrm::Nt* const expr = nts[0];
	const BnfGrm::Nt* const term = nts[1];

	assertEquals("Expr", expr->get_name().str());
	assertEquals("Term", term->get_name().str());

	const std::vector<const BnfGrm::Pr*>& expr_prs = expr->get_productions();
	assertEquals(2, expr_prs.size());
	
	assertEquals(3, expr_prs[0]->get_elements().size());
	assertEquals(expr, expr_prs[0]->get_elements()[0]);
	assertEquals(plus, expr_prs[0]->get_elements()[1]);
	assertEquals(term, expr_prs[0]->get_elements()[2]);

	assertEquals(1, expr_prs[1]->get_elements().size());
	assertEquals(term, expr_prs[1]->get_elements()[0]);

	const std::vector<const BnfGrm::Pr*>& term_prs = term->get_productions();
	assertEquals(3, term_prs.size());

	assertEquals(1, term_prs[0]->get_elements().size());
	assertEquals(number, term_prs[0]->get_elements()[0]);

	assertEquals(3, term_prs[1]->get_elements().size());
	assertEquals(oparen, term_prs[1]->get_elements()[0]);
	assertEquals(expr, term_prs[1]->get_elements()[1]);
	assertEquals(cparen, term_prs[1]->get_elements()[2]);

	assertEquals(2, term_prs[2]->get_elements().size());
	assertEquals(minus, term_prs[2]->get_elements()[0]);
	assertEquals(term, term_prs[2]->get_elements()[1]);
}

}
