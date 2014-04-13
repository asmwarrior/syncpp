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

//Definition of concrete LR tables generation function.

#ifndef SYN_CORE_CONCRETELRGEN_H_INCLUDED
#define SYN_CORE_CONCRETELRGEN_H_INCLUDED

#include <memory>

#include "cmdline.h"
#include "concretelrgen_res.h"

namespace synbin {

	std::unique_ptr<ConcreteLRResult> generate_LR_tables(
		const CommandLine& command_line,
		std::unique_ptr<ConversionResult> converting_result);

}

#endif//SYN_CORE_CONCRETELRGEN_H_INCLUDED
