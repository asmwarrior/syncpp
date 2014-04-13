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

//Implementation of ConcreteLRResult class and generate_LR_tables() function.

#include <memory>

#include "concretelrgen.h"
#include "converter_res.h"
#include "lrtables.h"

using std::unique_ptr;

namespace ns = synbin;
namespace util = ns::util;

//
//ConcreteLRResult
//

ns::ConcreteLRResult::ConcreteLRResult(
	ConversionResult* conversion_result,
	unique_ptr<const ConcreteLRTables> lr_tables)
	: m_bnf_grammar(std::move(conversion_result->m_bnf_grammar)),
	m_common_heap(std::move(conversion_result->m_common_heap)),
	m_nts(std::move(conversion_result->m_nts)),
	m_name_tokens(std::move(conversion_result->m_name_tokens)),
	m_str_tokens(std::move(conversion_result->m_str_tokens)),
	m_primitive_types(std::move(conversion_result->m_primitive_types)),
	m_string_literal_type(std::move(conversion_result->m_string_literal_type)),
	m_class_type_count(conversion_result->m_class_type_count),
	m_lr_tables(std::move(lr_tables))
{}

const ns::ConcreteBNF* ns::ConcreteLRResult::get_bnf_grammar() const {
	return m_bnf_grammar.get();
}

const ns::ConcreteLRTables* ns::ConcreteLRResult::get_lr_tables() const {
	return m_lr_tables.get();
}

const std::vector<const ns::NtDescriptor*>& ns::ConcreteLRResult::get_nts() const {
	return *m_nts;
}

const std::vector<const ns::NameTrDescriptor*>& ns::ConcreteLRResult::get_name_tokens() const {
	return *m_name_tokens;
}

const std::vector<const ns::StrTrDescriptor*>& ns::ConcreteLRResult::get_str_tokens() const {
	return *m_str_tokens;
}

const std::vector<const ns::PrimitiveTypeDescriptor*>& ns::ConcreteLRResult::get_primitive_types() const {
	return *m_primitive_types;
}

util::MPtr<const ns::PrimitiveTypeDescriptor> ns::ConcreteLRResult::get_string_literal_type() const {
	return m_string_literal_type;
}

std::size_t ns::ConcreteLRResult::get_class_type_count() const {
	return m_class_type_count;
}

//
//generate_LR_tables()
//

unique_ptr<ns::ConcreteLRResult> ns::generate_LR_tables(
	const CommandLine& command_line,
	unique_ptr<ConversionResult> conversion_result)
{
	unique_ptr<const ConcreteLRTables> lr_tables(LRGenerator<ConcreteBNFTraits>::create_LR_tables(
		*conversion_result->get_bnf_grammar(),
		conversion_result->get_start_nts(),
		command_line.is_verbose()));

	return unique_ptr<ConcreteLRResult>(new ConcreteLRResult(conversion_result.get(), std::move(lr_tables)));
}

