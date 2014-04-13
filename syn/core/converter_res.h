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

//Definition of the class ConversionResult, produced by the EBNF-to-BNF conversion procedure.

#ifndef SYN_CORE_CONVERTER_RES_H_INCLUDED
#define SYN_CORE_CONVERTER_RES_H_INCLUDED

#include <cstddef>
#include <memory>
#include <vector>

#include "concrete_bnf.h"
#include "descriptor.h"
#include "descriptor_type.h"
#include "noncopyable.h"
#include "util_mptr.h"

namespace synbin {

	class GrammarBuildingResult;
	class ConcreteLRResult;

	//
	//ConversionResult
	//

	class ConversionResult {
		NONCOPYABLE(ConversionResult);

		friend class ConcreteLRResult;

		std::unique_ptr<util::MHeap> m_common_heap;
		std::unique_ptr<const ConcreteBNF> m_bnf_grammar;
		std::unique_ptr<const std::vector<const ConcreteBNF::Nt*>> m_start_nts;
		std::unique_ptr<const std::vector<const NtDescriptor*>> m_nts;
		std::unique_ptr<const std::vector<const NameTrDescriptor*>> m_name_tokens;
		std::unique_ptr<const std::vector<const StrTrDescriptor*>> m_str_tokens;
		std::unique_ptr<const std::vector<const PrimitiveTypeDescriptor*>> m_primitive_types;
		util::MPtr<const PrimitiveTypeDescriptor> m_string_literal_type;
		const std::size_t m_class_type_count;

	public:
		ConversionResult(
			GrammarBuildingResult* building_result,
			std::unique_ptr<const ConcreteBNF> bnf_grammar,
			std::unique_ptr<const std::vector<const ConcreteBNF::Nt*>> start_nts,
			std::unique_ptr<const std::vector<const NtDescriptor*>> nts,
			std::unique_ptr<const std::vector<const NameTrDescriptor*>> name_tokens,
			std::unique_ptr<const std::vector<const StrTrDescriptor*>> str_tokens,
			std::unique_ptr<const std::vector<const PrimitiveTypeDescriptor*>> primitive_types,
			util::MPtr<const PrimitiveTypeDescriptor> string_literal_type,
			std::size_t class_type_count);

		const ConcreteBNF* get_bnf_grammar() const;
		const std::vector<const ConcreteBNF::Nt*>& get_start_nts() const;
		const std::vector<const NameTrDescriptor*>& get_name_tokens() const;
		const std::vector<const StrTrDescriptor*>& get_str_tokens() const;
		const std::vector<const PrimitiveTypeDescriptor*>& get_primitive_types() const;
	};

}

#endif//SYN_CORE_CONVERTER_RES_H_INCLUDED
