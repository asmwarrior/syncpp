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

//Framework for Script Language API.

#ifndef SYNSAMPLE_CORE_API_H_INCLUDED
#define SYNSAMPLE_CORE_API_H_INCLUDED

#include <functional>
#include <list>
#include <memory>

#include "api__dec.h"
#include "gc.h"
#include "scope__dec.h"
#include "sysclassbld.h"
#include "sysvalue.h"
#include "value__dec.h"

namespace syn_script {
	namespace rt {

		//
		//SysNamespaceInitializer
		//

		//Used to register a callback function intended to register names in the system namespace.
		class SysNamespaceInitializer {
			NONCOPYABLE(SysNamespaceInitializer);

		public:
			typedef std::function<void(SysClassBuilder<SysNamespaceValue>&)> InitFn;

		public:
			SysNamespaceInitializer(InitFn fn);
		};

		gc::Local<Value> create_sys_namespace_value(const gc::Local<rt::ExecContext>& context);

	}
}

#endif//SYNSAMPLE_CORE_API_H_INCLUDED
