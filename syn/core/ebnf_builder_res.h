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

//Definition of the result type produced by the EBNF Builder.

#ifndef SYN_CORE_EBNF_BUILDER_RES_H_INCLUDED
#define SYN_CORE_EBNF_BUILDER_RES_H_INCLUDED

#include <cstddef>
#include <memory>
#include <vector>

#include "conversion.h"
#include "ebnf__def.h"
#include "noncopyable.h"
#include "types.h"
#include "util_mptr.h"

namespace synbin {

	class GrammarParsingResult;
	class ConversionResult;

	//
	//GrammarBuildingResult
	//

	class GrammarBuildingResult {
		NONCOPYABLE(GrammarBuildingResult);

		friend class ConversionResult;

		std::unique_ptr<util::MRoot<ebnf::Grammar>> m_grammar_root;
		std::unique_ptr<util::MHeap> m_common_heap;
		std::unique_ptr<const std::vector<const types::PrimitiveType*>> m_primitive_types;
		std::unique_ptr<const std::vector<PartClassTag>> m_part_class_tags;
		const util::MPtr<const types::Type> m_string_literal_type;

	public:
		GrammarBuildingResult(
			GrammarParsingResult* parsing_result,
			std::unique_ptr<util::MContainer<const types::Type>> type_container,
			std::unique_ptr<const std::vector<const types::PrimitiveType*>> primitive_types,
			std::unique_ptr<const std::vector<PartClassTag>> part_class_tags,
			util::MPtr<const types::Type> string_literal_type);

		util::MPtr<ebnf::Grammar> get_grammar() const;
		util::MHeap* get_common_heap() const;
		const std::vector<const types::PrimitiveType*>& get_primitive_types() const;
		util::MPtr<const types::Type> get_string_literal_type() const;
	};

}

#endif//SYN_CORE_EBNF_BUILDER_RES_H_INCLUDED
