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

//EBNF grammar visitor classes declarations.

#ifndef SYN_CORE_EBNF_VISITOR_DEC_H_INCLUDED
#define SYN_CORE_EBNF_VISITOR_DEC_H_INCLUDED

namespace synbin {

	template<class T>
	class SymbolDeclarationVisitor;

	template<class T>
	class DeclarationVisitor;

	template<class T>
	class SyntaxExpressionVisitor;

	template<class T>
	class ConstExpressionVisitor;

	template<class T>
	class AndExpressionMeaningVisitor;

}

#endif//SYN_CORE_EBNF_VISITOR_DEC_H_INCLUDED
