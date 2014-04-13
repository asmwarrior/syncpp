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

//Definitions of script integer and floating-point types. And functions for handling them.

#ifndef SYNSAMPLE_CORE_BASETYPE_H_INCLUDED
#define SYNSAMPLE_CORE_BASETYPE_H_INCLUDED

#include <cstddef>

namespace syn_script {

	//Must be an unsigned integer type. (It must be unsigned in order to handle arithmetical overflow
	//correctly.)
	typedef unsigned long long ScriptIntegerType;

	typedef double ScriptFloatType;

	int scriptint_sign(ScriptIntegerType v);
	ScriptIntegerType scriptint_neg(ScriptIntegerType v);

	//The sign of the result is the same as the sign of (a - b).
	int cmp_scriptint_int(ScriptIntegerType a, int b);

	//If the value is out of range, the result is undefined.
	int scriptint_to_int(ScriptIntegerType v);

	ScriptIntegerType int_to_scriptint(int v);

	ScriptIntegerType int_to_scriptint_ex(int v);

	//If the value is out of range, the result is undefined.
	int scriptfloat_to_int(ScriptFloatType v);

	std::size_t scriptint_to_hashcode(ScriptIntegerType v);

	//Throws an exception if the value is out of range.
	int scriptint_to_int_ex(ScriptIntegerType v);

	//If the value is out of range, the result is undefined.
	std::size_t scriptint_to_size(ScriptIntegerType v);

	//Throws an exception if the value is out of range.
	std::size_t scriptint_to_size_ex(ScriptIntegerType v);

	char scriptint_to_char_ex(ScriptIntegerType v);

	ScriptIntegerType char_to_scriptint_ex(char c);

	ScriptIntegerType size_to_scriptint_ex(std::size_t s);

	ScriptIntegerType ulonglong_to_scriptint_opt(unsigned long long v);

}

#endif//SYNSAMPLE_CORE_BASETYPE_H_INCLUDED
