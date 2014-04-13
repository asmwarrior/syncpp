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

//Scope-related classes.

#ifndef SYNSAMPLE_CORE_SCOPE_H_INCLUDED
#define SYNSAMPLE_CORE_SCOPE_H_INCLUDED

#include <list>
#include <map>
#include <memory>
#include <vector>

#include "ast__dec.h"
#include "ast_type.h"
#include "common.h"
#include "gc.h"
#include "name.h"
#include "scope__dec.h"
#include "value__dec.h"

namespace syn_script {
	namespace rt {

		enum class DeclarationType {
			VARIABLE,
			CONSTANT,
			FUNCTION,
			CLASS
		};

		enum class StatementResultType {
			NONE,
			BREAK,
			CONTINUE,
			RETURN,
			THROW
		};

		const std::size_t BAD_OFS = SIZE_MAX;
		const std::size_t BAD_ID = SIZE_MAX;

		//-----------------------------------------------------------
		//Value classes.
		//-----------------------------------------------------------

		//
		//ScopeID
		//

		class ScopeID {
			friend class BindContext;

			std::size_t m_id;

			explicit ScopeID(std::size_t id);

		public:
			ScopeID();

			std::size_t get_id() const;
			bool operator==(const ScopeID& id) const;
			bool operator!=(const ScopeID& id) const;
			operator bool() const;
			bool operator!() const;
		};

		//
		//ScopeDescriptor
		//

		typedef gc::PrimitiveArray<ScopeID> ScopeIDArray;

		class ScopeDescriptor : public gc::Object {
			NONCOPYABLE(ScopeDescriptor);

			ScopeID m_id;
			ScopeID m_outer_id;
			std::size_t m_scope_idx;
			std::size_t m_size;
			gc::Ref<ScopeIDArray> m_accessible_scopes;

		public:
			ScopeDescriptor(){}
			void gc_enumerate_refs() override;
			void initialize(
				const ScopeID& id,
				const ScopeID& outer_id,
				std::size_t scope_idx,
				std::size_t size,
				const gc::Local<ScopeIDArray>& accessible_scopes);

			const ScopeID& get_id() const;
			const ScopeID& get_outer_id() const;
			std::size_t get_scope_idx() const;
			std::size_t get_size() const;
			bool is_scope_accessible(const ScopeID& id) const;
		};

		//
		//StatementResult
		//

		class StatementResult {
			StatementResultType m_type;
			gc::Local<Value> m_value;

		public:
			explicit StatementResult(StatementResultType type);
			StatementResult(StatementResultType type, const gc::Local<Value>& value);

			StatementResultType get_type() const;
			gc::Local<Value> get_value() const;

			static StatementResult none();
			static StatementResult exception(const gc::Local<Value>& exception);
		};

		//-----------------------------------------------------------
		//Non-GC classes.
		//-----------------------------------------------------------

		//
		//BindContext
		//

		class BindContext {
			NONCOPYABLE(BindContext);

			class Internal;
			Internal* const m_internal;

			NameTable& m_name_table;
			const ValueFactory& m_value_factory;

		public:
			BindContext(NameTable& name_table, const ValueFactory& value_factory);
			~BindContext();

			NameTable& get_name_table() const;
			const ValueFactory* get_value_factory() const;
			std::unique_ptr<BindScope> create_root_scope();
			ScopeID allocate_scope_id();
		};

		//
		//BindScope
		//

		class BindScope {
			NONCOPYABLE(BindScope);

			BindContext* const m_context;
			BindScope* const m_outer_scope;
			std::size_t const m_scope_ofs;
			ScopeID const m_id;

			std::size_t const m_this_scope_ofs;
			bool const m_loop;

			std::map<NameID, gc::Local<NameDescriptor>> m_name_to_desc;
			std::vector<gc::Local<const NameInfo>> m_idx_to_name;

			bool m_closed;

		private:
			BindScope(
				BindContext* context,
				BindScope* outer_scope,
				std::size_t scope_ofs,
				std::size_t this_scope_ofs,
				bool loop);

		public:
			BindScope(
				BindContext* context,
				BindScope* outer_scope,
				std::size_t scope_ofs,
				std::size_t this_scope_ofs);

			BindScope* get_outer_scope() const;
			const ScopeID& get_id() const;
			std::size_t get_this_scope_ofs() const;
			bool is_loop_control_statement_allowed() const;

			gc::Local<NameDescriptor> lookup(const gc::Local<ast::AstName>& name) const;
			bool contains_name(const NameID& name) const;

			gc::Local<NameDescriptor> declare_variable(const gc::Local<ast::AstName>& name, bool constant);

			gc::Local<NameDescriptor> declare_function(
				const gc::Local<ast::AstName>& name,
				const gc::Local<ast::FunctionDeclaration>& declaration);

			gc::Local<NameDescriptor> declare_class(
				const gc::Local<ast::AstName>& name,
				const gc::Local<ast::ClassDeclaration>& declaration);

			gc::Local<NameDescriptor> declare_sys_constant(const gc::Local<const NameInfo>& name_info);

			std::unique_ptr<BindScope> create_nested_scope(bool nested_this_allowed);
			std::unique_ptr<BindScope> create_nested_block(bool nested_loop);
			gc::Local<ScopeDescriptor> create_scope_descriptor();

		private:
			gc::Local<NameDescriptor> lookup_0(const NameID& name) const;
			bool contains_name_0(const NameID& name) const;
			void check_not_closed() const;
			void check_name_conflict(const gc::Local<ast::AstName>& name) const;

			void check_name_conflict(
				const gc::Local<const NameInfo>& name_info,
				const gc::Local<TextPos>& text_pos) const;

			gc::Local<ScopeIDArray> get_accessible_scopes() const;
		};

		//
		//NameDescriptor
		//

		class NameDescriptor : public gc::Object {
			NONCOPYABLE(NameDescriptor);

			ScopeID m_scope_id;
			std::size_t m_scope_ofs;

		protected:
			NameDescriptor(){}
			void initialize(const ScopeID& scope_id, std::size_t scope_ofs);

		public:
			virtual DeclarationType get_declaration_type() const = 0;
			virtual gc::Local<Value> get(const gc::Local<ExecScope>& scope) const = 0;

			virtual void set_initialize(const gc::Local<ExecScope>& scope, const gc::Local<Value>& value) const;
			virtual void set_modify(const gc::Local<ExecScope>& scope, const gc::Local<Value>& value) const;

		protected:
			const ScopeID& get_scope_id() const;
			std::size_t get_scope_ofs() const;
		};

		//-----------------------------------------------------------
		//GC classes.
		//-----------------------------------------------------------

		//
		//ExecContext
		//

		class ExecContext : public ObjectEx {
			NONCOPYABLE(ExecContext);

			BindContext* m_bind_context;

		public:
			ExecContext(){}
			void initialize(BindContext* bind_context);

			gc::Local<ExecScope> create_root_scope(const gc::Local<ScopeDescriptor>& scope_descriptor);
			BindContext* get_bind_context();
			const ValueFactory* get_value_factory() const;

			gc::Local<Value> get_undefined_value() const;
		};

		//
		//ExecScope
		//

		class ExecScope : public ObjectEx {
			NONCOPYABLE(ExecScope);

			gc::Ref<ExecContext> m_context;
			gc::Ref<ScopeDescriptor> m_descriptor;
			gc::Ref<ExecScope> m_outer_scope;
			std::size_t m_scope_idx;
			gc::Ref<gc::Array<Value>> m_values;
			gc::Ref<Value> m_this_value;

		public:
			ExecScope();
			void gc_enumerate_refs() override;

			void initialize(
				const gc::Local<ExecContext>& context,
				const gc::Local<ScopeDescriptor>& descriptor,
				const gc::Local<ExecScope>& outer_scope,
				const gc::Local<Value>& this_value);

			const ScopeID& get_id() const;
			const gc::Ref<ScopeDescriptor>& get_scope_descriptor() const;
			void check_id(const ScopeID& expected_id);
			void check_idx(std::size_t expected_idx);

			gc::Ref<Value>& get(const ScopeID& scope_id, std::size_t scope_ofs, std::size_t name_ofs);
			gc::Local<Value> get_this(std::size_t scope_ofs);

			gc::Local<ExecScope> create_nested_scope(
				const gc::Local<ScopeDescriptor>& scope_descriptor,
				const gc::Local<Value>& sub_this_value);

			gc::Local<ExecScope> get_target_scope(const ScopeID& scope_id, std::size_t scope_ofs);
		};

	}
}

#endif//SYNSAMPLE_CORE_SCOPE_H_INCLUDED
