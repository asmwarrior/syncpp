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

//Semantic actions code generator.

#ifndef SYN_CORE_CODEGEN_ACTION_H_INCLUDED
#define SYN_CORE_CODEGEN_ACTION_H_INCLUDED

#include <ostream>
#include <string>
#include <vector>

#include "action.h"
#include "cmdline.h"
#include "concrete_lr.h"
#include "concretelrgen_res.h"
#include "ebnf__dec.h"
#include "descriptor_type.h"
#include "noncopyable.h"
#include "util.h"
#include "util_mptr.h"
#include "util_string.h"

namespace synbin {

	//
	//ActionInfo
	//

	struct ActionInfo {
		const ConcreteLRPr* m_pr;
		std::size_t m_action_index;
		std::size_t m_nt_local_index;

		ActionInfo(){}

		ActionInfo(const ConcreteLRPr* pr, std::size_t action_index, std::size_t nt_local_index)
			: m_pr(pr), m_action_index(action_index), m_nt_local_index(nt_local_index)
		{}
	};

	//
	//PrIndexFn
	//

	struct PrIndexFn {
		std::size_t operator()(const ConcreteLRPr* pr) const {
			return pr->get_pr_index();
		}
	};

	//
	//ActionCodeGenerator
	//

	//Action code generator class.
	class ActionCodeGenerator {
		NONCOPYABLE(ActionCodeGenerator);

		const CommandLine& m_command_line;
		const std::string& m_type_namespace;
		const std::string& m_class_namespace;
		const std::string& m_code_namespace;
		const std::string& m_native_namespace;
		const std::size_t m_class_type_count;
		const ConcreteLRResult* const m_lr_result;

		typedef util::IndexedMap<const ConcreteLRPr*, ActionInfo, PrIndexFn> PrActionMap;
		PrActionMap m_pr_action_map;
		
		std::vector<ActionInfo> m_action_vector;
		std::vector<const PrimitiveTypeDescriptor*> m_used_primitive_types;
		const char* m_indent;

	public:
		ActionCodeGenerator(
			const CommandLine& command_line,
			const std::string& type_namespace,
			const std::string& class_namespace,
			const std::string& code_namespace,
			const std::string& native_namespace,
			const ConcreteLRResult* lr_result);
		
		const ActionInfo& get_action_info(const ConcreteLRPr* pr) const;

		void generate_action_declarations(std::ostream& out);
		void generate_actions(std::ostream& out);

		void generate_primitive_type(std::ostream& out, const PrimitiveTypeDescriptor* type) const;
		void generate_value_pool_member_name(std::ostream& out, const types::PrimitiveType* type) const;
		void generate_value_pool_allocator_name(std::ostream& out, const types::PrimitiveType* type) const;

		void generate_type_external(std::ostream& out, util::MPtr<const TypeDescriptor> type);
		bool generate_internal_to_external_conversion(std::ostream& out, util::MPtr<const TypeDescriptor> type);
		void generate_internal_to_external_conversion__list(std::ostream& out, const ListTypeDescriptor* type);

		void generate_nonterminal_function_name(std::ostream& out, const ConcreteLRNt* nt);
		void generate_primitive_type_function_name(std::ostream& out, const PrimitiveTypeDescriptor* type);
		void generate_nt_function_type(std::ostream& out, util::MPtr<const TypeDescriptor> type);
		void generate_nt_function_parameters(std::ostream& out, util::MPtr<const TypeDescriptor> type);

		void generate_production_constant_name(std::ostream& out, const ActionInfo& action_info) const;
		void generate_class_name(std::ostream& out, const util::String& class_name, const std::string& name_space) const;

	private:
		void generate_action_code_nt(std::ostream& out, const ConcreteLRNt* nt);
		void generate_action_code_pr(std::ostream& out, const ConcreteLRPr* pr);
		
		void generate_action_code__void(std::ostream& out, const ConcreteBNF::Pr* pr, const VoidAction* action);
		void generate_action_code__copy(std::ostream& out, const ConcreteBNF::Pr* pr, const CopyAction* action);
		void generate_action_code__cast(std::ostream& out, const ConcreteBNF::Pr* pr, const CastAction* action);
		void generate_action_code__class(std::ostream& out, const ConcreteBNF::Pr* pr, const ClassAction* action);
		void generate_action_code__part_class(std::ostream& out, const ConcreteBNF::Pr* pr, const PartClassAction* action);
		void generate_action_code__result_and(std::ostream& out, const ConcreteBNF::Pr* pr, const ResultAndAction* action);
		void generate_action_code__result_and0(std::ostream& out, const ConcreteBNF::Pr* pr, std::size_t ofs);
		void generate_action_code__first_list(std::ostream& out, const ConcreteBNF::Pr* pr, const FirstListAction* action);
		void generate_action_code__next_list(std::ostream& out, const ConcreteBNF::Pr* pr, const NextListAction* action);
		void generate_action_code__const(std::ostream& out, const ConcreteBNF::Pr* pr, const ConstAction* action);

		void generate_action_code__abstract_class(
			std::ostream& out,
			const ConcreteBNF::Pr* pr,
			const AbstractClassAction* action);

		void generate_type_external__list(std::ostream& out, const ListTypeDescriptor* type);
		void generate_type_external__list_no_ptr(std::ostream& out, const ListTypeDescriptor* type);
		void generate_type_internal(std::ostream& out, util::MPtr<const TypeDescriptor> type);
		void generate_type_internal__list(std::ostream& out, const ListTypeDescriptor* type);
		void generate_type__class(std::ostream& out, const ClassTypeDescriptor* type);
		void generate_type__class_no_ptr(std::ostream& out, const ClassTypeDescriptor* type);
		void generate_type__primitive(std::ostream& out, const PrimitiveTypeDescriptor* type);

		void generate_get_stack(std::ostream& out, const ConcreteBNF::Pr* pr, std::size_t ofs);
		void generate_get_stack_attribute(std::ostream& out, const ConcreteBNF::Pr* pr, std::size_t ofs);

		void generate_const_expression(std::ostream& out, const ebnf::ConstExpression* expr);
		void generate_const_expr__string(std::ostream& out, const ebnf::StringConstExpression* expr);
		void generate_const_expr__native(std::ostream& out, const ebnf::NativeConstExpression* expr);
		void generate_const_expr__native_name(std::ostream& out, const ebnf::NativeName* name);
		void generate_const_expr__native_name__function(std::ostream& out, const ebnf::NativeFunctionName* function);

		void fmt_attr_set_begin(std::ostream& out, const util::String& name);
		void fmt_attr_set_end(std::ostream& out);
	};
}

#endif//SYN_CORE_CODEGEN_ACTION_H_INCLUDED
