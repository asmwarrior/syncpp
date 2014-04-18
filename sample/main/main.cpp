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

//Script Language interpreter application entry point.

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int sample_main(
	const std::string& file_name,
	const std::vector<std::string>& arguments,
	std::size_t mem_limit_mb);

namespace {
	int command_line_error() {
		std::cerr << "Usage: script [-m MEMORY_LIMIT_MB] FILE (ARGUMENT)*\n";
		return 1;
	}
}

int main(int argc, const char** argv) {
	std::size_t mem_limit = 0;

	int argpos = 1;

	if (argpos < argc && 0 == strcmp("-m", argv[argpos])) {
		++argpos;
		if (argpos == argc) return command_line_error();

		std::string limit_str = argv[argpos++];

		int v;
		try {
			v = std::stoi(limit_str);
		} catch (std::logic_error&) {
			std::cerr << "Invalid memory limit\n";
			return 1;
		}
		
		if (v < 1 || v > 2048) {
			std::cerr << "Memory limit is out of range\n";
			return 1;
		}

		mem_limit = v;
	}

	if (argpos == argc) return command_line_error();

	const std::string file_name = argv[argpos++];
	std::vector<std::string> arguments;
	while (argpos < argc) arguments.push_back(argv[argpos++]);

	sample_main(file_name, arguments, mem_limit);
}
