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

//Unit tests for the reference-counting string.

#include <cstddef>

#include "core/util_string.h"

#include "unittest.h"

namespace ns = synbin;
namespace util = ns::util;

using util::String;

TEST(null_string) {
	String str;
	assertTrue(str.empty());
	assertTrue(str.str().empty());
}

TEST(null_string_instances) {
	String str1;
	String str2;
	assertPtrEquals(&str1.str(), &str2.str());
}

TEST(empty_string) {
	String str("");
	assertTrue(str.empty());
}

TEST(empty_string_instances) {
	String str1;
	String str2("");

	assertPtrNotEquals(&str1.str(), &str2.str());
}

TEST(non_empty_string) {
	String str("Hello");
	assertFalse(str.empty());
}

TEST(null_string_copy_construct) {
	String str0;
	String str = str0;
	assertTrue(str.empty());
	assertTrue(str.str().empty());
}

TEST(empty_string_copy_construct) {
	String str0("");
	String str = str0;
	assertTrue(str.empty());
}

TEST(non_empty_string_copy_construct) {
	std::string* std_string = new std::string("Hello");
	String str0("Hello");
	String str = str0;
	assertFalse(str.empty());
}

TEST(null_string_assign_null_string) {
	String str0;
	String str;
	str = str0;
	assertTrue(str.empty());
	assertTrue(str.str().empty());
}

TEST(null_string_assign_empty_string) {
	std::string* std_string = new std::string("");
	String str0("");
	String str;
	str = str0;
	assertTrue(str.empty());
}

TEST(null_string_assign_non_empty_string) {
	std::string* std_string = new std::string("Hello");
	String str0("Hello");
	String str;
	str = str0;
}

TEST(empty_string_assign_null_string) {
	String str0;

	String str("");
	str = str0;
	assertTrue(str.empty());
	assertPtrEquals(&str0.str(), &str.str());
}

TEST(empty_string_assign_empty_string) {
	String str0("");

	String str("");
	str = str0;
	assertTrue(str.empty());
	assertPtrEquals(&str0.str(), &str.str());
}

TEST(empty_string_assign_non_empty_string) {
	String str0("Hello");

	String str("");
	str = str0;
	assertFalse(str.empty());
	assertPtrEquals(&str0.str(), &str.str());
}

TEST(non_empty_string_assign_null_string) {
	String str0;

	String str("Hello");
	str = str0;
	assertTrue(str.empty());
	assertPtrEquals(&str0.str(), &str.str());
}

TEST(non_empty_string_assign_empty_string) {
	String str0("");

	String str("Hello");
	str = str0;
	assertTrue(str.empty());
	assertPtrEquals(&str0.str(), &str.str());
}

TEST(non_empty_string_assign_non_empty_string) {
	String str0("Hello2");

	String str("Hello");
	str = str0;
	assertFalse(str.empty());
	assertPtrEquals(&str0.str(), &str.str());
}
