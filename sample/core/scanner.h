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

//Scanner.

#ifndef SYNSAMPLE_CORE_SCANNER_H_INCLUDED
#define SYNSAMPLE_CORE_SCANNER_H_INCLUDED

#include <memory>

#include "common.h"
#include "syn.h"
#include "syngen.h"
#include "name__dec.h"
#include "stringex__dec.h"

namespace syn_script {

	class InternalScanner;

	//
	//Scanner
	//

	class Scanner {
		NONCOPYABLE(Scanner);

		std::unique_ptr<InternalScanner> m_internal_scanner;

	public:
		Scanner(NameRegistry& name_registry, const StringLoc& file_name, const StringLoc& text);
		~Scanner(); //For std::unique_ptr<InternalScanner>.

		syngen::Token scan(syngen::TokenValue& token_value);
		gc::Local<TextPos> get_text_pos() const;
	};

}

#endif//SYNSAMPLE_CORE_SCANNER_H_INCLUDED
