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

//Command line processing tools.

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cmdline.h"

namespace ns = synbin;

namespace {
	typedef const char* Str;
}

namespace synbin {
	
	//This operator is used to find duplicated included files.
	bool operator==(const IncludeFile& a, const IncludeFile& b) {
		return a.is_system() == b.is_system() && a.get_name() == b.get_name();
	}

	//
	//OptionsParser
	//

	class OptionsParser {
		NONCOPYABLE(OptionsParser);

		CommandLine* const m_command_line;

		const Str* m_cur_ptr;
		const Str* const m_end_ptr;

		bool m_attr_name_pattern_set;
		bool m_namespace_set;
		bool m_namespace_classes_set;
		bool m_namespace_types_set;
		bool m_namespace_native_set;
		bool m_namespace_code_set;
		bool m_allocator_set;
		bool m_use_attr_setters_set;
		bool m_verbose_set;

		void check_already_set(bool OptionsParser::*set_var);

	public:
		OptionsParser(CommandLine* m_command_line, const Str* start_ptr, const Str* end_ptr);

		void parse_option_i();
		void parse_option_mm(std::string CommandLine::*target_var, bool OptionsParser::*set_var);
		void parse_option_n(std::string CommandLine::*target_var, bool OptionsParser::*set_var);
		void parse_option_s();
		void parse_option_v();
		void parse_option_a();
		void parse_option();
		const Str* parse_options();
	};

}

namespace {

	const char* const g_usage_short = "Usage: syn <options> <source file> [<destination file>]\n";

	const char* const g_usage_options =
		"\n"
		"Options:\n"
		"  -i <file>        Include the file into the generated C++ code\n"
		"  -mc <pattern>    Class name pattern for nonterminals in form 'prefix^suffix'\n"
		"  -mm <pattern>    Member name pattern for attributes in form 'prefix^suffix'\n"
		"  -n <namespace>   Namespace of user-supplied definitions\n"
		"  -nc <namespace>  Namespace of user-supplied classes (overrides -n)\n"
		"  -nt <namespace>  Namespace of user-supplied types (overrides -n)\n"
		"  -nn <namespace>  Namespace of native expressions (overrides -n)\n"
		"  -ng <namespace>  Namespace of the generated code\n"
		"  -s               Use member functions to set attributes (instead of member\n"
		"                   variables)\n"
		"  -a <typename>    Use the specified allocator in the generated code\n"
		"  -v               Verbose output\n";

	//
	//parse_error
	//

	//Class used to throw exceptions occurred while parsing the command line.
	class parse_error {
		const bool m_print_message;
	public:
		parse_error(bool print_message = true) : m_print_message(print_message){}

		bool is_print_message() {
			return m_print_message;
		}
	};

	//
	//(Functions)
	//

	void print_short_usage(std::ostream& out) {
		out << g_usage_short;
	}

	void print_usage() {
		print_short_usage(std::cout);
		std::cout << g_usage_options;
	}

	const Str* find_end(const Str* start) {
		while (*start) {
			++start;
		}
		return start;
	}

	bool is_option(const Str str) {
		return str && '-' == str[0];
	}
}

//
//OptionsParser
//

ns::OptionsParser::OptionsParser(CommandLine* command_line, const Str* start_ptr, const Str* end_ptr)
	: m_command_line(command_line), m_cur_ptr(start_ptr), m_end_ptr(end_ptr)
{
	m_attr_name_pattern_set = false;
	m_namespace_set = false;
	m_namespace_classes_set = false;
	m_namespace_types_set = false;
	m_namespace_native_set = false;
	m_namespace_code_set = false;
	m_use_attr_setters_set = false;
	m_verbose_set = false;
	m_allocator_set = false;
}

//Throws an exception if the specified flag is already set, otherwise sets the flag.
void ns::OptionsParser::check_already_set(bool OptionsParser::*set_var) {
	if (this->*set_var) {
		std::cerr << "Option '" << *m_cur_ptr << "' is specified more than once\n";
		throw parse_error(false);
	}
	this->*set_var = true;
}

//-i FILE
void ns::OptionsParser::parse_option_i() {
	const Str* start_ptr = m_cur_ptr;
	++m_cur_ptr;
		
	if (m_end_ptr == m_cur_ptr) {
		std::cerr << "Option " << *start_ptr << " requires file name\n";
		throw parse_error(false);
	}

	Str file_name = *m_cur_ptr;
	++m_cur_ptr;

	//File name is empty (e. g. syn -i "").
	if (!std::strlen(file_name)) throw parse_error();
		
	std::string name;
	bool system;

	if ('<' == file_name[0]) {
		//file name in angle brackets.
		std::size_t len = std::strlen(file_name);
		if (len < 2 || '>' != file_name[len - 1]) {
			std::cerr << "Invalid included file name: '" << file_name << "'\n";
			throw parse_error(false);
		}
		name = std::string(file_name + 1, file_name + len - 1);
		system = true;
	} else {
		//regular file name.
		name = std::string(file_name);
		system = false;
	}

	ns::IncludeFile include_file(name, system);

	//Check duplication and add.
	typedef std::vector<ns::IncludeFile>::const_iterator Iter;
	std::vector<ns::IncludeFile>& include_files = m_command_line->m_include_files;
	const Iter dup = std::find(include_files.begin(), include_files.end(), include_file);
	if (dup == include_files.end()) include_files.push_back(include_file);
}
		
//-mm PATTERN
void ns::OptionsParser::parse_option_mm(std::string ns::CommandLine::*target_var, bool ns::OptionsParser::*set_var) {
	check_already_set(set_var);

	const Str* start_ptr = m_cur_ptr++;
	if (m_end_ptr == m_cur_ptr) {
		std::cerr << "Option '" << *start_ptr << "' requires one argument\n";
		throw parse_error(false);
	}

	//TODO Check if the pattern is valid, e. g. does not contain invalid characters.
	m_command_line->*target_var = std::string(*m_cur_ptr);
	++m_cur_ptr;
}

//-n NAMESPACE
//-nc NAMESPACE
//-ng NAMESPACE
//-nn NAMESPACE
//-nt NAMESPACE
void ns::OptionsParser::parse_option_n(std::string ns::CommandLine::*target_var, bool ns::OptionsParser::*set_var) {
	check_already_set(set_var);

	const Str* start_ptr = m_cur_ptr++;
	if (m_end_ptr == m_cur_ptr) {
		std::cerr << "Option '" << *start_ptr << "' requires one argument\n";
		throw parse_error(false);
	}

	//TODO Check if the string is a valid C++ namespace name.
	m_command_line->*target_var = std::string(*m_cur_ptr);
	++m_cur_ptr;
}

//-s
void ns::OptionsParser::parse_option_s() {
	check_already_set(&OptionsParser::m_use_attr_setters_set);
	m_command_line->m_use_attr_setters = true;
	++m_cur_ptr;
}

//-v
void ns::OptionsParser::parse_option_v() {
	check_already_set(&OptionsParser::m_verbose_set);
	m_command_line->m_verbose = true;
	++m_cur_ptr;
}

//-a TYPENAME
void ns::OptionsParser::parse_option_a() {
	check_already_set(&OptionsParser::m_allocator_set);

	const Str* start_ptr = m_cur_ptr++;
	if (m_end_ptr == m_cur_ptr) {
		std::cerr << "Option '" << *start_ptr << "' requires one argument\n";
		throw parse_error(false);
	}

	m_command_line->m_allocator = std::string(*m_cur_ptr);
	++m_cur_ptr;
}

//Parse one option.
void ns::OptionsParser::parse_option()
{
	const Str option = *m_cur_ptr;
	if (!std::strcmp("-i", option)) {
		parse_option_i();
	} else if (!std::strcmp("-mm", option)) {
		parse_option_mm(&CommandLine::m_attr_name_pattern, &OptionsParser::m_attr_name_pattern_set);
	} else if (!std::strcmp("-n", option)) {
		parse_option_n(&CommandLine::m_namespace, &OptionsParser::m_namespace_set);
	} else if (!std::strcmp("-nc", option)) {
		parse_option_n(&CommandLine::m_namespace_classes, &OptionsParser::m_namespace_classes_set);
	} else if (!std::strcmp("-ng", option)) {
		parse_option_n(&CommandLine::m_namespace_code, &OptionsParser::m_namespace_code_set);
	} else if (!std::strcmp("-nn", option)) {
		parse_option_n(&CommandLine::m_namespace_native, &OptionsParser::m_namespace_native_set);
	} else if (!std::strcmp("-nt", option)) {
		parse_option_n(&CommandLine::m_namespace_types, &OptionsParser::m_namespace_types_set);
	} else if (!std::strcmp("-s", option)) {
		parse_option_s();
	} else if (!std::strcmp("-v", option)) {
		parse_option_v();
	} else if (!std::strcmp("-a", option)) {
		parse_option_a();
	} else {
		std::cerr << "Unknown option: '" << option << "'\n";
		throw parse_error(false);
	}	
}

//Parse sequence of options.
const Str* ns::OptionsParser::parse_options() {
	while (m_end_ptr != m_cur_ptr && is_option(*m_cur_ptr)) {
		const Str* start_ptr = m_cur_ptr;
		const Str* next_ptr = std::find_if(m_cur_ptr + 1, m_end_ptr, is_option);
		parse_option();
		if (next_ptr != m_end_ptr && next_ptr != m_cur_ptr) {
			std::cerr << "Too many arguments for option '" << *start_ptr << "'\n";
			throw parse_error();
		}
	}
	return m_cur_ptr;
}

//Parses the passed command line.
//Returns:
//- nullptr, if the command line is not correct. Error message is printed.
//- CommandLine object, if everything is OK.
std::unique_ptr<const ns::CommandLine> ns::CommandLine::parse_command_line(const char* const* arguments) {

	if (!arguments[0] || !std::strcmp("-?", arguments[0])) {
		print_usage();
		return std::unique_ptr<const CommandLine>(nullptr);
	}

	try {
		std::unique_ptr<CommandLine> command_line(new CommandLine);

		const Str* const start_ptr = arguments;
		const Str* const end_ptr = find_end(start_ptr);

		//Parse options.

		OptionsParser options_parser(&*command_line, start_ptr, end_ptr);
		const Str* cur_ptr = options_parser.parse_options();

		//Parse input file name.

		//End of command line, but file name is expected.
		if (cur_ptr == end_ptr) throw parse_error();
		
		//File name is empty.
		if (!std::strlen(*cur_ptr)) throw parse_error();

		command_line->m_in_file = std::string(*cur_ptr);
		++cur_ptr;

		//Parse output file name.
		if (cur_ptr != end_ptr) {
			if (!std::strlen(*cur_ptr)) throw parse_error();

			command_line->m_out_file = std::string(*cur_ptr);
			++cur_ptr;

			//End of command line is expected, but argument found.
			if (cur_ptr != end_ptr) throw parse_error();
		}
			
		//Return the command line.
		return std::unique_ptr<const CommandLine>(std::move(command_line));
	} catch (parse_error& e) {
		if (e.is_print_message()) {
			print_short_usage(std::cerr);
			std::cerr << "use -? for a list of possible options\n";
		}
		return std::unique_ptr<const CommandLine>(nullptr);
	}

}
