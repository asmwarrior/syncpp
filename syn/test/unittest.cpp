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

//Unit tests framework implementation.

#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "core/commons.h"
#include "unittest.h"

namespace ns = synbin;
const char* const unittest::UnitTest::s_log_file_name = "test.log";

unittest::UnitTest* unittest::UnitTest::m_first = 0;
unittest::UnitTest* unittest::UnitTest::m_last = 0;

unittest::UnitTest::UnitTest(void (*testfn)(), const char* file, int line, const char* name)
	: m_testfn(testfn),
	m_file(file),
	m_line(line),
	m_name(name),
	m_failed(false)
{
	if (m_last) {
		m_last->m_next = this;
	} else {
		m_first = this;
	}
	m_last = this;
	m_next = 0;
}

bool unittest::UnitTest::run() {
	m_failed = false;

	try {
		m_testfn();
	} catch (StrMissmatchTestException& e) {
		std::ostringstream sout;
		sout << "unittest::StrMissmatchTestException";
		if (e.file()) sout << "(" << e.file() << ":" << e.line() << ")";
		sout << ":\n";
		sout << "Expected: '" << e.get_expected() << "'\n";
		sout << "Actual:   '" << e.get_actual() << "'";
		failed(sout.str());
		return false;
	} catch (TestException& e) {
		std::ostringstream sout;
		sout << "unittest::TestException";
		if (e.file()) sout << "(" << e.file() << ":" << e.line() << ")";
		sout << ": " << e.get_message();
		failed(sout.str());
		return false;
	} catch (std::exception& e) {
		failed(std::string("std::exception: ") + e.what());
		return false;
	} catch (ns::Exception& e) {
		failed("syn::Exception: " + e.to_string());
		return false;
	} catch (...) {
		failed("Unhandled exception");
		return false;
	}
	succeed();
	return true;
}

void unittest::UnitTest::succeed() {
	std::ostringstream str_out;
	str_out << "OK " << *this;
	log(str_out.str());
}

void unittest::UnitTest::failed(const std::string& message) {
	std::ostringstream str_out;
	str_out << "FAILED " << *this << " : " + message;
	log(str_out.str());
	m_failed = true;
	m_fail_message = message;
}

void unittest::UnitTest::log(const std::string& message) {
	std::cout << message << "\n";

	std::ofstream out(s_log_file_name, std::ios_base::app);
	out << message << "\n";
}

bool unittest::UnitTest::accept_test(const char* test_file_filter, const char* test_name_filter) {
	if (test_file_filter && !std::strstr(m_file, test_file_filter)) return false;
	if (test_name_filter && !std::strstr(m_name, test_name_filter)) return false;
	return true;
}

void unittest::UnitTest::start(const char* test_file_filter, const char* test_name_filter) {
	std::ofstream out(s_log_file_name, std::ios_base::trunc);
	out.close();

	std::size_t total_count = 0;
	for (UnitTest* test = m_first; test; test = test->m_next) {
		bool accepted = test->accept_test(test_file_filter, test_name_filter);
		test->m_accepted = accepted;
		if (accepted) ++total_count;
	}

	std::size_t index = 0;
	std::size_t ok_count = 0;
	for (UnitTest* test = m_first; test; test = test->m_next) {
		if (test->m_accepted) {
			std::cout << "\ntest " << (index + 1) << "/" << total_count << ": " << *test << "\n";
			bool ok = test->run();
			if (ok) ++ok_count;
			++index;
		}
	}

	std::size_t fail_count = total_count - ok_count;
	
	if (fail_count) {
		std::cout << "-----\n";
		for (UnitTest* test = m_first; test; test = test->m_next) {
			if (test->m_failed) std::cout << "FAILED " << *test << " : " << test->m_fail_message << "\n";
		}
	}

	std::cout << "-----\n";
	std::cout << "Tests run: " << total_count << ", success: " << ok_count << ", failed: " << fail_count << "\n";
	std::cout << (fail_count ? "*** FAILED ***" : "*** OK ***") << "\n";
}

std::ostream& unittest::operator<<(std::ostream& out, const unittest::UnitTest& test) {
	out << test.m_name << " (" << test.m_file << ":" << test.m_line << ")";
	return out;
}

void unittest::assertEquals0(const char* file, int line, const std::string& expected, const char* actual) {
	if (!actual) {
		throw TestException("Expected '" + expected + "', was 0", file, line);
	} else if (expected != actual) {
		throw StrMissmatchTestException(file, line, expected, actual);
	}
}

void unittest::assertEquals0(const char* file, int line, const char* expected, const std::string& actual) {
	if (!expected) {
		throw TestException("Expected 0, was '" + actual + "'", file, line);
	} else if (expected != actual) {
		throw StrMissmatchTestException(file, line, expected, actual);
	}
}

void unittest::assertEquals0(const char* file, int line, const std::string& expected, const std::string& actual) {
	if (expected != actual) throw StrMissmatchTestException(file, line, expected, actual);
}

void unittest::assertEquals0(const char* file, int line, const char* expected, const char* actual) {
	if (!actual && expected) {
		throw TestException(std::string("Expected '") + expected + "', was 0", file, line);
	} else if (!expected && actual) {
		throw TestException(std::string("Expected 0, was '") + actual + "'", file, line);
	} else if (actual && expected && std::strcmp(expected, actual)) {
		throw StrMissmatchTestException(file, line, expected, std::string(actual));
	}
}

void unittest::assertEquals0(const char* file, int line, int expected, int actual) {
	if (expected != actual) {
		std::ostringstream out;
		out << "Expected '" << expected << "', was '" << actual << "'";
		throw TestException(out.str(), file, line);
	}
}

void unittest::assertEquals0(const char* file, int line, const void* expected, const void* actual) {
	assertPtrEquals0(file, line, expected, actual);
}

void unittest::assertPtrEquals0(const char* file, int line, const void* expected, const void* actual) {
	if (expected != actual) {
		std::ostringstream out;
		out << "Expected '" << expected << "', was '" << actual << "'";
		throw TestException(out.str(), file, line);
	}
}

void unittest::assertPtrNotEquals0(const char* file, int line, const void* expected, const void* actual) {
	if (expected == actual) throw TestException(file, line);
}

void unittest::assertNotEquals0(const char* file, int line, int unexpected, int actual) {
	if (actual == unexpected) throw TestException(file, line);
}

void unittest::assertNull0(const char* file, int line, const void* actual) {
	if (actual) throw TestException(file, line);
}

void unittest::assertNotNull0(const char* file, int line, const void* actual) {
	if (!actual) throw TestException(file, line);
}

void unittest::assertFalse0(const char* file, int line, bool actual) {
	if (actual) throw TestException(file, line);
}

void unittest::assertTrue0(const char* file, int line, bool actual) {
	if (!actual) throw TestException(file, line);
}

void unittest::fail0(const char* file, int line) {
	throw TestException(file, line);
}

void unittest::failMsg0(const char* file, int line, const char* msg) {
	throw TestException(msg, file, line);
}

void unittest::failMsg0(const char* file, int line, const std::string& msg) {
	throw TestException(msg, file, line);
}
