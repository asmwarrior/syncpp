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

//AST: declaration nodes.

#ifndef SYNSAMPLE_CORE_AST_DECLARATION_H_INCLUDED
#define SYNSAMPLE_CORE_AST_DECLARATION_H_INCLUDED

#include <memory>

#include "ast.h"
#include "ast__dec.h"
#include "ast_type.h"
#include "gc.h"
#include "scope.h"

namespace syn_script {
	namespace ast {

		//
		//Declaration
		//

		class Declaration : public Node {
			NONCOPYABLE(Declaration);
		
			gc::Ref<TextPos> m_syn_pos;
			gc::Ref<AstName> m_syn_name;

			rt::ScopeID m_scope_id;
			gc::Ref<const rt::NameDescriptor> m_name_descriptor;

		protected:
			Declaration(){}
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_name(const SynName&);

		public:
			const gc::Ref<TextPos>& get_pos() const;
			const gc::Ref<AstName>& get_name() const;
			const rt::NameDescriptor* get_name_descriptor() const;
			virtual ast_ptr<FunctionDeclaration> get_function_opt();
			virtual ModifierType get_default_access() const;

			void bind_declare(rt::BindContext* context, rt::BindScope* scope);
			virtual void bind_define(rt::BindContext* context, rt::BindScope* scope) = 0;

			void exec_define(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception);

		protected:
			virtual rt::DeclarationType get_declaration_type() const = 0;

			virtual gc::Local<const rt::NameDescriptor> bind_declare_0(
				rt::BindContext* context,
				rt::BindScope* scope) = 0;

			virtual void exec_define_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				const rt::NameDescriptor* name_desc) = 0;
		};

		//
		//VariableDeclaration
		//

		class VariableDeclaration : public Declaration {
			NONCOPYABLE(VariableDeclaration);

			ast_ref<Expression> m_syn_expression;

		public:
			VariableDeclaration(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_expression(const ast_ptr<Expression>&);

		protected:
			rt::DeclarationType get_declaration_type() const override;

			gc::Local<const rt::NameDescriptor> bind_declare_0(
				rt::BindContext* context,
				rt::BindScope* scope) override;

			void bind_define(rt::BindContext* context, rt::BindScope* scope) override;

			void exec_define_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				const rt::NameDescriptor* name_desc) override;
		};

		//
		//ConstantDeclaration
		//

		class ConstantDeclaration : public Declaration {
			NONCOPYABLE(ConstantDeclaration);

			ast_ref<Expression> m_syn_expression;

		public:
			ConstantDeclaration(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_expression(const ast_ptr<Expression>&);

		protected:
			rt::DeclarationType get_declaration_type() const override;

			gc::Local<const rt::NameDescriptor> bind_declare_0(
				rt::BindContext* context,
				rt::BindScope* scope) override;

			void bind_define(rt::BindContext* context, rt::BindScope* scope) override;

			void exec_define_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				const rt::NameDescriptor* name_desc) override;
		};

		//
		//FunctionDeclaration
		//

		class FunctionDeclaration : public Declaration {
			NONCOPYABLE(FunctionDeclaration);

			ast_ref<FunctionFormalParameters> m_syn_parameters;
			ast_ref<FunctionBody> m_syn_body;

			ast_ref<FunctionExpression> m_expression;

		public:
			FunctionDeclaration(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_parameters(const ast_ptr<FunctionFormalParameters>&);
			void syn_body(const ast_ptr<FunctionBody>&);

		public:
			ast_ptr<FunctionDeclaration> get_function_opt() override;
			ModifierType get_default_access() const override;
			const ast_ref<FunctionExpression>& get_expression() const;
			rt::DeclarationType get_declaration_type() const override;

			void bind_define(rt::BindContext* context, rt::BindScope* scope) override;

		protected:
			gc::Local<const rt::NameDescriptor> bind_declare_0(
				rt::BindContext* context,
				rt::BindScope* scope) override;

			void exec_define_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				const rt::NameDescriptor* name_desc) override;
		};

		//
		//FunctionFormalParameters
		//

		class FunctionFormalParameters : public Node {
			NONCOPYABLE(FunctionFormalParameters);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<const ast_node_list<AstName>> m_syn_parameters;

		public:
			FunctionFormalParameters(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_parameters(const ast_ptr<const ast_node_list<AstName>>&);

		public:
			const gc::Ref<TextPos>& get_pos() const;
			const ast_ref<const ast_node_list<AstName>>& get_parameters() const;
		};

		//
		//FunctionBody
		//

		class FunctionBody : public Node {
			NONCOPYABLE(FunctionBody);

			gc::Ref<TextPos> m_syn_pos;
			ast_ref<Block> m_syn_block;

		public:
			FunctionBody(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_pos(const SynPos&);
			void syn_block(const ast_ptr<Block>&);

		public:
			const gc::Ref<TextPos>& get_pos() const;
			const ast_ref<Block>& get_block();
		};

		//
		//ClassDeclaration
		//

		class ClassDeclaration : public Declaration {
			NONCOPYABLE(ClassDeclaration);

			ast_ref<ClassBody> m_syn_body;

			ast_ref<ClassExpression> m_expression;

		public:
			ClassDeclaration(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_body(const ast_ptr<ClassBody>&);

		public:
			const ast_ref<ClassExpression>& get_expression() const;

		protected:
			rt::DeclarationType get_declaration_type() const override;

			gc::Local<const rt::NameDescriptor> bind_declare_0(
				rt::BindContext* context,
				rt::BindScope* scope) override;

			void bind_define(rt::BindContext* context, rt::BindScope* scope) override;

			void exec_define_0(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope,
				gc::Local<rt::Value>& exception,
				const rt::NameDescriptor* name_desc) override;
		};

		//
		//ClassBody
		//

		class ClassBody : public Node {
			NONCOPYABLE(ClassBody);

			ast_ref<const ast_node_list<ClassMemberDeclaration>> m_syn_members;

			ast_ref<FunctionDeclaration> m_constructor;

		public:
			ClassBody(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_members(const ast_ptr<const ast_node_list<ClassMemberDeclaration>>&);

		public:
			void bind_constructor();
			const ast_ref<const ast_node_list<ClassMemberDeclaration>>& get_members() const;
			const ast_ref<FunctionDeclaration>& get_constructor();
		};

		//
		//ClassMemberDeclaration
		//

		class ClassMemberDeclaration : public Node {
			NONCOPYABLE(ClassMemberDeclaration);

			ModifierType m_syn_modifier;
			ast_ref<Declaration> m_syn_declaration;
			bool m_private;

		public:
			ClassMemberDeclaration();

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_modifier(ModifierType);
			void syn_declaration(const ast_ptr<Declaration>&);

		public:
			const ast_ref<Declaration>& get_declaration() const;
			bool is_private() const;

			void bind_declare(rt::BindContext* context, rt::BindScope* scope);
			void bind_define(rt::BindContext* context, rt::BindScope* scope);
		};

	}
}

#endif//SYNSAMPLE_CORE_AST_DECLARATION_H_INCLUDED
