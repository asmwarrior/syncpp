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

//Script Language interpreter entry point.

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "gc.h"
#include "name.h"
#include "script.h"
#include "stringex__dec.h"
#include "value_core.h"

#include "syngen.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;

namespace {
	ss::StringLoc load_file(const std::string& file_name) {
		std::ifstream in;
		std::ostringstream out;

		try {
			in.exceptions(std::ios_base::badbit | std::ios_base::failbit);
			in.open(file_name, std::ios_base::in);
		} catch (const std::ios_base::failure&) {
			std::string msg = "File not found: ";
			msg += file_name;
			throw ss::RuntimeError(msg);
		}

		in.exceptions(std::ios_base::badbit);
		
		char c;
		while (in.get(c)) out << c;

		return gc::create<ss::String>(out.str());
	}

	gc::Local<ss::StringArray> create_arguments_array(const std::vector<const char*>& arguments_c) {
		std::size_t n = arguments_c.size();
		gc::Local<ss::StringArray> array = ss::StringArray::create(n);
		for (std::size_t i = 0; i < n; ++i) (*array)[i] = gc::create<ss::String>(arguments_c[i]);
		return array;
	}
}//namespace

void link__api();

int sample_main(const char* file_name_c, const std::vector<const char*>& arguments_c) {
	link__api();

	try {
		gc::startup_guard gc_startup(8 * 1024 * 1024);
		gc::manage_thread_guard gc_thread;
		gc::enable_guard gc_enable;

		try {
			ss::StringLoc file_name = gc::create<ss::String>(file_name_c);
			ss::StringLoc code = load_file(file_name_c);
			gc::Local<ss::StringArray> arguments = create_arguments_array(arguments_c);
			gc::Local<gc::Array<rt::ScriptSource>> sources = rt::get_single_script_source(file_name, code);
			bool ok = rt::execute_top_script(sources, arguments);
			return ok ? 0 : 1;
		} catch (const ss::BasicError& e) {
			std::cerr << e << std::endl;
			return 1;
		} catch (const gc::out_of_memory&) {
			std::cerr << "Out of memory!\n";
			return 1;
		}
	} catch (const char* e) {
		std::cerr << "String exception: " << e << '\n';
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception!\n";
		return 1;
	}
}
