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

//Declarations of AST classes.

#ifndef SYNSAMPLE_CORE_AST_DEC_H_INCLUDED
#define SYNSAMPLE_CORE_AST_DEC_H_INCLUDED

namespace syn_script {
	namespace ast {

		class Node;

		//Script
		class Script;
		class Block;

		//Statement
		class Statement;
		class EmptyStatement;
		class DeclarationStatement;
		class ExpressionStatement;
		class IfStatement;
		class WhileStatement;
		class ForStatement;
		class RegularForStatement;
		class ForEachStatement;
		class ForInit;
		class VariableForInit;
		class ForVariableDeclaration;
		class ExpressionForInit;
		class BlockStatement;
		class ContinueStatement;
		class BreakStatement;
		class ReturnStatement;
		class TryStatement;
		class ThrowStatement;

		//Declaration
		class Declaration;
		class VariableDeclaration;
		class ConstantDeclaration;
		class FunctionDeclaration;
		class FunctionFormalParameters;
		class FunctionBody;
		class ClassDeclaration;
		class ClassBody;
		class ClassMemberDeclaration;

		//Expression
		class Expression;
		class AssignmentExpression;
		class ConditionalExpression;
		class OrExpression;
		class AndExpression;
		class EqExpression;
		class RelExpression;
		class AddExpression;
		class MulExpression;
		class PrefixExpression;
		class PostfixExpression;
		class TerminalExpression;
		class MemberExpression;
		class InvocationExpression;
		class NewObjectExpression;
		class TypeExpression;
		class TypeNameExpression;
		class NewArrayExpression;
		class ArrayExpression;
		class SubscriptExpression;
		class NameExpression;
		class FunctionExpression;
		class ClassExpression;
		class LiteralExpression;
		class IntegerLiteralExpression;
		class FloatingPointLiteralExpression;
		class StringLiteralExpression;
		class NullExpression;
		class BooleanLiteralExpression;

	}
}

#endif//SYNSAMPLE_CORE_AST_DEC_H_INCLUDED
