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

//Unit tests for command line parser.

#include <memory>

#include "core/cmdline.h"

#include "unittest.h"

namespace ns = synbin;

namespace {//anonymous

TEST(empty) {
	const char* args[] = { nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(help) {
	const char* args[] = { "-?", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(invalid_option) {
	const char* args[] = { "-invalidoption", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(no_file_name) {
	const char* args[] = { "-s", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(filename) {
	const char* args[] = { "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNotNull(cmdline.get());
	assertEquals("filename.txt", cmdline->get_in_file());
}

TEST(too_few_option_arguments) {
	const char* args[] = { "-n", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(too_many_option_arguments) {
	const char* args[] = { "-s", "aaa", "bbb", "ccc", "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(duplicated_option_i) {
	const char* args[] = { "-i", "file1.h", "-i", "<file2.h>", "-i", "file3.h", "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNotNull(cmdline.get());
	assertEquals(3, cmdline->get_include_files().size());
	assertEquals("file1.h", cmdline->get_include_files()[0].get_name());
	assertFalse(cmdline->get_include_files()[0].is_system());
	assertEquals("file2.h", cmdline->get_include_files()[1].get_name());
	assertTrue(cmdline->get_include_files()[1].is_system());
	assertEquals("file3.h", cmdline->get_include_files()[2].get_name());
	assertFalse(cmdline->get_include_files()[2].is_system());
}

TEST(duplicated_option_mm) {
	const char* args[] = { "-mm", "pattern1", "-mm", "pattern2", "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(duplicated_option_n) {
	const char* args[] = { "-n", "ns1", "-n", "ns2", "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(duplicated_option_nc) {
	const char* args[] = { "-nc", "ns1", "-nc", "ns2", "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(duplicated_option_ng) {
	const char* args[] = { "-ng", "ns1", "-ng", "ns2", "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(duplicated_option_nt) {
	const char* args[] = { "-nt", "ns1", "-nt", "ns2", "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(duplicated_option_s) {
	const char* args[] = { "-s", "-s", "filename.txt", nullptr };
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNull(cmdline.get());
}

TEST(all_options) {
	const char* args[] = {
		"-i", "file1.h", "-i", "<file2.h>",
		"-mm", "mm_pattern",
		"-n", "ns_n", "-nc", "ns_nc", "-ng", "ns_ng", "-nt", "ns_nt",
		"-s",
		"infile.txt",
		"outfile",
		nullptr
	};
	std::unique_ptr<const ns::CommandLine> cmdline = ns::CommandLine::parse_command_line(args);
	assertNotNull(cmdline.get());

	assertEquals(2, cmdline->get_include_files().size());
	assertEquals("file1.h", cmdline->get_include_files()[0].get_name());
	assertFalse(cmdline->get_include_files()[0].is_system());
	assertEquals("file2.h", cmdline->get_include_files()[1].get_name());
	assertTrue(cmdline->get_include_files()[1].is_system());
	assertEquals("mm_pattern", cmdline->get_attr_name_pattern());
	assertEquals("ns_n", cmdline->get_namespace());
	assertEquals("ns_nc", cmdline->get_namespace_classes());
	assertEquals("ns_ng", cmdline->get_namespace_code());
	assertEquals("ns_nt", cmdline->get_namespace_types());
	assertTrue(cmdline->is_use_attr_setters());
	assertEquals("infile.txt", cmdline->get_in_file());
	assertEquals("outfile", cmdline->get_out_file());
}

}
