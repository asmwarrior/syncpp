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

//Unit tests for the EBNF Grammar Builder related to recursion detection.

#include <istream>
#include <memory>
#include <sstream>
#include <string>

#include "core/ebnf_builder.h"
#include "core/grm_parser.h"
#include "core/grm_parser_impl.h"

#include "ebnf_builder_test.h"

#include "unittest.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace prs = ns::grm_parser;

namespace {

	typedef ebnf::NonterminalDeclaration NtDecl;

	class Helper : public syn_test::AbstractHelper {
	public:
		Helper(const char* grammar_text)
			: AbstractHelper(grammar_text, &ns::EBNF_BuilderTestGate::verify_recursion)
		{}
	};

	void test_ok(const char* grammar_text) {
		Helper helper(grammar_text);
		helper.test_ok();
	}

	void test_fail(const char* grammar_text, const char* error_msg) {
		Helper helper(grammar_text);
		helper.test_fail(error_msg);
	}

}

namespace {//anonymous

TEST(ok__no_recursion) {
	test_ok("X : Y ; Y : Z ; Z : A ; A : B ; B : C ; C : {FF} ;");
}

TEST(ok__simple_recursion) {
	test_ok("%token ID{str}; X : ID | Y ; Y : ID | Z ; Z : ID | X ;");
}

TEST(ok__void_loop_recursion) {
	test_ok("X : A | Y ; Y : B | Z ; Z : C | X+ ; A : ; B : ; C : ;");
}

TEST(fail__nonvoid_loop_recursion) {
	test_fail("%token ID{str}; X : Y ; Y : ID | Z ; Z : X+ ;", "Recursion through loop: X Y Z");
}

}
