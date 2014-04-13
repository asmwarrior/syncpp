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

//AST: statement nodes.

#ifndef SYNSAMPLE_CORE_AST_STATEMENT_H_INCLUDED
#define SYNSAMPLE_CORE_AST_STATEMENT_H_INCLUDED

#include <memory>

#include "ast.h"
#include "ast__dec.h"
#include "ast_type.h"
#include "gc.h"
#include "scope.h"

namespace syn_script {
	namespace ast {

		//
		//Statement
		//

		class Statement : public Node {
			NONCOPYABLE(Statement);

		protected:
			Statement(){}

		public:
			virtual gc::Local<TextPos> get_pos() const = 0;
			virtual ast_ptr<Declaration> get_declaration() const = 0;
			virtual void bind(rt::BindContext* context, rt::BindScope* scope) = 0;

			rt::StatementResult execute(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope);

		protected:
			virtual rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) = 0;
		};

		//
		//DeclarationStatement
		//

		class DeclarationStatement : public Statement {
			NONCOPYABLE(DeclarationStatement);

			ast_ref<Declaration> m_syn_declaration;

		public:
			DeclarationStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_declaration(const ast_ptr<Declaration>&);

		public:
			gc::Local<TextPos> get_pos() const override final;
			ast_ptr<Declaration> get_declaration() const override final;
			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//ExecutionStatement
		//

		class ExecutionStatement : public Statement {
			NONCOPYABLE(ExecutionStatement);

		protected:
			ExecutionStatement(){}

		public:
			ast_ptr<Declaration> get_declaration() const override final;
		};

		//
		//EmptyStatement
		//

		class EmptyStatement : public ExecutionStatement {
			NONCOPYABLE(EmptyStatement);

			gc::Ref<TextPos> m_syn_pos;

		public:
			EmptyStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//ExpressionStatement
		//

		class ExpressionStatement : public ExecutionStatement {
			NONCOPYABLE(ExpressionStatement);

			ast_ref<Expression> m_syn_expression;

		public:
			ExpressionStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_expression(const ast_ptr<Expression>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//IfStatement
		//

		class IfStatement : public ExecutionStatement {
			NONCOPYABLE(IfStatement);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_expression;
			ast_ref<Statement> m_syn_true_statement;
			ast_ref<Statement> m_syn_false_statement;

		public:
			IfStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_expression(const ast_ptr<Expression>&);
			void syn_true_statement(const ast_ptr<Statement>&);
			void syn_false_statement(const ast_ptr<Statement>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//LoopStatement
		//

		class LoopStatement : public ExecutionStatement {
			NONCOPYABLE(LoopStatement);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_expression;
			ast_ref<Statement> m_syn_statement;

			gc::Ref<rt::ScopeDescriptor> m_scope_descriptor;

		protected:
			LoopStatement(){}
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_expression(const ast_ptr<Expression>&);
			void syn_statement(const ast_ptr<Statement>&);

		public:
			gc::Local<TextPos> get_pos() const override final;
			const ast_ref<Expression>& get_expression() const;
			const ast_ref<Statement>& get_statement() const;

			void bind(rt::BindContext* context, rt::BindScope* scope) override final;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override final;

		protected:
			virtual void bind_loop(rt::BindContext* context, rt::BindScope* scope);

			virtual rt::StatementResult exec_loop(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) = 0;
		};

		//
		//RegularLoopStatement
		//

		class RegularLoopStatement : public LoopStatement {
			NONCOPYABLE(RegularLoopStatement);

		protected:
			RegularLoopStatement(){}

			rt::StatementResult exec_loop(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override final;

			virtual void exec_loop_init(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception);

			virtual void exec_loop_update(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception);
		};

		//
		//WhileStatement
		//

		class WhileStatement : public RegularLoopStatement {
			NONCOPYABLE(WhileStatement);

		public:
			WhileStatement(){}
		};

		//
		//RegularForStatement
		//

		class RegularForStatement : public RegularLoopStatement {
			NONCOPYABLE(RegularForStatement);

			ast_ref<ForInit> m_syn_init;
			ast_ref<const ast_node_list<Expression>> m_syn_update;

		public:
			RegularForStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_init(const ast_ptr<ForInit>&);
			void syn_update(const ast_ptr<const ast_node_list<Expression>>&);

		protected:
			void bind_loop(rt::BindContext* context, rt::BindScope* scope) override;

			void exec_loop_init(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;

			void exec_loop_update(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//ForEachStatement
		//

		class ForEachValueIterator;

		class ForEachStatement : public LoopStatement {
			NONCOPYABLE(ForEachStatement);

			friend class ForEachValueIterator;

			bool m_syn_new_variable;
			gc::Ref<AstName> m_syn_variable;

			gc::Ref<rt::NameDescriptor> m_name_descriptor;

		public:
			ForEachStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_new_variable(bool);
			void syn_variable(const SynName&);

		protected:
			void bind_loop(rt::BindContext* context, rt::BindScope* scope) override;

			rt::StatementResult exec_loop(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//ForInit
		//

		class ForInit : public Node {
			NONCOPYABLE(ForInit);

		protected:
			ForInit(){}

		public:
			virtual void bind(rt::BindContext* context, rt::BindScope* scope) = 0;

			virtual void execute(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) = 0;
		};

		//
		//VariableForInit
		//

		class VariableForInit : public ForInit {
			NONCOPYABLE(VariableForInit);

			ast_ref<const ast_node_list<ForVariableDeclaration>> m_syn_variables;

		public:
			VariableForInit(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_variables(const ast_ptr<const ast_node_list<ForVariableDeclaration>>&);

		public:
			void bind(rt::BindContext* context, rt::BindScope* scope) override;

			void execute(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//ForVariableDeclaration
		//

		class ForVariableDeclaration : public Node {
			NONCOPYABLE(ForVariableDeclaration);

			gc::Ref<AstName> m_syn_name;
			ast_ref<Expression> m_syn_expression;

			gc::Ref<rt::NameDescriptor> m_name_descriptor;

		public:
			ForVariableDeclaration(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_name(const SynName&);
			void syn_expression(const ast_ptr<Expression>&);

		public:
			void bind(rt::BindContext* context, rt::BindScope* scope);

			void execute(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception);
		};

		//
		//ExpressionForInit
		//

		class ExpressionForInit : public ForInit {
			NONCOPYABLE(ExpressionForInit);

			ast_ref<const ast_node_list<Expression>> m_syn_expressions;

		public:
			ExpressionForInit(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_expressions(const ast_ptr<const ast_node_list<Expression>>&);

		public:
			void bind(rt::BindContext* context, rt::BindScope* scope) override;

			void execute(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//BlockStatement
		//

		class BlockStatement : public ExecutionStatement {
			NONCOPYABLE(BlockStatement);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Block> m_syn_block;

			gc::Ref<rt::ScopeDescriptor> m_scope_descriptor;

		public:
			BlockStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_block(const ast_ptr<Block>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//TryStatement
		//

		class TryStatement : public ExecutionStatement {
			NONCOPYABLE(TryStatement);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Statement> m_syn_try_statement;
			gc::Ref<AstName> m_syn_catch_variable;
			ast_ref<Statement> m_syn_catch_statement;
			ast_ref<Statement> m_syn_finally_statement;

			gc::Ref<rt::ScopeDescriptor> m_catch_scope_descriptor;
			gc::Ref<rt::NameDescriptor> m_catch_name_descriptor;

		public:
			TryStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_try_statement(const ast_ptr<Statement>&);
			void syn_catch_variable(const SynName&);
			void syn_catch_statement(const ast_ptr<Statement>&);
			void syn_finally_statement(const ast_ptr<Statement>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//ControlStatement
		//

		class ControlStatement : public ExecutionStatement {
			NONCOPYABLE(ControlStatement);

			gc::Ref<TextPos> m_syn_pos;

		protected:
			ControlStatement(){}
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);

		public:
			gc::Local<TextPos> get_pos() const override final;
		};

		//
		//ContinueStatement
		//

		class ContinueStatement : public ControlStatement {
			NONCOPYABLE(ContinueStatement);

		public:
			ContinueStatement(){}

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//BreakStatement
		//

		class BreakStatement : public ControlStatement {
			NONCOPYABLE(BreakStatement);

		public:
			BreakStatement(){}

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//ReturnStatement
		//

		class ReturnStatement : public ControlStatement {
			NONCOPYABLE(ReturnStatement);

			ast_ref<Expression> m_syn_return_value;

		public:
			ReturnStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_return_value(const ast_ptr<Expression>&);

		public:
			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;
		};

		//
		//ThrowStatement
		//

		class ThrowStatement : public ControlStatement {
			NONCOPYABLE(ThrowStatement);

			ast_ref<Expression> m_syn_expression;

		public:
			ThrowStatement(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_expression(const ast_ptr<Expression>&);

		public:
			void bind(rt::BindContext* context, rt::BindScope* scope) override;

			static gc::Local<rt::Value> create_exception_value(
				const gc::Local<TextPos>& text_pos,
				const RuntimeError& e);

		protected:
			rt::StatementResult execute_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope) override;

		private:
			static gc::Local<rt::Value> create_exception_value(
				const gc::Local<TextPos>& text_pos,
				const gc::Local<rt::Value>& value);
		};

	}
}

#endif//SYNSAMPLE_CORE_AST_STATEMENT_H_INCLUDED
