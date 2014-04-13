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

//Core entry point.

#include <malloc.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <utility>

#include "cmdline.h"
#include "codegen.h"
#include "commons.h"
#include "concretelrgen.h"
#include "converter__dec.h"
#include "ebnf__imp.h"
#include "ebnf_builder.h"
#include "grm_parser.h"
#include "lrtables.h"
#include "main.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace prs = ns::grm_parser;
namespace ebnf = ns::ebnf;
namespace util = ns::util;

using std::unique_ptr;

using util::String;

using util::MRoot;
using util::MHeap;
using util::MContainer;
using util::MPtr;

const std::string ns::g_empty_std_string;

namespace {

	unique_ptr<ns::GrammarParsingResult> parse_grammar(const ns::CommandLine* command_line) {
		String file_name(command_line->get_in_file());

		std::ifstream in;
		in.open(file_name.str(), std::ios_base::in);
		
		unique_ptr<ns::GrammarParsingResult> result = prs::parse_grammar(in, file_name);
		MPtr<const ebnf::Grammar> grammar = result->get_grammar();

		if (command_line->is_verbose()) {
			std::cout << "*** EBNF GRAMMAR ***\n";
			std::cout << '\n';
			grammar->print(std::cout);
		}
		
		return result;
	}

}

int ns::main(int argc, const char* const argv[]) {
	
	//* Parse Command Line *
	
	unique_ptr<const ns::CommandLine> command_line = ns::CommandLine::parse_command_line(argv + 1);
	if (!command_line.get()) {
		//The error message must have already been output.
		return 1;
	}

	//* Parse Source Grammar File *
	
	unique_ptr<ns::GrammarParsingResult> parsing_result = parse_grammar(command_line.get());
	
	//* Build EBNF Grammar from Abstract Syntax Tree *

	unique_ptr<ns::GrammarBuildingResult> building_result =
		EBNF_Builder::build(command_line->is_verbose(), std::move(parsing_result));

	//* Generate BNF Grammar from EBNF Grammar *

	unique_ptr<ns::ConversionResult> conversion_result =
		ns::convert_EBNF_to_BNF(command_line->is_verbose(), std::move(building_result));

	//* Generate LR Tables *
	
	unique_ptr<const ConcreteLRResult> lr_result =
		ns::generate_LR_tables(*command_line, std::move(conversion_result));

	//* Generate Result Files *

	ns::generate_result_files(*command_line, std::move(lr_result));

	//* End *

	if (command_line->is_verbose()) std::cout << "OK\n";

	return 0;
}
