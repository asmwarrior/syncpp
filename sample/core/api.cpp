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

//General API framework functionality.

#include <algorithm>
#include <functional>
#include <list>
#include <memory>

#include "api.h"
#include "common.h"
#include "gc.h"
#include "scope.h"
#include "util.h"
#include "sysclass.h"
#include "sysclassbld.h"
#include "sysvalue.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;

namespace {
	typedef std::list<rt::SysNamespaceInitializer::InitFn> SysNamespaceInitList;

	SysNamespaceInitList& get_sys_namespace_initializers() {
		static SysNamespaceInitList lst;
		return lst;
	}

	void init_SysNamespaceValue(rt::SysClassBuilder<rt::SysNamespaceValue>& bld) {
	};

	class SysNamespaceClassInitializer : public rt::BasicSysClassInitializer {
	protected:
		gc::Local<rt::SysClass> create_sys_class(ss::NameRegistry& name_registry) const override {
			rt::SysClassBuilder<rt::SysNamespaceValue> bld(name_registry);
			for (rt::SysNamespaceInitializer::InitFn fn : get_sys_namespace_initializers()) fn(bld);
			return bld.create_sys_class();
		}
	};

	SysNamespaceClassInitializer s_init_sys_namespace;
}

//
//SysNamespaceInitializer
//

rt::SysNamespaceInitializer::SysNamespaceInitializer(InitFn fn) {
	get_sys_namespace_initializers().push_back(fn);
}


gc::Local<rt::Value> rt::create_sys_namespace_value(const gc::Local<rt::ExecContext>& context) {
	std::size_t class_id = s_init_sys_namespace.get_class_id();
	gc::Local<SysClass> cls = context->get_value_factory()->get_sys_class(class_id);
	return gc::create<SysNamespaceValue>(cls);
}

//
//(Functions)
//

//This project is configured as a static library in Visual Studio. For this reason, API modules
//have to be references explicitly in order to be linked and thus loaded at run-time.
//Constructors of global variables are invoked then, activating a module.
void link__api() {
	void link__api_basic();
	void link__api_collections();
	void link__api_execute();
	void link__api_file();
	void link__api_io();
	void link__api_socket();

	link__api_basic();
	link__api_collections();
	link__api_execute();
	link__api_file();
	link__api_io();
	link__api_socket();
}
