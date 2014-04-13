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

//Declaration of GC String class and related types.

#ifndef SYNSAMPLE_CORE_STRINGEX_DEC_H_INCLUDED
#define SYNSAMPLE_CORE_STRINGEX_DEC_H_INCLUDED

#include "gc.h"

namespace syn_script {

	class String;

	typedef gc::Local<const String> StringLoc;
	typedef gc::Ref<const String> StringRef;
	typedef gc::Array<const String> StringArray;

}

#endif//SYNSAMPLE_CORE_STRINGEX_DEC_H_INCLUDED
