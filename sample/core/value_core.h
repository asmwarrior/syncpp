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

//Definitions of core value classes.

#ifndef SYNSAMPLE_CORE_VALUE_CORE_H_INCLUDED
#define SYNSAMPLE_CORE_VALUE_CORE_H_INCLUDED

#include <ostream>

#include "api__dec.h"
#include "ast__dec.h"
#include "ast_type.h"
#include "gc.h"
#include "name__dec.h"
#include "value.h"
#include "scope.h"
#include "stacktrace.h"
#include "stringex.h"
#include "sysclassbld.h"
#include "sysvalue.h"

namespace syn_script {
	namespace rt {

		//
		//LiteralValue
		//

		template<class T> class LiteralValue : public Value {
			NONCOPYABLE(LiteralValue);

			T m_value;

		public:
			LiteralValue(){}
			void initialize(const T& value) { m_value = value; }

			const T& get_value() const { return m_value; }
		};

		//
		//BooleanValue
		//

		class BooleanValue : public LiteralValue<bool> {
			NONCOPYABLE(BooleanValue);

		public:
			BooleanValue(){}

			OperandType get_operand_type() const override;
			bool get_boolean() const override;
			StringLoc to_string(const gc::Local<ExecContext>& context) const override;
			StringLoc typeof(const gc::Local<ExecContext>& context) const override;

			bool value_equals(const gc::Local<const Value>& value) const override;
			std::size_t value_hash_code() const override;
			int value_compare_to(const gc::Local<Value>& value) const override;
		};

		//
		//IntegerValue
		//

		class IntegerValue : public LiteralValue<ScriptIntegerType> {
			NONCOPYABLE(IntegerValue);

		public:
			IntegerValue(){}

			OperandType get_operand_type() const override;
			ScriptIntegerType get_integer() const override;
			StringLoc to_string(const gc::Local<ExecContext>& context) const override;
			StringLoc typeof(const gc::Local<ExecContext>& context) const override;

			bool value_equals(const gc::Local<const Value>& value) const override;
			std::size_t value_hash_code() const override;
			int value_compare_to(const gc::Local<Value>& value) const override;
		};

		//
		//FloatValue
		//

		class FloatValue : public LiteralValue<ScriptFloatType> {
			NONCOPYABLE(FloatValue);

		public:
			FloatValue(){}

			OperandType get_operand_type() const override;
			ScriptFloatType get_float() const override;
			StringLoc to_string(const gc::Local<ExecContext>& context) const override;
			StringLoc typeof(const gc::Local<ExecContext>& context) const override;
			int value_compare_to(const gc::Local<Value>& value) const override;
		};

		//
		//UndefinedValue
		//

		class UndefinedValue : public Value {
			NONCOPYABLE(UndefinedValue);

		public:
			UndefinedValue(){}

			bool is_undefined() const override;
		};

		//
		//VoidValue
		//

		class VoidValue : public Value {
			NONCOPYABLE(VoidValue);

		public:
			VoidValue(){}

			bool is_void() const override;
		};

		//
		//NullValue
		//

		class NullValue : public ReferenceValue {
			NONCOPYABLE(NullValue);

		public:
			NullValue(){}

			bool is_null() const override;
			StringLoc to_string(const gc::Local<ExecContext>& context) const override;

			bool iterate(InternalValueIterator& iterator) override;

			gc::Local<Value> get_member(
				const gc::Local<ExecContext>& context,
				const gc::Local<ExecScope>& scope,
				const gc::Local<const NameInfo>& name_info) override;

			gc::Local<Value> get_array_element(const gc::Local<ExecContext>& context, std::size_t index) override;

			gc::Local<Value> invoke(
				const gc::Local<ExecContext>& context,
				const gc::Local<gc::Array<Value>>& arguments,
				gc::Local<rt::Value>& exception) override;

			gc::Local<Value> instantiate(
				const gc::Local<ExecContext>& context,
				const gc::Local<gc::Array<Value>>& arguments,
				gc::Local<rt::Value>& exception) override;

			StringLoc typeof(const gc::Local<ExecContext>& context) const override;

			bool value_equals(const gc::Local<const Value>& value) const override;
			std::size_t value_hash_code() const override;
			int value_compare_to(const gc::Local<Value>& value) const override;

		private:
			RuntimeError null_pointer_error() const;
		};

		//
		//ArrayValue
		//

		class ArrayValue : public SysObjectValue {
			NONCOPYABLE(ArrayValue);

			friend class SysAPI<ArrayValue>;
			class API;

			gc::Ref<gc::Array<Value>> m_array;

		public:
			ArrayValue(){}
			void gc_enumerate_refs() override;
			void initialize(const gc::Local<gc::Array<Value>>& array);

			StringLoc to_string(const gc::Local<ExecContext>& context) const override;
			bool iterate(InternalValueIterator& iterator) override;

			gc::Local<Value> get_array_element(const gc::Local<ExecContext>& context, std::size_t index) override;

			void set_array_element(
				const gc::Local<ExecContext>& context,
				std::size_t index,
				const gc::Local<Value>& value) override;

			StringLoc typeof(const gc::Local<ExecContext>& context) const override;

			const gc::Ref<gc::Array<Value>>& get_array() const;

		protected:
			std::size_t get_sys_class_id() const override;

		private:
			gc::Ref<Value>& get_element_ref(std::size_t index);

			ScriptIntegerType api_length(const gc::Local<ExecContext>& context);
			void api_sort(const gc::Local<ExecContext>& context);
		};

		//
		//FunctionValue
		//

		class FunctionValue : public ReferenceValue {
			NONCOPYABLE(FunctionValue);

			gc::Ref<ExecScope> m_scope;
			gc::Ref<ast::FunctionExpression> m_expr;

		public:
			FunctionValue(){}
			void gc_enumerate_refs() override;
			void initialize(const gc::Local<ExecScope>& scope, const gc::Local<ast::FunctionExpression>& expr);

			StringLoc typeof(const gc::Local<ExecContext>& context) const override;

			gc::Local<Value> invoke(
				const gc::Local<ExecContext>& context,
				const gc::Local<gc::Array<Value>>& arguments,
				gc::Local<rt::Value>& exception) override;
		};

		//
		//ClassValue
		//

		class ClassValue : public ReferenceValue {
			NONCOPYABLE(ClassValue);

			gc::Ref<ExecScope> m_scope;
			gc::Ref<ast::ClassExpression> m_expr;

		public:
			ClassValue(){}
			void gc_enumerate_refs() override;
			void initialize(const gc::Local<ExecScope>& scope, const gc::Local<ast::ClassExpression>& expr);

			gc::Local<Value> instantiate(
				const gc::Local<ExecContext>& context,
				const gc::Local<gc::Array<Value>>& arguments,
				gc::Local<rt::Value>& exception) override;

			StringLoc typeof(const gc::Local<ExecContext>& context) const override;
		};

		//
		//ObjectValue
		//

		class ObjectValue : public ReferenceValue {
			NONCOPYABLE(ObjectValue);

			gc::Ref<const ast::ClassExpression> m_expr;
			gc::Ref<ExecScope> m_scope;

		public:
			ObjectValue(){}
			void gc_enumerate_refs() override;

			void initialize(
				const gc::Local<const ast::ClassExpression>& expr,
				const gc::Local<ExecScope>& outer_scope,
				const gc::Local<ScopeDescriptor>& scope_descriptor);

			gc::Local<Value> get_member(
				const gc::Local<ExecContext>& context,
				const gc::Local<ExecScope>& scope,
				const gc::Local<const NameInfo>& name_info) override;

			void set_member(
				const gc::Local<ExecContext>& context,
				const gc::Local<ExecScope>& scope,
				const gc::Local<const NameInfo>& name_info,
				const gc::Local<Value>& value) override;

			StringLoc typeof(const gc::Local<ExecContext>& context) const override;

			gc::Local<ExecScope> get_object_scope();
		};

		//
		//ExceptionValue
		//

		class ExceptionValue : public SysObjectValue {
			NONCOPYABLE(ExceptionValue);

			friend class SysAPI<ExceptionValue>;
			class API;

			gc::Ref<Value> m_value;
			gc::Ref<gc::Array<StackTraceElement>> m_stack_trace;

		public:
			ExceptionValue(){}
			void gc_enumerate_refs() override;

			void initialize(
				const gc::Local<Value>& value,
				const gc::Local<gc::Array<StackTraceElement>>& stack_trace);

			StringLoc to_string(const gc::Local<ExecContext>& context) const override;

			StringLoc typeof(const gc::Local<ExecContext>& context) const override;

			void print_stack_trace(const gc::Local<ExecContext>& context) const;
			void print_stack_trace(const gc::Local<ExecContext>& context, std::ostream& out) const;

		protected:
			std::size_t get_sys_class_id() const override;

		private:
			void api_print_0(const gc::Local<ExecContext>& context);
			void api_print_1(const gc::Local<ExecContext>& context, const gc::Local<TextOutputValue>& out);
		};

	}
}

#endif//SYNSAMPLE_CORE_VALUE_CORE_H_INCLUDED
