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

//Unit tests for EBNF Grammar Builder: common definitions.

#ifndef SYN_TEST_EBNF_BUILDER_TEST_H_INCLUDED
#define SYN_TEST_EBNF_BUILDER_TEST_H_INCLUDED

#include <memory>
#include <sstream>
#include <string>

#include "core/commons.h"
#include "core/ebnf_builder.h"
#include "core/grm_parser.h"
#include "core/util_mptr.h"

#include "unittest.h"

namespace syn_test {

	//
	//AbstractHelper
	//

	//Unit test helper.
	class AbstractHelper {
		NONCOPYABLE(AbstractHelper);

		typedef void (synbin::EBNF_BuilderTestGate::*BuildFn)();

		std::unique_ptr<synbin::GrammarParsingResult> m_parsing_result;
		const synbin::ebnf::Grammar* m_grammar;
		std::unique_ptr<synbin::EBNF_BuilderTestGate> m_gate;
		const BuildFn m_build_fn;

	public:
		AbstractHelper(const char* grammar_text, BuildFn build_fn);

		void test_ok();
		void test_fail(const char* error_msg);

		const synbin::ebnf::Grammar* get_grammar() const;
	};

}

#endif//SYN_TEST_EBNF_BUILDER_TEST_H_INCLUDED
