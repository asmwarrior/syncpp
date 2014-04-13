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

//Collections API values.

#ifndef SYNSAMPLE_CORE_API_COLLECTION_H_INCLUDED
#define SYNSAMPLE_CORE_API_COLLECTION_H_INCLUDED

#include "api.h"
#include "gc.h"
#include "gc_hashmap.h"
#include "scope__dec.h"
#include "sysclassbld.h"
#include "sysvalue.h"
#include "value__dec.h"

namespace syn_script {
	namespace rt {

		inline std::size_t value_hash_fn(const gc::Local<rt::Value>& key) {
			assert(!!key);
			return key->value_hash_code();
		}

		inline bool value_equals_fn(const gc::Local<rt::Value>& key1, const gc::Local<rt::Value>& key2) {
			assert(!!key1);
			assert(!!key2);
			return key1->value_equals(key2);
		}

		typedef gc::BasicGenericHashMap<rt::Value, rt::Value, value_hash_fn, value_equals_fn> ValueHashMap;

		//
		//HashMapValue
		//

		class HashMapValue : public SysObjectValue {
			NONCOPYABLE(HashMapValue);

			gc::Ref<ValueHashMap> m_map;

		public:
			HashMapValue(){}
			void gc_enumerate_refs() override;
			void initialize();

			const gc::Ref<ValueHashMap>& get_map() const;

		protected:
			std::size_t get_sys_class_id() const override;

		private:
			void check_key(const gc::Local<Value>& key) const;

			gc::Local<Value> external_value(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& value) const;

			static gc::Local<HashMapValue> api_create(const gc::Local<ExecContext>& context);

			bool api_is_empty(const gc::Local<ExecContext>& context);
			ScriptIntegerType api_size(const gc::Local<ExecContext>& context);
			void api_clear(const gc::Local<ExecContext>& context);
			bool api_contains(const gc::Local<ExecContext>& context, const gc::Local<Value>& key);
			gc::Local<Value> api_get(const gc::Local<ExecContext>& context, const gc::Local<Value>& key);
			gc::Local<Value> api_remove(const gc::Local<ExecContext>& context, const gc::Local<Value>& key);
			gc::Local<gc::Array<Value>> api_keys(const gc::Local<ExecContext>& context);
			gc::Local<gc::Array<Value>> api_values(const gc::Local<ExecContext>& context);

			gc::Local<Value> api_put(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& key,
				const gc::Local<Value>& value);

		public:
			class API;
		};

	}
}

#endif//SYNSAMPLE_CORE_API_COLLECTION_H_INCLUDED
