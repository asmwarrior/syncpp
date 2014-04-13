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

//EBNF Builder definition.

#ifndef SYN_CORE_EBNF_BUILDER_H_INCLUDED
#define SYN_CORE_EBNF_BUILDER_H_INCLUDED

#include <functional>

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ebnf__imp.h"
#include "ebnf_builder_res.h"
#include "ebnf_extension__def.h"
#include "noncopyable.h"
#include "types.h"
#include "util.h"
#include "util_mptr.h"

namespace synbin {

	class EBNF_Builder;
	class EBNF_BuilderTestGate;

	//
	//EBNF_Builder
	//

	//A class responsible for EBNF grammar validation and processing. Calculates properties of grammar elements,
	//like types of nonterminals and expressions.
	class EBNF_Builder {
		NONCOPYABLE(EBNF_Builder);

		friend class EBNF_BuilderTestGate;
		
		const bool m_verbose_output;

		util::MPtr<ebnf::Grammar> m_grammar;
		util::MHeap* const m_const_heap_ref;

		std::unique_ptr<std::vector<const types::PrimitiveType*>> m_primitive_types;
		std::unique_ptr<util::MContainer<const types::Type>> m_type_container;
		std::unique_ptr<std::vector<PartClassTag>> m_part_class_tags;

		std::map<const util::String, ebnf::NonterminalDeclaration*> m_nt_map;
		std::map<const util::String, ebnf::TerminalDeclaration*> m_tr_map;
		std::map<const util::String, util::MPtr<const types::Type>> m_type_map;
		std::map<const util::String, util::MPtr<const types::PrimitiveType>> m_primitive_type_map;
		
		//Map of type declarations. Used for duplicated declarations check only.
		std::map<const util::String, const ebnf::TypeDeclaration*> m_type_decl_map;

		util::MPtr<const types::Type> m_void_type;
		util::MPtr<const types::Type> m_string_literal_type;
		bool m_string_literal_type_specified;
		util::MPtr<const types::PrimitiveType> m_const_integer_type;
		util::MPtr<const types::PrimitiveType> m_const_boolean_type;
		util::MPtr<const types::PrimitiveType> m_const_string_type;

		explicit EBNF_Builder(bool verbose_output, const GrammarParsingResult* parsing_result);

		void check_name_duplication(const syntax_string& name_syn) const;
		util::MPtr<const types::PrimitiveType> register_implicit_primitive_type(const syntax_string& name_syn);
		util::MPtr<const types::PrimitiveType> manage_primitive_type(types::PrimitiveType* type);

		void process_nts(void (ebnf::NonterminalDeclaration::*fn)());
		void process_nts(void (ebnf::NonterminalDeclaration::*fn)(EBNF_Builder* builder));

		//Build step flags.
		bool m_install_extensions_completed;
		bool m_register_names_completed;
		bool m_resolve_name_references_completed;
		bool m_verify_attributes_completed;
		bool m_calculate_is_void_completed;
		bool m_verify_recursion_completed;
		bool m_calculate_general_types_completed;
		bool m_calculate_types_completed;

		//Build step functions.
		void install_extensions();
		void register_names();
		void resolve_name_references();
		void verify_attributes();
		void calculate_is_void();
		void verify_recursion();
		void calculate_general_types();
		void calculate_types();

		std::unique_ptr<GrammarBuildingResult> build0(GrammarParsingResult* parsing_result);

	public:		
		void register_nt_declaration(ebnf::NonterminalDeclaration* nt);
		void register_tr_declaration(ebnf::TerminalDeclaration* tr);
		void register_type_declaration(const ebnf::TypeDeclaration* type);
		void register_custom_terminal_type_declaration(const ebnf::CustomTerminalTypeDeclaration* decl);
		ebnf::SymbolDeclaration* resolve_symbol_name(const syntax_string& name_syn) const;
		util::MPtr<const types::ClassType> create_nt_class_type(ebnf::NonterminalDeclaration* nt);
		util::MPtr<const types::Type> resolve_type_name(const syntax_string& name_syn);

		util::MPtr<const types::Type> get_void_type() const { return m_void_type; }
		util::MPtr<const types::Type> get_string_literal_type() const { return m_string_literal_type; }
		util::MPtr<const types::PrimitiveType> get_const_integer_type() const { return m_const_integer_type; }
		util::MPtr<const types::PrimitiveType> get_const_boolean_type() const { return m_const_boolean_type; }
		util::MPtr<const types::PrimitiveType> get_const_string_type() const { return m_const_string_type; }

		util::MPtr<const types::Type> manage_type(types::Type* type);

		static std::unique_ptr<GrammarBuildingResult> build(
			bool verbose_output,
			std::unique_ptr<GrammarParsingResult> parsing_result);
	};

	//
	//EBNF_BuilderTestGate
	//

	//This class is used in unit tests to access build step functions of EBNF_Builder. This is needed for
	//testing individual steps instead of the entire build.
	class EBNF_BuilderTestGate {
		NONCOPYABLE(EBNF_BuilderTestGate);

		EBNF_Builder m_builder;

	public:
		EBNF_BuilderTestGate(GrammarParsingResult* parsing_result) : m_builder(false, parsing_result){}

		void install_extensions() { m_builder.install_extensions(); }
		void register_names() { m_builder.register_names(); }
		void resolve_name_references() { m_builder.resolve_name_references(); }
		void verify_attributes() { m_builder.verify_attributes(); }
		void calculate_is_void() { m_builder.calculate_is_void(); }
		void verify_recursion() { m_builder.verify_recursion(); }
		void calculate_general_types() { m_builder.calculate_general_types(); }
		void calculate_types() { m_builder.calculate_types(); }

		const EBNF_Builder& get_builder() const { return m_builder; }
	};

}

#endif//SYN_CORE_EBNF_BUILDER_H_INCLUDED
