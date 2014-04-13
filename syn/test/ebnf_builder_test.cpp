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

//Utilities for EBNF Grammar Builder testing, implementation.

#include <memory>

#include "core/util_string.h"

#include "ebnf_builder_test.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace prs = ns::grm_parser;
namespace util = ns::util;

namespace tst = syn_test;

using std::unique_ptr;

tst::AbstractHelper::AbstractHelper(const char* grammar_text, BuildFn build_fn)
	: m_build_fn(build_fn)
{
	std::istringstream in(grammar_text);
	util::String file_name("test");
	m_parsing_result = synbin::grm_parser::parse_grammar(in, file_name);

	m_grammar = m_parsing_result->get_grammar().get();
	m_gate = std::unique_ptr<synbin::EBNF_BuilderTestGate>(new ns::EBNF_BuilderTestGate(m_parsing_result.get()));

	static BuildFn all_build_fns[] = {
		&ns::EBNF_BuilderTestGate::install_extensions,
		&ns::EBNF_BuilderTestGate::register_names,
		&ns::EBNF_BuilderTestGate::resolve_name_references,
		&ns::EBNF_BuilderTestGate::verify_attributes,
		&ns::EBNF_BuilderTestGate::calculate_is_void,
		&ns::EBNF_BuilderTestGate::verify_recursion,
		&ns::EBNF_BuilderTestGate::calculate_general_types,
		&ns::EBNF_BuilderTestGate::calculate_types,
		0
	};

	//Call all build functions that precede the passed one.
	for (BuildFn* pfn = all_build_fns; *pfn && *pfn != build_fn; ++pfn) {
		BuildFn fn = *pfn;
		(m_gate.get()->*fn)();
	}
}

void tst::AbstractHelper::test_ok() {
	(m_gate.get()->*m_build_fn)();
}

void tst::AbstractHelper::test_fail(const char* error_msg) {
	try {
		test_ok();
		fail();
	} catch (const synbin::Exception& e) {
		assertEquals(error_msg, e.message());
	}
}

const ebnf::Grammar* tst::AbstractHelper::get_grammar() const {
	return m_grammar;
}
