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

//Reference-counting string class implementation.

#include <cassert>
#include <memory>
#include <ostream>
#include <string>

#include "util_string.h"

namespace ns = synbin;
namespace util = ns::util;

using std::unique_ptr;

namespace {

	const std::string g_empty_std_string;

}

util::String::String(const String& other_string)
: m_shared_block(other_string.m_shared_block)
{}

util::String& util::String::operator=(const String& other_string) {
	m_shared_block = other_string.m_shared_block;
	return *this;
}

util::String::String() {
	m_shared_block = nullptr;
}

util::String::String(const std::string& std_string) {
	m_shared_block = new SharedBlock(std_string);
}

util::String::String(const char* c_string) {
	m_shared_block = new SharedBlock(c_string);
}

const std::string& util::String::str() const {
	return !!m_shared_block ? m_shared_block->m_str : g_empty_std_string;
}

bool util::String::empty() const {
	return !m_shared_block || m_shared_block->m_str.empty();
}

std::ostream& util::operator<<(std::ostream& out, const util::String& string) {
	return out << string.str();
}

bool util::operator==(const String& str1, const String& str2) {
	return str1.str() == str2.str();
}

bool util::operator!=(const String& str1, const String& str2) {
	return !(str1 == str2);
}

bool util::operator<(const String& str1, const String& str2) {
	return str1.str() < str2.str();
}

bool util::operator>(const String& str1, const String& str2) {
	return str1.str() > str2.str();
}

bool util::operator<=(const String& str1, const String& str2) {
	return !(str1.str() > str2.str());
}

bool util::operator>=(const String& str1, const String& str2) {
	return !(str1.str() < str2.str());
}
