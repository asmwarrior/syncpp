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

//Declarations related to the EBNF-to-BNF converter.

#ifndef SYN_CORE_CONVERTER_DEC_H_INCLUDED
#define SYN_CORE_CONVERTER_DEC_H_INCLUDED

#include <memory>
#include <vector>

#include "cmdline.h"
#include "concrete_bnf.h"
#include "converter_res.h"
#include "descriptor.h"
#include "descriptor_type.h"
#include "ebnf__dec.h"
#include "types.h"
#include "util_mptr.h"

namespace synbin {

	namespace conv {
		class ConvSym;
		class ConvNt;
		class ConvTr;
		class ConvPr;
	}

	class ConverterFacade;
	class ConvPrBuilderFacade;

	std::unique_ptr<ConversionResult> convert_EBNF_to_BNF(
		bool verbose_output,
		std::unique_ptr<GrammarBuildingResult> building_result);

}

#endif//SYN_CORE_CONVERTER_DEC_H_INCLUDED
