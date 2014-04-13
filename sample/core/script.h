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

//Script classes.

#ifndef SYNSAMPLE_CORE_SCRIPT_H_INCLUDED
#define SYNSAMPLE_CORE_SCRIPT_H_INCLUDED

#include <string>

#include "scope.h"
#include "stringex.h"

namespace syn_script {
	namespace rt {

		//
		//ScriptScopeInitializer
		//

		class ScriptScopeInitializer {
			NONCOPYABLE(ScriptScopeInitializer);

		protected:
			ScriptScopeInitializer(){}

		public:
			virtual void bind(NameRegistry& name_registry, BindScope& scope) = 0;
			virtual void exec(const gc::Local<ExecContext>& context, const gc::Local<ExecScope>& scope) = 0;
		};

		//
		//ScriptSource
		//

		class ScriptSource : public gc::Object {
			NONCOPYABLE(ScriptSource);

			StringRef m_file_name;
			StringRef m_code;

		public:
			ScriptSource(){}
			void gc_enumerate_refs() override;
			void initialize(const StringLoc& file_name, const StringLoc& code);

			const StringRef& get_file_name() const;
			const StringRef& get_code() const;
		};

		//
		//(Functions)
		//

		gc::Local<gc::Array<ScriptSource>> get_single_script_source(
			const StringLoc& file_name,
			const StringLoc& code);

		bool execute_top_script(
			const gc::Local<gc::Array<ScriptSource>>& sources,
			const gc::Local<StringArray>& arguments);

		StatementResult execute_sub_script(
			const gc::Local<ExecContext>& context,
			const gc::Local<gc::Array<ScriptSource>>& sources,
			ScriptScopeInitializer& initializer);

	}
}

#endif//SYNSAMPLE_CORE_SCRIPT_H_INCLUDED
