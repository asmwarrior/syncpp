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

//Command line options parsing and representation.

#ifndef SYN_CORE_CMDLINE_H_INCLUDED
#define SYN_CORE_CMDLINE_H_INCLUDED

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "noncopyable.h"

namespace synbin {

	//
	//IncludeFile
	//

	//Describes a file which is specified in the command line to be included into the result code.
	class IncludeFile {
		//The name of the file without quotes/brackets.
		const std::string m_name;

		//true for #include <name>, false for #include "name".
		const bool m_system;

	public:
		IncludeFile(const std::string& name, bool system) : m_name(name), m_system(system){}

		const std::string& get_name() const { return m_name; }
		bool is_system() const { return m_system; }
	};

	class CommandLineParser;

	//
	//CommandLine
	//

	//Command line.
	class CommandLine {
		NONCOPYABLE(CommandLine);

		//Input file. Cannot be an empty string.
		std::string m_in_file;

		//Output file. An empty string if not specified.
		std::string m_out_file;

		//Files to be #included.
		std::vector<IncludeFile> m_include_files;

		//Pattern for mapping attribute names to member names. An empty string if not specified.
		std::string m_attr_name_pattern; 

		//Default namespace. An empty string if not specified.
		std::string m_namespace;

		//Namespace for user-supplied classes. An empty string if not specified.
		std::string m_namespace_classes;

		//Namespace for user-supplied types. An empty string if not specified.
		std::string m_namespace_types;

		//Namespace for generated code. An empty string if not specified.
		std::string m_namespace_code;

		//Namespace for native constant expressinos. An empty string if not specified.
		std::string m_namespace_native;

		//true if syntax attributes have to be set by setters, false if they have to be set directly
		//by a member variable assignment.
		bool m_use_attr_setters;

		//Custom allocator. An empty string, if not specified.
		std::string m_allocator;

		//Verbose output.
		bool m_verbose;

		friend class OptionsParser;

		CommandLine() : m_use_attr_setters(false), m_verbose(false){}

	public:
		const std::string& get_in_file() const { return m_in_file; }
		const std::string& get_out_file() const { return m_out_file; }
		const std::vector<IncludeFile>& get_include_files() const { return m_include_files; }
		const std::string& get_attr_name_pattern() const { return m_attr_name_pattern; }
		const std::string& get_namespace() const { return m_namespace; }
		const std::string& get_namespace_classes() const { return m_namespace_classes; }
		const std::string& get_namespace_types() const { return m_namespace_types; }
		const std::string& get_namespace_code() const { return m_namespace_code; }
		const std::string& get_namespace_native() const { return m_namespace_native; }
		const std::string& get_allocator() const { return m_allocator; }
		bool is_use_attr_setters() const { return m_use_attr_setters; }
		bool is_verbose() const { return m_verbose; }

		//Parses the command line. Returns nullptr on error.
		static std::unique_ptr<const CommandLine> parse_command_line(const char* const* arguments);
	};

}

#endif//SYN_CORE_CMDLINE_H_INCLUDED
