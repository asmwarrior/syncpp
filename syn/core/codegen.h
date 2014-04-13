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

//Code generator definitions.

#ifndef SYN_CORE_CODEGEN_H_INCLUDED
#define SYN_CORE_CODEGEN_H_INCLUDED

#include <ostream>
#include <vector>

#include "cmdline.h"
#include "concrete_bnf.h"
#include "concretelrgen_res.h"
#include "descriptor.h"
#include "lrtables.h"

namespace synbin {

	//Generates result files for given LR tables.
	void generate_result_files(
		const CommandLine& command_line,
		std::unique_ptr<const ConcreteLRResult> lr_result);

}

#endif//SYN_CORE_CODEGEN_H_INCLUDED
