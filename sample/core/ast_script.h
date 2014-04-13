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

//AST: script and block.

#ifndef SYNSAMPLE_CORE_AST_SCRIPT_H_INCLUDED
#define SYNSAMPLE_CORE_AST_SCRIPT_H_INCLUDED

#include <memory>
#include <vector>

#include "ast.h"
#include "ast__dec.h"
#include "ast_type.h"
#include "name__dec.h"
#include "scope.h"
#include "sysclass__dec.h"

namespace syn_script {
	namespace ast {

		//
		//Script
		//

		class Script : public Node {
			NONCOPYABLE(Script);

			ast_ref<Block> m_syn_block;

		public:
			Script(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_block(const ast_ptr<Block>&);

		public:
			gc::Local<Block> get_block() const;
		};

		//
		//Block
		//

		class Block : public Node {
			NONCOPYABLE(Block);

			ast_ref<const ast_node_list<Statement>> m_syn_statements;

			ast_ref<const ast_node_list<Declaration>> m_declarations;
			ast_ref<const ast_node_list<Statement>> m_statements;

		public:
			Block(){}

		protected:
			void gc_enumerate_refs() override;

		private:
			friend struct syngen::Actions;
			void syn_statements(const ast_ptr<const ast_node_list<Statement>>&);

		public:
			void bind(rt::BindContext* context, rt::BindScope* scope);
			void bind_declare(rt::BindContext* context, rt::BindScope* scope);
			void bind_define(rt::BindContext* context, rt::BindScope* scope);
			rt::StatementResult execute(
				const gc::Local<rt::ExecContext>& context,
				const gc::Local<rt::ExecScope>& scope);

		private:
			void split_statements();
		};
	}
}

#endif//SYNSAMPLE_CORE_AST_SCRIPT_H_INCLUDED
