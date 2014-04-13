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

//Definition of the class returned by the Grammar Parser.

#ifndef SYN_CORE_GRM_PARSER_RES_H_INCLUDED
#define SYN_CORE_GRM_PARSER_RES_H_INCLUDED

#include <memory>

#include "ebnf__dec.h"
#include "noncopyable.h"
#include "util_mptr.h"

namespace synbin {

	class GrammarBuildingResult;

	class GrammarParsingResult {
		NONCOPYABLE(GrammarParsingResult);

		friend class GrammarBuildingResult;

		std::unique_ptr<util::MRoot<ebnf::Grammar>> m_grammar_root;
		std::unique_ptr<util::MHeap> m_const_heap;

	public:
		GrammarParsingResult(
			std::unique_ptr<util::MRoot<ebnf::Grammar>> grammar_root,
			std::unique_ptr<util::MHeap> const_heap);

		util::MPtr<ebnf::Grammar> get_grammar() const;
		util::MHeap* get_const_heap() const;
	};

}

#endif//SYN_CORE_GRM_PARSER_RES_H_INCLUDED
