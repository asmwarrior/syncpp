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

//Unit tests framework definitions.

#ifndef SYN_TEST_UNITTEST_H_INCLUDED
#define SYN_TEST_UNITTEST_H_INCLUDED

#include <cstddef>
#include <ostream>
#include <string>

#include "../core/commons.h"

#define TEST(NAME) \
	void test_##NAME();\
	unittest::UnitTest test_##NAME##_case(&test_##NAME, __FILE__, __LINE__, #NAME);\
	void test_##NAME()

#define assertEquals(expected, actual) unittest::assertEquals0(__FILE__, __LINE__, (expected), (actual))
#define assertPtrEquals(expected, actual) unittest::assertPtrEquals0(__FILE__, __LINE__, (expected), (actual))
#define assertPtrNotEquals(expected, actual) unittest::assertPtrNotEquals0(__FILE__, __LINE__, (expected), (actual))
#define assertNotEquals(unexpected, actual) unittest::assertNotEquals0(__FILE__, __LINE__, (unexpected), (actual))
#define assertNull(actual) unittest::assertNull0(__FILE__, __LINE__, (actual))
#define assertNotNull(actual) unittest::assertNotNull0(__FILE__, __LINE__, (actual))
#define assertFalse(actual) unittest::assertFalse0(__FILE__, __LINE__, (actual))
#define assertTrue(actual) unittest::assertTrue0(__FILE__, __LINE__, (actual))
#define fail() unittest::fail0(__FILE__, __LINE__)
#define failMsg(msg) unittest::failMsg0(__FILE__, __LINE__, msg)

namespace unittest {

	class UnitTest;

	std::ostream& operator<<(std::ostream& out, const UnitTest& test);

	//
	//UnitTest
	//

	//Describes a unit test.
	class UnitTest {
		NONCOPYABLE(UnitTest);

		friend std::ostream& operator<<(std::ostream& out, const UnitTest& test);

		static const char* const s_log_file_name;

		static UnitTest* m_first;
		static UnitTest* m_last;

		void (*const m_testfn)();
		const char* const m_file;
		const int m_line;
		const char* const m_name;
		UnitTest* m_next;

		bool m_accepted;
		bool m_failed;
		std::string m_fail_message;

		bool run();
		void succeed();
		void failed(const std::string& message);
		void log(const std::string& message);

		bool accept_test(const char* test_file_filter, const char* test_name_filter);

	public:
		UnitTest(void (*testfn)(), const char* file, int line, const char* name);
		static void start(const char* test_file_filter = 0, const char* test_name_filter = 0);
	};

	//
	//TestException
	//

	class TestException {
		const char* const m_file;
		const int m_line;
		const std::string m_message;
	public:
		explicit TestException(const char* file = 0, int line = 0) : m_file(file), m_line(line){}
		explicit TestException(const std::string& message, const char* file = 0, int line = 0)
			: m_message(message), m_file(file), m_line(line){}
		
		virtual ~TestException(){}

		const char* file() const { return m_file; }
		int line() const { return m_line; }
		const std::string& get_message() const { return m_message; }
	};

	//
	//StrMissmatchTestException
	//

	class StrMissmatchTestException : public TestException {
		const std::string m_expected;
		const std::string m_actual;
	public:
		StrMissmatchTestException(const char* file, int line, const std::string& expected, const std::string& actual)
			: TestException("Expected: '" + expected + "', was '" + actual + "'", file, line),
			m_expected(expected),
			m_actual(actual)
		{}

		const std::string& get_expected() const { return m_expected; }
		const std::string& get_actual() const { return m_actual; }
	};

	//
	//(Functions)
	//

	void assertEquals0(const char* file, int line, const std::string& expected, const char* actual);
	void assertEquals0(const char* file, int line, const char* expected, const std::string& actual);
	void assertEquals0(const char* file, int line, const std::string& expected, const std::string& actual);
	void assertEquals0(const char* file, int line, const char* expected, const char* actual);

	void assertEquals0(const char* file, int line, int expected, int actual);
	void assertEquals0(const char* file, int line, const void* expected, const void* actual);

	void assertPtrEquals0(const char* file, int line, const void* expected, const void* actual);
	void assertPtrNotEquals0(const char* file, int line, const void* expected, const void* actual);

	void assertNotEquals0(const char* file, int line, int unexpected, int actual);

	void assertNull0(const char* file, int line, const void* actual);
	void assertNotNull0(const char* file, int line, const void* actual);

	void assertFalse0(const char* file, int line, bool actual);
	void assertTrue0(const char* file, int line, bool actual);

	void fail0(const char* file, int line);
	void failMsg0(const char* file, int line, const char* msg);
	void failMsg0(const char* file, int line, const std::string& msg);
}

#endif//SYN_TEST_UNITTEST_H_INCLUDED
