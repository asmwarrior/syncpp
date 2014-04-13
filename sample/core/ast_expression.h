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

//AST: expression nodes.

#ifndef SYNSAMPLE_CORE_AST_EXPRESSION_H_INCLUDED
#define SYNSAMPLE_CORE_AST_EXPRESSION_H_INCLUDED

#include <memory>

#include "ast.h"
#include "ast__dec.h"
#include "ast_type.h"
#include "gc.h"
#include "op.h"
#include "scope.h"
#include "value__dec.h"

namespace syn_script {
	namespace ast {

		//
		//Expression
		//

		class Expression : public Node {
			NONCOPYABLE(Expression);

		protected:
			Expression(){}

		public:
			virtual gc::Local<TextPos> get_pos() const = 0;
			virtual gc::Local<TextPos> get_start_pos() const;

			virtual bool is_assignment_allowed() const;
			virtual bool is_invocation_allowed() const;
			virtual bool is_instantiation_allowed() const;

			virtual void bind(rt::BindContext* context, rt::BindScope* scope) = 0;

			gc::Local<rt::Value> evaluate(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception);

			gc::Local<rt::Value> modify(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				rt::ValueModifier& modifier);

		protected:
			virtual gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) = 0;

			virtual gc::Local<rt::Value> modify_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				rt::ValueModifier& modifier);
		};

		//
		//ConditionalExpression
		//

		class ConditionalExpression : public Expression {
			NONCOPYABLE(ConditionalExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_condition;
			ast_ref<Expression> m_syn_true_expression;
			ast_ref<Expression> m_syn_false_expression;

		public:
			ConditionalExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_condition(const ast_ptr<Expression>&);
			void syn_true_expression(const ast_ptr<Expression>&);
			void syn_false_expression(const ast_ptr<Expression>&);

		public:
			gc::Local<TextPos> get_pos() const override;
			gc::Local<TextPos> get_start_pos() const override;

			bool is_invocation_allowed() const override;
			bool is_instantiation_allowed() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//BinaryExpression
		//

		class BinaryExpression : public Expression {
			NONCOPYABLE(BinaryExpression);

			gc::Ref<TextPos> m_syn_pos;
			AstBinOp m_syn_op;
			ast_ref<Expression> m_syn_left;
			ast_ref<Expression> m_syn_right;

			const rt::BinaryOperator* m_op;

		protected:
			BinaryExpression(){}
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_op(const AstBinOp&);
			void syn_left(const ast_ptr<Expression>&);
			void syn_right(const ast_ptr<Expression>&);

		protected:
			const rt::BinaryOperator* get_op() const;
			const ast_ref<Expression>& get_left() const;
			const ast_ref<Expression>& get_right() const;

		public:
			gc::Local<TextPos> get_pos() const override;
			gc::Local<TextPos> get_start_pos() const override final;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;
		};

		//
		//AssignmentExpression
		//

		class AssignmentExpression : public BinaryExpression {
			NONCOPYABLE(AssignmentExpression);

			class Modifier;

		public:
			AssignmentExpression(){}

			bool is_invocation_allowed() const override;
			bool is_instantiation_allowed() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//RegularBinaryExpression
		//

		class RegularBinaryExpression : public BinaryExpression {
			NONCOPYABLE(RegularBinaryExpression);

		public:
			RegularBinaryExpression(){}

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//UnaryExpression
		//

		class UnaryExpression : public Expression {
			NONCOPYABLE(UnaryExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_expression;

		protected:
			UnaryExpression(){}
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_expression(const ast_ptr<Expression>&);

		public:
			gc::Local<TextPos> get_pos() const override;
			const ast_ref<Expression>& get_expression() const;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;
		};

		//
		//RegularUnaryExpression
		//

		class RegularUnaryExpression : public UnaryExpression {
			NONCOPYABLE(RegularUnaryExpression);

			AstUnOp m_syn_op;

			const rt::UnaryOperator* m_op;

		public:
			RegularUnaryExpression(){}

		private:
			friend struct syngen::Actions;
			void syn_op(const AstUnOp&);

		public:
			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//IncrementDecrementExpression
		//

		class IncrementDecrementExpression : public UnaryExpression {
			NONCOPYABLE(IncrementDecrementExpression);

			class Modifier;

			bool m_syn_increment;
			bool m_syn_postfix;

		public:
			IncrementDecrementExpression(){}

		private:
			friend struct syngen::Actions;
			void syn_increment(bool);
			void syn_postfix(bool);

		public:
			gc::Local<TextPos> get_start_pos() const override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//TerminalExpression
		//

		class TerminalExpression : public Expression {
			NONCOPYABLE(TerminalExpression);

		protected:
			TerminalExpression(){}
		};

		//
		//MemberExpression
		//

		class MemberExpression : public TerminalExpression {
			NONCOPYABLE(MemberExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_object;
			gc::Ref<AstName> m_syn_name;

		public:
			MemberExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_object(const ast_ptr<Expression>&);
			void syn_name(const SynName&);

		public:
			gc::Local<TextPos> get_pos() const override;
			gc::Local<TextPos> get_start_pos() const override;

			bool is_assignment_allowed() const override;
			bool is_invocation_allowed() const override;
			bool is_instantiation_allowed() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;

			gc::Local<rt::Value> modify_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				rt::ValueModifier& modifier) override;
		};

		//
		//InvocationExpression
		//

		class InvocationExpression : public TerminalExpression {
			NONCOPYABLE(InvocationExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_function;
			ast_ref<const ast_node_list<Expression>> m_syn_arguments;

		public:
			InvocationExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_function(const ast_ptr<Expression>&);
			void syn_arguments(const ast_ptr<const ast_node_list<Expression>>&);

		public:
			gc::Local<TextPos> get_pos() const override;
			gc::Local<TextPos> get_start_pos() const override;

			bool is_invocation_allowed() const override;
			bool is_instantiation_allowed() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//NewObjectExpression
		//

		class NewObjectExpression : public TerminalExpression {
			NONCOPYABLE(NewObjectExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_type_expr;
			ast_ref<const ast_node_list<Expression>> m_syn_arguments;

		public:
			NewObjectExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_type_expr(const ast_ptr<Expression>&);
			void syn_arguments(const ast_ptr<const ast_node_list<Expression>>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//NewArrayExpression
		//

		class NewArrayExpression : public TerminalExpression {
			NONCOPYABLE(NewArrayExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_length;

		public:
			NewArrayExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_length(const ast_ptr<Expression>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//ArrayExpression
		//

		class ArrayExpression : public TerminalExpression {
			NONCOPYABLE(ArrayExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<const ast_node_list<Expression>> m_syn_expressions;

		public:
			ArrayExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_expressions(const ast_ptr<const ast_node_list<Expression>>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//SubscriptExpression
		//

		class SubscriptExpression : public TerminalExpression {
			NONCOPYABLE(SubscriptExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_array;
			ast_ref<Expression> m_syn_index;

		public:
			SubscriptExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_array(const ast_ptr<Expression>&);
			void syn_index(const ast_ptr<Expression>&);

		public:
			gc::Local<TextPos> get_pos() const override;
			gc::Local<TextPos> get_start_pos() const override;

			bool is_assignment_allowed() const override;
			bool is_invocation_allowed() const override;
			bool is_instantiation_allowed() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;

			gc::Local<rt::Value> modify_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				rt::ValueModifier& modifier) override;

		private:
			std::size_t evaluate_index(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception);
		};

		//
		//NameExpression
		//

		class NameExpression : public TerminalExpression {
			NONCOPYABLE(NameExpression);

			gc::Ref<AstName> m_syn_name;

			rt::ScopeID m_scope_id;
			gc::Ref<rt::NameDescriptor> m_name_descriptor;

		public:
			NameExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_name(const SynName&);

		public:
			gc::Local<TextPos> get_pos() const override;

			bool is_assignment_allowed() const override;
			bool is_invocation_allowed() const override;
			bool is_instantiation_allowed() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		
			gc::Local<rt::Value> modify_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				rt::ValueModifier& modifier) override;
		};

		//
		//ThisExpression
		//

		class ThisExpression : public TerminalExpression {
			NONCOPYABLE(ThisExpression);

			gc::Ref<TextPos> m_syn_pos;

			std::size_t m_scope_ofs;

		public:
			ThisExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//FunctionExpression
		//

		class FunctionExpression : public TerminalExpression {
			NONCOPYABLE(FunctionExpression);

			ast_ref<FunctionFormalParameters> m_syn_parameters;
			ast_ref<FunctionBody> m_syn_body;

			gc::Ref<gc::Array<rt::NameDescriptor>> m_parameter_descriptors;
			gc::Ref<rt::ScopeDescriptor> m_scope_descriptor;

		public:
			FunctionExpression(){}

		protected:
			void gc_enumerate_refs() override;

		public:
			void initialize();
			void initialize(const ast_ptr<FunctionFormalParameters>& parameters, const ast_ptr<FunctionBody>& body);

		private:
			friend struct syngen::Actions;
			void syn_parameters(const ast_ptr<FunctionFormalParameters>&);
			void syn_body(const ast_ptr<FunctionBody>&);

		public:
			gc::Local<TextPos> get_pos() const override;
			gc::Local<TextPos> get_start_pos() const override;

			bool is_invocation_allowed() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

			gc::Local<rt::Value> invoke(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				const gc::Local<gc::Array<rt::Value>>& arguments,
				gc::Local<rt::Value>& exception) const;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//ClassExpression
		//

		class ClassExpression : public TerminalExpression {
			NONCOPYABLE(ClassExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<ClassBody> m_syn_body;

			gc::Ref<rt::ScopeDescriptor> m_scope_descriptor;

		public:
			ClassExpression(){}

		protected:
			void gc_enumerate_refs() override;

		public:
			void initialize();
			void initialize(const SynPos& pos, const ast_ptr<ClassBody>& body);

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_body(const ast_ptr<ClassBody>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			bool is_instantiation_allowed() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

			gc::Local<rt::Value> instantiate(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				const gc::Local<gc::Array<rt::Value>>& arguments,
				gc::Local<rt::Value>& exception) const;

			gc::Local<rt::Value> get_object_member(
				const gc::Local<rt::ExecScope>& object_scope,
				const gc::Local<rt::ExecScope>& access_scope,
				const gc::Local<const NameInfo>& name_info) const;

			void set_object_member(
				const gc::Local<rt::ExecScope>& object_scope,
				const gc::Local<rt::ExecScope>& access_scope,
				const gc::Local<const NameInfo>& name_info,
				const gc::Local<rt::Value>& value) const;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;

		private:
			const ast_ref<Declaration>& access_declaration(
				const gc::Local<rt::ExecScope>& access_scope,
				const gc::Local<const NameInfo>& name_info) const;

			const ast_ref<ClassMemberDeclaration>& find_declaration(
				const gc::Local<const NameInfo>& name_info) const;
		};

		//
		//LiteralExpression
		//

		class LiteralExpression : public TerminalExpression {
			NONCOPYABLE(LiteralExpression);

		private:
			gc::Ref<rt::Value> m_value;

		protected:
			LiteralExpression(){}
			void gc_enumerate_refs() override {
				TerminalExpression::gc_enumerate_refs();
				gc_ref(m_value);
			}

		public:
			void bind(rt::BindContext* context, rt::BindScope* scope) override final;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override final;
			
			virtual gc::Local<rt::Value> get_runtime_value(rt::BindContext* context) const = 0;
		};

		//
		//IntegerLiteralExpression
		//

		class IntegerLiteralExpression : public LiteralExpression {
			NONCOPYABLE(IntegerLiteralExpression);

			gc::Ref<AstInteger> m_syn_value;

		public:
			IntegerLiteralExpression(){}
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_value(const SynInteger&);

		public:
			gc::Local<TextPos> get_pos() const override;

		protected:
			gc::Local<rt::Value> get_runtime_value(rt::BindContext* context) const override;
		};

		//
		//FloatingPointLiteralExpression
		//

		class FloatingPointLiteralExpression : public LiteralExpression {
			NONCOPYABLE(FloatingPointLiteralExpression);

			gc::Ref<AstFloat> m_syn_value;

		public:
			FloatingPointLiteralExpression(){}
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_value(const SynFloat&);

		public:
			gc::Local<TextPos> get_pos() const override;

		protected:
			gc::Local<rt::Value> get_runtime_value(rt::BindContext* context) const override;
		};

		//
		//StringLiteralExpression
		//

		class StringLiteralExpression : public LiteralExpression {
			NONCOPYABLE(StringLiteralExpression);

			gc::Ref<AstString> m_syn_value;

		public:
			StringLiteralExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_value(const SynString&);

		public:
			gc::Local<TextPos> get_pos() const override;

		protected:
			gc::Local<rt::Value> get_runtime_value(rt::BindContext* context) const override;
		};

		//
		//BooleanLiteralExpression
		//

		class BooleanLiteralExpression : public Expression {
			NONCOPYABLE(BooleanLiteralExpression);

			gc::Ref<TextPos> m_syn_pos;
			bool m_syn_value;

		public:
			BooleanLiteralExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_value(bool);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//NullExpression
		//

		class NullExpression : public Expression {
			NONCOPYABLE(NullExpression);

			gc::Ref<TextPos> m_syn_pos;

		public:
			NullExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//TypeofExpression
		//

		class TypeofExpression : public TerminalExpression {
			NONCOPYABLE(TypeofExpression);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Expression> m_syn_expression;

		public:
			TypeofExpression(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_expression(const ast_ptr<Expression>&);

		public:
			gc::Local<TextPos> get_pos() const override;

			void bind(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<rt::Value> evaluate_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception) override;
		};

	}
}

#endif//SYNSAMPLE_CORE_AST_EXPRESSION_H_INCLUDED
