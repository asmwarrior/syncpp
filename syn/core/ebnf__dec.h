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

//EBNF grammar classes declarations.

#ifndef SYN_CORE_EBNF_DEC_H_INCLUDED
#define SYN_CORE_EBNF_DEC_H_INCLUDED

namespace synbin {
	namespace ebnf {

		//Base Classes

		class Object;
		class Grammar;
		class Declaration;
		class RegularDeclaration;
		class TypeDeclaration;
		class SymbolDeclaration;
		class TerminalDeclaration;
		class NonterminalDeclaration;
		class CustomTerminalTypeDeclaration;
		class RawType;

		//Syntax Expressions

		class SyntaxExpression;
		class EmptySyntaxExpression;
		class CompoundSyntaxExpression;
		class SyntaxOrExpression;
		class SyntaxAndExpression;
		class SyntaxElement;
		class NameSyntaxElement;
		class ThisSyntaxElement;
		class NameSyntaxExpression;
		class StringSyntaxExpression;
		class CastSyntaxExpression;
		class ZeroOneSyntaxExpression;
		class LoopSyntaxExpression;
		class ZeroManySyntaxExpression;
		class OneManySyntaxExpression;
		class LoopBody;
		class ConstSyntaxExpression;

		//Constant Expressions

		class ConstExpression;
		class IntegerConstExpression;
		class StringConstExpression;
		class BooleanConstExpression;
		class NativeConstExpression;
		class NativeName;
		class NativeVariableName;
		class NativeFunctionName;
		class NativeReference;
		class NativePointerReference;
		class NativeReferenceReference;

	}
}

#endif//SYN_CORE_EBNF_DEC_H_INCLUDED
