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

//Script execution API.

#include <algorithm>
#include <memory>

#include "api.h"
#include "api_collection.h"
#include "common.h"
#include "gc.h"
#include "name.h"
#include "scope.h"
#include "script.h"
#include "sysclass.h"
#include "sysclassbld__imp.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace rt = ss::rt;

namespace {
	typedef gc::Array<rt::ScriptSource> SourceArray;
}

namespace syn_script {
	namespace rt {
		gc::Local<Value> api_execute_2(
			const gc::Local<ExecContext>& context,
			const StringLoc& file_name,
			const StringLoc& code);

		gc::Local<Value> api_execute_3(
			const gc::Local<ExecContext>& context,
			const StringLoc& file_name,
			const StringLoc& code,
			const gc::Local<HashMapValue>& scope);

		gc::Local<Value> api_execute_ex(
			const gc::Local<ExecContext>& context,
			const gc::Local<ValueArray>& sources,
			const gc::Local<HashMapValue>& scope);
	}
}

namespace {
	//
	//RootScopeName
	//

	struct RootScopeName {
		gc::Local<rt::NameDescriptor> m_name_descriptor;
		gc::Local<rt::Value> m_value;
	};

	//
	//SubScriptScopeInitializer
	//

	class SubScriptScopeInitializer : public rt::ScriptScopeInitializer {
		NONCOPYABLE(SubScriptScopeInitializer);

		gc::Local<rt::ValueHashMap> m_scope_map;
		std::vector<RootScopeName> m_names;

	public:
		SubScriptScopeInitializer(const gc::Local<rt::ValueHashMap>& scope_map)
			: m_scope_map(scope_map)
		{}

		void bind(ss::NameRegistry& name_registry, rt::BindScope& scope) override {
			if (!m_scope_map) return;

			gc::Local<rt::ValueArray> keys = m_scope_map->keys();
			for (std::size_t i = 0, n = keys->length(); i < n; ++i) {
				gc::Local<rt::Value> key = (*keys)[i];
				ss::StringLoc name = key->get_string();

				gc::Local<rt::Value> value = m_scope_map->get(key);
				assert(!!value);

				RootScopeName name_rec;
				gc::Local<const ss::NameInfo> name_info = name_registry.register_name(name);
				name_rec.m_name_descriptor = scope.declare_sys_constant(name_info);
				name_rec.m_value = value;
				m_names.push_back(name_rec);
			}
		}

		void exec(const gc::Local<rt::ExecContext>& context, const gc::Local<rt::ExecScope>& scope) override {
			for (std::size_t i = 0, n = m_names.size(); i < n; ++i) {
				const RootScopeName& name_rec = m_names[i];
				name_rec.m_name_descriptor->set_initialize(scope, name_rec.m_value);
			}
		}
	};

	rt::StatementResult execute_scripts_code(
		const gc::Local<rt::ExecContext>& context,
		gc::Local<SourceArray> sources,
		rt::ScriptScopeInitializer& initializer)
	{
		try {
			return execute_sub_script(context, sources, initializer);
		} catch (ss::CompilationError& e) {
			throw ss::RuntimeError(e.to_string());
		}
	}

	gc::Local<rt::Value> execute_scripts(
		const gc::Local<rt::ExecContext>& context,
		gc::Local<SourceArray> sources,
		const gc::Local<rt::HashMapValue>& scope)
	{
		gc::Local<rt::ValueHashMap> map;
		if (!!scope) map = scope->get_map();

		SubScriptScopeInitializer initializer(map);
		rt::StatementResult result = execute_scripts_code(context, sources, initializer);

		if (rt::StatementResultType::RETURN == result.get_type()) {
			return result.get_value();
		} else if (rt::StatementResultType::THROW == result.get_type()) {
			//TODO Rethrow the exception.
			const rt::ExceptionValue* e = dynamic_cast<const rt::ExceptionValue*>(result.get_value().get());
			assert(e);
			e->print_stack_trace(context);
			throw ss::RuntimeError("script execution failed");
		} else {
			return context->get_value_factory()->get_null_value();
		}
	}
}

gc::Local<rt::Value> rt::api_execute_2(
	const gc::Local<ExecContext>& context,
	const ss::StringLoc& file_name,
	const ss::StringLoc& code)
{
	return api_execute_3(context, file_name, code, nullptr);
}

gc::Local<rt::Value> rt::api_execute_3(
	const gc::Local<ExecContext>& context,
	const ss::StringLoc& file_name,
	const ss::StringLoc& code,
	const gc::Local<HashMapValue>& scope)
{
	gc::Local<SourceArray> sources = rt::get_single_script_source(file_name, code);
	return execute_scripts(context, sources, scope);
}

gc::Local<rt::Value> rt::api_execute_ex(
	const gc::Local<ExecContext>& context,
	const gc::Local<ValueArray>& sources,
	const gc::Local<HashMapValue>& scope)
{
	const std::size_t count = sources->length();
	gc::Local<SourceArray> adapted_sources = SourceArray::create(count);

	for (std::size_t i = 0; i < count; ++i) {
		gc::Local<Value> value = sources->get(i);
		gc::Local<ArrayValue> array_value = value.cast<ArrayValue>();
		gc::Local<ValueArray> array = array_value->get_array();
		if (2 != array->length()) throw RuntimeError("Invalid argument: array length != 2");

		StringLoc file_name = array->get(0)->get_string();
		StringLoc code = array->get(1)->get_string();
		(*adapted_sources)[i] = gc::create<ScriptSource>(file_name, code);
	}

	return execute_scripts(context, adapted_sources, scope);
}

//
//(Misc.)
//

namespace {
	rt::SysNamespaceInitializer s_sys_namespace_initializer([](rt::SysClassBuilder<rt::SysNamespaceValue>& bld){
		bld.add_static_method("execute", rt::api_execute_2);
		bld.add_static_method("execute", rt::api_execute_3);
		bld.add_static_method("execute_ex", rt::api_execute_ex);
	});
}

void link__api_execute(){}
