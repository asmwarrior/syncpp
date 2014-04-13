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

//Value classes definitions.

#ifndef SYNSAMPLE_CORE_VALUE_H_INCLUDED
#define SYNSAMPLE_CORE_VALUE_H_INCLUDED

#include "ast_type.h"
#include "ast_expression.h"
#include "gc.h"
#include "name__dec.h"
#include "value__dec.h"
#include "scope__dec.h"
#include "stringex.h"
#include "sysclass__dec.h"

namespace syn_script {
	namespace rt {

		//
		//Value
		//

		//Base class for a Script Language value.
		class Value : public ObjectEx {
			NONCOPYABLE(Value);

		protected:
			Value(){}

		public:
			virtual bool is_undefined() const;
			virtual bool is_void() const;
			virtual bool is_null() const;

			virtual bool get_boolean() const;
			virtual ScriptIntegerType get_integer() const;
			virtual ScriptFloatType get_float() const;
			virtual StringLoc get_string() const;
			virtual StringLoc to_string(const gc::Local<ExecContext>& context) const;

			virtual OperandType get_operand_type() const;

			virtual bool iterate(InternalValueIterator& iterator);

			virtual gc::Local<Value> get_member(
				const gc::Local<ExecContext>& context,
				const gc::Local<ExecScope>& scope,
				const gc::Local<const NameInfo>& name_info);

			virtual gc::Local<Value> get_array_element(const gc::Local<ExecContext>& context, std::size_t index);

			virtual void set_member(
				const gc::Local<ExecContext>& context,
				const gc::Local<ExecScope>& scope,
				const gc::Local<const NameInfo>& name_info,
				const gc::Local<Value>& value);

			virtual void set_array_element(
				const gc::Local<ExecContext>& context,
				std::size_t index,
				const gc::Local<Value>& value);

			virtual gc::Local<Value> invoke(
				const gc::Local<ExecContext>& context,
				const gc::Local<gc::Array<Value>>& arguments,
				gc::Local<Value>& exception);

			virtual gc::Local<Value> instantiate(
				const gc::Local<ExecContext>& context,
				const gc::Local<gc::Array<Value>>& arguments,
				gc::Local<Value>& exception);

			virtual StringLoc typeof(const gc::Local<ExecContext>& context) const;

			virtual bool value_equals(const gc::Local<const Value>& value) const;
			virtual std::size_t value_hash_code() const;
			virtual int value_compare_to(const gc::Local<Value>& value) const;
		};

		//
		//ReferenceValue
		//

		class ReferenceValue : public Value {
			NONCOPYABLE(ReferenceValue);

		protected:
			ReferenceValue(){}

			OperandType get_operand_type() const override;
		};

		//
		//ValueModifier
		//

		class ValueModifier {
			NONCOPYABLE(ValueModifier);

		protected:
			ValueModifier(){}

		public:
			virtual gc::Local<Value> modify_short(gc::Local<Value>& result) { return nullptr; }
			virtual gc::Local<Value> modify(const gc::Local<Value>& value, gc::Local<Value>& result) = 0;
		};

		//
		//InternalValueIterator
		//

		class InternalValueIterator {
			NONCOPYABLE(InternalValueIterator);

		public:
			InternalValueIterator(){}

			virtual bool iterate(const gc::Local<Value>& value) = 0;
		};

		//
		//ValueFactory
		//

		class ValueFactory {
			NONCOPYABLE(ValueFactory);

			static const int INT_CACHE_MIN = -1024;
			static const int INT_CACHE_MAX = 1024;
			static const int FLOAT_CACHE_MIN = INT_CACHE_MIN;
			static const int FLOAT_CACHE_MAX = INT_CACHE_MAX;
			static const std::size_t CHAR_CACHE_SIZE = 256;

			gc::Local<Value> m_undefined_value;
			gc::Local<Value> m_void_value;
			gc::Local<Value> m_null_value;
			gc::Local<Value> m_false_value;
			gc::Local<Value> m_true_value;

			gc::Local<Value> m_arguments_value;

			gc::Local<ValueArray> m_int_cache;
			gc::Local<ValueArray> m_float_cache;
			gc::Local<ValueArray> m_char_str_cache;

			StringLoc m_empty_str;
			StringLoc m_null_str;
			StringLoc m_false_str;
			StringLoc m_true_str;

			gc::Local<gc::Array<SysClass>> m_sys_classes;

		public:
			ValueFactory(NameRegistry& name_registry, const gc::Local<StringArray>& arguments);

		private:
			static gc::Local<Value> create_arguments_value(const gc::Local<StringArray>& arguments);
			static gc::Local<ValueArray> create_int_cache();
			static gc::Local<ValueArray> create_float_cache();
			static gc::Local<ValueArray> create_char_str_cache();
			void init_sys_classes(NameRegistry& name_registry);

		public:
			gc::Local<Value> get_arguments_value() const;

			gc::Local<Value> get_undefined_value() const;
			gc::Local<Value> get_void_value() const;
			gc::Local<Value> get_null_value() const;
			gc::Local<Value> get_boolean_value(bool value) const;
			gc::Local<Value> get_integer_value(ScriptIntegerType value) const;
			gc::Local<Value> get_float_value(ScriptFloatType value) const;
			gc::Local<Value> get_string_value(const StringLoc& value) const;
			gc::Local<Value> get_char_string_value(char c) const;

			StringLoc get_empty_str() const;
			StringLoc get_null_str() const;
			StringLoc get_false_str() const;
			StringLoc get_true_str() const;

			gc::Local<SysClass> get_sys_class(std::size_t class_id) const;
		};

	}
}

#endif//SYNSAMPLE_CORE_VALUE_H_INCLUDED
