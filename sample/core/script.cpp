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

//Script execution functions.

#include <iostream>

#include "api.h"
#include "ast_script.h"
#include "gc.h"
#include "name.h"
#include "scanner.h"
#include "scope.h"
#include "script.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace ast = ss::ast;
namespace rt = ss::rt;
namespace gc = ss::gc;

//
//ScriptSource
//

void rt::ScriptSource::gc_enumerate_refs() {
	Object::gc_enumerate_refs();
	gc_ref(m_file_name);
	gc_ref(m_code);
}

void rt::ScriptSource::initialize(const ss::StringLoc& file_name, const ss::StringLoc& code) {
	m_file_name = file_name;
	m_code = code;
}

const ss::StringRef& rt::ScriptSource::get_file_name() const {
	return m_file_name;
}

const ss::StringRef& rt::ScriptSource::get_code() const {
	return m_code;
}

//
//(Helper Classes and Functions)
//

namespace {
	typedef gc::Array<ast::Script> ScriptArray;

	//
	//TopScriptScopeInitializer
	//

	class TopScriptScopeInitializer : public rt::ScriptScopeInitializer {
		NONCOPYABLE(TopScriptScopeInitializer);

		gc::Local<rt::NameDescriptor> m_sys_name_desc;

	public:
		TopScriptScopeInitializer(){}

		void bind(ss::NameRegistry& name_registry, rt::BindScope& scope) override {
			gc::Local<const ss::NameInfo> sys_name_info = name_registry.register_name("sys");
			m_sys_name_desc = scope.declare_sys_constant(sys_name_info);
		}

		void exec(const gc::Local<rt::ExecContext>& context, const gc::Local<rt::ExecScope>& scope) override {
			m_sys_name_desc->set_initialize(scope, rt::create_sys_namespace_value(context));
		}
	};

	ast::ast_ptr<ast::Script> parse_script(ss::NameTable& name_table, const gc::Local<rt::ScriptSource>& source) {
		ss::NameRegistry name_registry(name_table);
		ss::Scanner scanner(name_registry, source->get_file_name(), source->get_code());

		try {
			ast::ast_ptr<ast::Script> script = ss::syngen::SynParser::parse_Script(scanner);
			assert(!!script);
			return script;
		} catch (syn::SynSyntaxError&) {
			throw ss::CompilationError(scanner.get_text_pos(), "Syntax error");
		} catch (const syn::SynError&) {
			throw ss::CompilationError("Parser error");
		}
	}

	gc::Local<ScriptArray> parse_scripts(
		ss::NameTable& name_table,
		const gc::Local<gc::Array<rt::ScriptSource>>& sources)
	{
		const std::size_t n = sources->length();
		gc::Local<ScriptArray> scripts = ScriptArray::create(n);
		for (std::size_t i = 0; i < n; ++i) (*scripts)[i] = parse_script(name_table, (*sources)[i]);
		return scripts;
	}

	bool get_return_value(const gc::Local<rt::ExecContext>& context, const rt::StatementResult& result) {
		if (rt::StatementResultType::THROW == result.get_type()) {
			const rt::Value* value0 = result.get_value().get();
			const rt::ExceptionValue* value = dynamic_cast<const rt::ExceptionValue*>(value0);
			assert(value);

			std::cout << "*** Unhandled exception ***" << std::endl;
			value->print_stack_trace(context);

			return false;
		}

		return true;
	}

	std::unique_ptr<rt::ValueFactory> create_value_factory(
		ss::NameTable& name_table,
		const gc::Local<ss::StringArray>& arguments)
	{
		ss::NameRegistry name_registry(name_table);
		return std::unique_ptr<rt::ValueFactory>(new rt::ValueFactory(name_registry, arguments));
	}

	gc::Local<rt::ScopeDescriptor> bind_scripts(
		ss::NameTable& name_table,
		rt::BindContext& bind_context,
		rt::ScriptScopeInitializer& initializer,
		const gc::Local<ScriptArray>& scripts)
	{
		std::unique_ptr<rt::BindScope> bind_scope = bind_context.create_root_scope();

		ss::NameRegistry name_registry(name_table);
		initializer.bind(name_registry, *bind_scope);

		for (const gc::Ref<ast::Script>& script : *scripts) {
			script->get_block()->bind_declare(&bind_context, bind_scope.get());
		}
		for (const gc::Ref<ast::Script>& script : *scripts) {
			script->get_block()->bind_define(&bind_context, bind_scope.get());
		}

		return bind_scope->create_scope_descriptor();
	}

	rt::StatementResult exec_scripts(
		const gc::Local<rt::ExecContext>& context,
		const gc::Local<rt::ScopeDescriptor>& scope_descriptor,
		rt::ScriptScopeInitializer& initializer,
		const gc::Local<ScriptArray>& scripts)
	{
		gc::Local<rt::ExecScope> scope = context->create_root_scope(scope_descriptor);
		initializer.exec(context, scope);

		rt::StatementResult result = rt::StatementResult::none();
		for (const gc::Ref<ast::Script>& script : *scripts) {
			result = script->get_block()->execute(context, scope);
			if (rt::StatementResultType::THROW == result.get_type()) break;
		}

		return result;
	}
}

//
//get_single_script_source()
//

gc::Local<gc::Array<rt::ScriptSource>> rt::get_single_script_source(
	const ss::StringLoc& file_name,
	const ss::StringLoc& code)
{
	gc::Local<gc::Array<rt::ScriptSource>> sources = gc::Array<rt::ScriptSource>::create(1);
	(*sources)[0] = gc::create<rt::ScriptSource>(file_name, code);
	return sources;
}

//
//execute_top_script()
//

bool rt::execute_top_script(
	const gc::Local<gc::Array<ScriptSource>>& sources,
	const gc::Local<ss::StringArray>& arguments)
{
	TopScriptScopeInitializer initializer;

	ss::NameTable name_table;
	gc::Local<ScriptArray> scripts = parse_scripts(name_table, sources);

	std::unique_ptr<ValueFactory> value_factory = create_value_factory(name_table, arguments);
	rt::BindContext bind_context(name_table, *value_factory);
	gc::Local<rt::ScopeDescriptor> scope_descriptor = bind_scripts(name_table, bind_context, initializer, scripts);

	gc::Local<rt::ExecContext> exec_context = gc::create<rt::ExecContext>(&bind_context);
	rt::StatementResult result = exec_scripts(exec_context, scope_descriptor, initializer, scripts);

	bool ret = get_return_value(exec_context, result);
	return ret;
}

//
//execute_sub_script()
//

rt::StatementResult rt::execute_sub_script(
	const gc::Local<ExecContext>& context,
	const gc::Local<gc::Array<ScriptSource>>& sources,
	ScriptScopeInitializer& initializer)
{
	BindContext* bind_context = context->get_bind_context();
	NameTable& name_table = bind_context->get_name_table();

	gc::Local<ScriptArray> scripts = parse_scripts(name_table, sources);
	gc::Local<rt::ScopeDescriptor> scope_descriptor = bind_scripts(name_table, *bind_context, initializer, scripts);
	rt::StatementResult result = exec_scripts(context, scope_descriptor, initializer, scripts);
	return result;
}
