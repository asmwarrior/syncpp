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

//Definition of the class produced by the LR tables generation step.

#ifndef SYN_CORE_CONCRETELRGEN_RES_H_INCLUDED
#define SYN_CORE_CONCRETELRGEN_RES_H_INCLUDED

#include <cstddef>
#include <memory>
#include <vector>

#include "concrete_lr.h"
#include "descriptor.h"
#include "descriptor_type.h"
#include "lrtables.h"
#include "noncopyable.h"

namespace synbin {

	class ConversionResult;

	//
	//ConcreteLRResult
	//

	class ConcreteLRResult {
		NONCOPYABLE(ConcreteLRResult);

		std::unique_ptr<const ConcreteBNF> m_bnf_grammar;
		std::unique_ptr<util::MHeap> m_common_heap;
		std::unique_ptr<const std::vector<const NtDescriptor*>> m_nts;
		std::unique_ptr<const std::vector<const NameTrDescriptor*>> m_name_tokens;
		std::unique_ptr<const std::vector<const StrTrDescriptor*>> m_str_tokens;
		std::unique_ptr<const std::vector<const PrimitiveTypeDescriptor*>> m_primitive_types;
		const util::MPtr<const PrimitiveTypeDescriptor> m_string_literal_type;
		const std::size_t m_class_type_count;
		std::unique_ptr<const ConcreteLRTables> m_lr_tables;

	public:
		ConcreteLRResult(
			ConversionResult* conversion_result,
			std::unique_ptr<const ConcreteLRTables> lr_tables);
	
		const ConcreteBNF* get_bnf_grammar() const;
		const ConcreteLRTables* get_lr_tables() const;
		const std::vector<const NtDescriptor*>& get_nts() const;
		const std::vector<const NameTrDescriptor*>& get_name_tokens() const;
		const std::vector<const StrTrDescriptor*>& get_str_tokens() const;
		const std::vector<const PrimitiveTypeDescriptor*>& get_primitive_types() const;
		util::MPtr<const PrimitiveTypeDescriptor> get_string_literal_type() const;
		std::size_t get_class_type_count() const;
	};

}

#endif//SYN_CORE_CONCRETELRGEN_RES_H_INCLUDED
