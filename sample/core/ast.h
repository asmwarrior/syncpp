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

//General definitions for AST.

#ifndef SYNSAMPLE_CORE_AST_H_INCLUDED
#define SYNSAMPLE_CORE_AST_H_INCLUDED

#include <memory>
#include <vector>

#include "ast__dec.h"
#include "gc.h"
#include "noncopyable.h"

namespace syn_script {
	namespace ast {

		template<class T> using ast_ptr = gc::Local<T>;
		template<class T> using ast_ref = gc::Ref<T>;
		template<class T> using ast_list = gc::Array<T>;
		template<class T> using ast_node_list = gc::Array<T>;

		//
		//AstAllocator
		//

		struct AstAllocator {
			template<class T> using Ptr = ast_ptr<T>;
			template<class T> using List = ast_list<T>;
			template<class T> using NodeList = ast_node_list<T>;

			template<class T>
			static gc::Local<T> create() {
				return gc::create<T>();
			}

		private:
			template<class A, class T>
			static gc::Local<A> create_list_0(const std::vector<T>& v) {
				std::size_t len = v.size();
				gc::Local<A> array = A::create(len);
				A& array_ref = *array;
				for (std::size_t i = 0; i < len; ++i) array_ref[i] = v[i];
				return array;
			}

		public:
			template<class T>
			static gc::Local<gc::Array<T>> list(std::unique_ptr<std::vector<gc::Local<T>>> v) {
				return create_list_0<gc::Array<T>>(*v);
			}

			template<class T>
			static gc::Local<gc::Array<T>> node_list(std::unique_ptr<std::vector<gc::Local<T>>> v) {
				return create_list_0<gc::Array<T>>(*v);
			}
		};

		//
		//Node
		//

		//Using inheritance: typedef will not work, because the declaration 'class Node;' exists.
		class Node : public gc::Object {
			NONCOPYABLE(Node);

		protected:
			Node(){}
		};

	}

	namespace syngen {
		struct Actions;
	}
}

#endif//SYNSAMPLE_CORE_AST_H_INCLUDED
