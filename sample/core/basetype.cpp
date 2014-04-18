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

//Functions for handling script integer and floating-point types.

#include <limits>
#include <stdexcept>
#include <string>

#include "basetype.h"
#include "common.h"

namespace ss = syn_script;

namespace {
	typedef std::numeric_limits<ss::ScriptIntegerType> ScriptIntLimits;
	typedef std::numeric_limits<int> IntLimits;
	typedef std::numeric_limits<std::size_t> SizeLimits;
	typedef std::numeric_limits<unsigned char> UCharLimits;

	const std::size_t SCRIPTINT_BITS = ScriptIntLimits::digits;
	const std::size_t SIZE_BITS = SizeLimits::digits;

	const ss::ScriptIntegerType SIGN_MASK = static_cast<ss::ScriptIntegerType>(1) << (SCRIPTINT_BITS - 1);
}

int ss::scriptint_sign(ScriptIntegerType v) {
	if (!v) return 0;
	return v < SIGN_MASK ? 1 : -1;
}

ss::ScriptIntegerType ss::scriptint_neg(ScriptIntegerType v) {
	return 0 - v;
}

int ss::cmp_scriptint_int(ScriptIntegerType a, int b0) {
	ScriptIntegerType b = int_to_scriptint(b0);
	ScriptIntegerType d = a - b;
	return scriptint_sign(d);
}

int ss::scriptint_to_int(ScriptIntegerType v) {
	int sign = scriptint_sign(v);
	return sign >= 0 ? static_cast<int>(v) : -static_cast<int>(scriptint_neg(v));
}

ss::ScriptIntegerType ss::int_to_scriptint(int v) {
	return static_cast<ScriptIntegerType>(v);
}

ss::ScriptIntegerType ss::int_to_scriptint_ex(int v) {
	return int_to_scriptint(v);
}

int ss::scriptfloat_to_int(ScriptFloatType v) {
	return static_cast<int>(v);
}

std::size_t ss::scriptint_to_hashcode(ScriptIntegerType v) {
	static_assert(sizeof(ScriptIntegerType) == 2 * sizeof(std::size_t), "Wrong sizeof");
	std::size_t hash = static_cast<std::size_t>(v);
	hash ^= static_cast<std::size_t>(v >> SIZE_BITS);
	return hash;
}

int ss::scriptint_to_int_ex(ScriptIntegerType v) {
	if (v > IntLimits::max() && v < IntLimits::min()) throw RuntimeError("Value out of range");
	return scriptint_to_int(v);
}

std::size_t ss::scriptint_to_size(ScriptIntegerType v) {
	return static_cast<std::size_t>(v);
}

std::size_t ss::scriptint_to_size_ex(ScriptIntegerType v) {
	if (v > SizeLimits::max()) throw RuntimeError("Value out of range");
	return static_cast<std::size_t>(v);
}

char ss::scriptint_to_char_ex(ScriptIntegerType v) {
	if (v > UCharLimits::max()) throw RuntimeError("Value out of range");
	unsigned char uc = static_cast<unsigned char>(v);
	return reinterpret_cast<char&>(uc);
}

ss::ScriptIntegerType ss::char_to_scriptint_ex(char c) {
	unsigned char uc = reinterpret_cast<unsigned char&>(c);
	return static_cast<ScriptIntegerType>(uc);
}

ss::ScriptIntegerType ss::size_to_scriptint_ex(std::size_t s) {
	return s;
}

ss::ScriptIntegerType ss::ulonglong_to_scriptint_opt(unsigned long long v) {
	if (v < SIGN_MASK) return v;
	return int_to_scriptint(-1);
}

bool ss::str_to_int(const std::string& str, ScriptIntegerType& result, int base) {
	try {
		result = std::stoll(str, nullptr, base);
		return true;
	} catch (const std::logic_error&) {
		return false;
	}
}

bool ss::str_to_float(const std::string& str, ScriptFloatType& result) {
	try {
		result = std::stod(str);
		return true;
	} catch (const std::logic_error&) {
		return false;
	}
}
