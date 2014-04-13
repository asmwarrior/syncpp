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

//AST: script.

#include "api.h"
#include "ast.h"
#include "ast_combined.h"
#include "gc.h"
#include "name.h"
#include "scope.h"
#include "sysclass.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace ast = ss::ast;
namespace rt = ss::rt;
namespace gc = ss::gc;

//
//Script
//

void ast::Script::gc_enumerate_refs() {
	Node::gc_enumerate_refs();
	gc_ref(m_syn_block);
}

void ast::Script::syn_block(const ast_ptr<Block>& block) {
	m_syn_block = block;
}

gc::Local<ast::Block> ast::Script::get_block() const {
	return m_syn_block;
}

//
//Block
//

void ast::Block::gc_enumerate_refs() {
	Node::gc_enumerate_refs();
	gc_ref(m_syn_statements);
	gc_ref(m_declarations);
	gc_ref(m_statements);
}

void ast::Block::syn_statements(const ast_ptr<const ast_node_list<Statement>>& statements) {
	m_syn_statements = statements;
}

void ast::Block::bind(rt::BindContext* context, rt::BindScope* scope) {
	bind_declare(context, scope);
	bind_define(context, scope);
}

void ast::Block::bind_declare(rt::BindContext* context, rt::BindScope* scope) {
	split_statements();
	for (const ast_ref<Declaration>& d : *m_declarations) d->bind_declare(context, scope);
}

void ast::Block::bind_define(rt::BindContext* context, rt::BindScope* scope) {
	for (const ast_ref<Declaration>& d : *m_declarations) d->bind_define(context, scope);
	for (const ast_ref<Statement>& s : *m_statements) s->bind(context, scope);
}

rt::StatementResult ast::Block::execute(
	const gc::Local<rt::ExecContext>& context,
	const gc::Local<rt::ExecScope>& scope)
{
	gc::Local<rt::Value> exception;
	for (const ast_ref<Declaration>& d : *m_declarations) {
		d->exec_define(context, scope, exception);
		if (!!exception) return rt::StatementResult::exception(exception);
	}

	for (const ast_ref<Statement>& s : *m_statements) {
		rt::StatementResult result = s->execute(context, scope);
		if (rt::StatementResultType::NONE != result.get_type()) return result;
	}

	return rt::StatementResult::none();
}

void ast::Block::split_statements() {
	const std::size_t total_cnt = m_syn_statements->size();
	std::size_t decl_cnt = 0;
	while (decl_cnt < total_cnt && !!(*m_syn_statements)[decl_cnt]->get_declaration()) ++decl_cnt;

	ast_ptr<ast_node_list<Declaration>> declarations = gc::Array<Declaration>::create(decl_cnt);
	for (std::size_t i = 0; i < decl_cnt; ++i) {
		(*declarations)[i] = (*m_syn_statements)[i]->get_declaration();
	}
	m_declarations = declarations;

	std::size_t stmt_cnt = total_cnt - decl_cnt;
	ast_ptr<ast_node_list<Statement>> statements = gc::Array<Statement>::create(stmt_cnt);
	for (std::size_t i = 0; i < stmt_cnt; ++i) {
		(*statements)[i] = (*m_syn_statements)[decl_cnt + i];
	}
	m_statements = statements;
}
