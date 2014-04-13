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

//EBNF Grammar Builder implementation.

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "commons.h"
#include "ebnf__imp.h"
#include "ebnf_bld_common.h"
#include "ebnf_builder.h"
#include "ebnf_extension__def.h"
#include "ebnf_visitor__imp.h"
#include "grm_parser_res.h"
#include "types.h"
#include "util.h"
#include "util_string.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

namespace {

	using std::unique_ptr;

	using util::MPtr;
	using util::MHeap;
	using util::MRoot;
	using util::MContainer;
	using util::String;

	typedef std::map<const String, ebnf::TerminalDeclaration*> TrMapType;
	typedef std::map<const String, ebnf::NonterminalDeclaration*> NtMapType;

	template<class K, class T>
	T* ptr_map_get(const std::map<const K, T*>& map, const K& key) {
		typename std::map<const K, T*>::const_iterator it = map.find(key);
		if (it == map.end()) return nullptr;
		return (*it).second;
	}

	template<class K, class T>
	MPtr<T> ptr_map_get(const std::map<const K, MPtr<T>>& map, const K& key) {
		typename std::map<const K, MPtr<T>>::const_iterator it = map.find(key);
		if (it == map.end()) return nullptr;
		return (*it).second;
	}

	template<class T>
	void check_name_duplication_for_map(
		const std::map<const String, T>& map,
		const ns::syntax_string& name_syn,
		const char* symbol_type)
	{
		const String& name = name_syn.get_string();
		if (map.count(name)) {
			throw raise_error(name_syn, "Duplicate name '" + name.str() + "' (" + symbol_type
				+ " with the same name exists)");
		}
	}

}

//
//GrammarBuildingResult
//

ns::GrammarBuildingResult::GrammarBuildingResult(
	GrammarParsingResult* parsing_result,
	unique_ptr<MContainer<const types::Type>> type_container,
	unique_ptr<const std::vector<const types::PrimitiveType*>> primitive_types,
	unique_ptr<const std::vector<PartClassTag>> part_class_tags,
	MPtr<const types::Type> string_literal_type)
	: m_grammar_root(std::move(parsing_result->m_grammar_root)),
	m_primitive_types(std::move(primitive_types)),
	m_part_class_tags(std::move(part_class_tags)),
	m_common_heap(new MHeap()),
	m_string_literal_type(string_literal_type)
{
	m_common_heap->add_heap(std::move(parsing_result->m_const_heap));
	m_common_heap->add_container(std::move(type_container));
}

MPtr<ebnf::Grammar> ns::GrammarBuildingResult::get_grammar() const {
	return m_grammar_root->ptr();
}

MHeap* ns::GrammarBuildingResult::get_common_heap() const {
	return m_common_heap.get();
}

const std::vector<const types::PrimitiveType*>& ns::GrammarBuildingResult::get_primitive_types() const {
	return *m_primitive_types;
}

MPtr<const types::Type> ns::GrammarBuildingResult::get_string_literal_type() const {
	return m_string_literal_type;
}

//
//EBNF_Builder
//

ns::EBNF_Builder::EBNF_Builder(bool verbose_output, const GrammarParsingResult* parsing_result)
	: m_verbose_output(verbose_output),
	m_grammar(parsing_result->get_grammar()),
	m_const_heap_ref(parsing_result->get_const_heap()),
	m_primitive_types(new std::vector<const types::PrimitiveType*>()),
	m_type_container(new MContainer<const types::Type>()),
	m_part_class_tags(new std::vector<PartClassTag>()),
	m_install_extensions_completed(false),
	m_register_names_completed(false),
	m_resolve_name_references_completed(false),
	m_verify_attributes_completed(false),
	m_calculate_is_void_completed(false),
	m_verify_recursion_completed(false),
	m_calculate_general_types_completed(false),
	m_calculate_types_completed(false)
{
	m_void_type = m_type_container->add(new types::VoidType());
	m_const_integer_type = manage_primitive_type(new types::SystemPrimitiveType(String("const_int")));
	m_const_boolean_type = manage_primitive_type(new types::SystemPrimitiveType(String("const_bool")));
	m_const_string_type = manage_primitive_type(new types::SystemPrimitiveType(String("const_str")));

	m_string_literal_type = m_void_type;
	m_string_literal_type_specified = false;
}

void ns::EBNF_Builder::check_name_duplication(const syntax_string& name_syn) const {
	check_name_duplication_for_map(m_nt_map, name_syn, "a nonterminal");
	check_name_duplication_for_map(m_tr_map, name_syn, "a terminal");
}

MPtr<const types::PrimitiveType> ns::EBNF_Builder::register_implicit_primitive_type(const syntax_string& name_syn) {
	const String& name = name_syn.get_string();
	
	MPtr<const types::PrimitiveType> type = ptr_map_get(m_primitive_type_map, name);
	if (type.get()) return type;

	//Type is undefined. Create a new implicit primitive type.
	if (ptr_map_get(m_nt_map, name) || ptr_map_get(m_tr_map, name)) {
		throw raise_error(name_syn, "Name '" + name.str() +
			"' denotes a grammar symbol and cannot be used as a token type");
	}

	MPtr<const types::PrimitiveType> new_type = manage_primitive_type(new types::UserPrimitiveType(name));
	
	m_primitive_type_map[name] = new_type;
	m_type_map[name] = new_type;
	return new_type;
}

MPtr<const types::PrimitiveType> ns::EBNF_Builder::manage_primitive_type(types::PrimitiveType* type) {
	m_primitive_types->push_back(type);
	return m_type_container->add(type);
}

void ns::EBNF_Builder::process_nts(void (ebnf::NonterminalDeclaration::*fn)()) {
	for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) (nt->*fn)();
}

void ns::EBNF_Builder::process_nts(void (ebnf::NonterminalDeclaration::*fn)(EBNF_Builder* builder)) {
	for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) (nt->*fn)(this);
}

void ns::EBNF_Builder::register_nt_declaration(ebnf::NonterminalDeclaration* nt) {
	const syntax_string& name_syn = nt->get_name();
	check_name_duplication(name_syn);
	check_name_duplication_for_map(m_type_map, name_syn, "a type");
	m_nt_map[name_syn.get_string()] = nt;
}

void ns::EBNF_Builder::register_tr_declaration(ebnf::TerminalDeclaration* tr) {
	const syntax_string& name_syn = tr->get_name();
	check_name_duplication(name_syn);
	check_name_duplication_for_map(m_type_map, name_syn, "a type");
	m_tr_map[name_syn.get_string()] = tr;

	if (const ebnf::RawType* raw_type = tr->get_raw_type()) {
		const syntax_string& type_name_syn = raw_type->get_name();
		MPtr<const types::PrimitiveType> type = register_implicit_primitive_type(type_name_syn);
		tr->set_type(type.get());
	}
}

void ns::EBNF_Builder::register_type_declaration(const ebnf::TypeDeclaration* type_decl) {
	const syntax_string& name_syn = type_decl->get_name();
	check_name_duplication(name_syn);
	check_name_duplication_for_map(m_type_decl_map, name_syn, "a type");

	const String& name = name_syn.get_string();
	m_type_decl_map[name] = type_decl;
	if (!ptr_map_get(m_type_map, name).get()) {
		//If the type is not in m_type_map, it cannot be in m_primitive_type_map as well.
		MPtr<const types::PrimitiveType> new_type = manage_primitive_type(new types::UserPrimitiveType(name));
		
		m_type_map[name] = new_type;
		m_primitive_type_map[name] = new_type;
	}
}

void ns::EBNF_Builder::register_custom_terminal_type_declaration(const ebnf::CustomTerminalTypeDeclaration* decl) {
	if (m_string_literal_type_specified) {
		const syntax_string& name = decl->get_raw_type()->get_name();
		throw raise_error(name, "Custom terminal type has already been specified");
	}

	const ebnf::RawType* raw_type = decl->get_raw_type();
	const syntax_string& type_name_syn = raw_type->get_name();
	m_string_literal_type = register_implicit_primitive_type(type_name_syn);
	m_string_literal_type_specified = true;
}

ebnf::SymbolDeclaration* ns::EBNF_Builder::resolve_symbol_name(const syntax_string& name_syn) const {
	const String& name = name_syn.get_string();
	if (ebnf::NonterminalDeclaration* nt = ptr_map_get(m_nt_map, name)) {
		return nt;
	} else if (ebnf::TerminalDeclaration* tr = ptr_map_get(m_tr_map, name)) {
		return tr;
	} else if (ptr_map_get(m_type_map, name).get()) {
		throw raise_error(name_syn, "Name '" + name.str() + "' denotes a type, not a grammar symbol");
	}
	throw raise_error(name_syn, "Name '" + name.str() + "' is undefined");
}

MPtr<const types::ClassType> ns::EBNF_Builder::create_nt_class_type(ebnf::NonterminalDeclaration* nt) {
	NonterminalDeclarationExtension* nt_extension = nt->get_extension();
	MPtr<const types::ClassType> type = nt_extension->get_class_type_opt();
	if (!type.get()) {
		type = m_type_container->add(new types::NonterminalClassType(nt));
		nt_extension->set_class_type(type);
	}
	return type;
}

MPtr<const types::Type> ns::EBNF_Builder::resolve_type_name(const syntax_string& name_syn) {
	const String& name = name_syn.get_string();

	MPtr<const types::Type> existing_type = ptr_map_get(m_type_map, name);
	if (existing_type.get()) return existing_type;

	if (ebnf::NonterminalDeclaration* nt = ptr_map_get(m_nt_map, name)) {
		//Create an implicit nonterminal class type.
		MPtr<const types::Type> type = create_nt_class_type(nt);
		m_type_map[name] = type;
		return type;
	} else if (ptr_map_get(m_tr_map, name)) {
		throw raise_error(name_syn, "Name '" + name.str() + "' denotes a token and cannot be used as a type");
	}

	//Create an implicit class type.
	MPtr<types::Type> new_type = m_type_container->add(new types::NameClassType(name));
	m_type_map[name] = new_type;
	return new_type;
}

void ns::EBNF_Builder::install_extensions() {
	assert(!m_install_extensions_completed);

	class ExprExtensionInstallingVisitor : public ns::SyntaxExpressionVisitor<void> {
	public:
		ExprExtensionInstallingVisitor(){}
		
		void visit_SyntaxExpression(ebnf::SyntaxExpression* expr) override {
			expr->install_extension(make_unique1<SyntaxExpressionExtension>());
		}

		void visit_SyntaxAndExpression(ebnf::SyntaxAndExpression* expr) override {
			SyntaxExpressionVisitor<void>::visit_SyntaxAndExpression(expr);
			expr->install_and_extension(make_unique1<SyntaxAndExpressionExtension>());
		}
	};

	for (ebnf::NonterminalDeclaration* nt : m_grammar->get_nonterminals()) {
		nt->install_extension(make_unique1<NonterminalDeclarationExtension>());

		ExprExtensionInstallingVisitor expr_visitor;
		RecursiveSyntaxExpressionVisitor recursive_expr_visitor(&expr_visitor);
		nt->get_expression()->visit(&recursive_expr_visitor);
	}

	m_install_extensions_completed = true;
}

MPtr<const types::Type> ns::EBNF_Builder::manage_type(types::Type* type) {
	return m_type_container->add(type);
}

unique_ptr<ns::GrammarBuildingResult> ns::EBNF_Builder::build0(GrammarParsingResult* parsing_result) {
	install_extensions();
	register_names();
	resolve_name_references();
	verify_attributes();
	calculate_is_void();
	verify_recursion();
	calculate_general_types();
	calculate_types();

	return make_unique1<GrammarBuildingResult>(
		parsing_result,
		std::move(m_type_container),
		util::const_vector_ptr(std::move(m_primitive_types)),
		util::const_vector_ptr(std::move(m_part_class_tags)),
		m_string_literal_type);
}

unique_ptr<ns::GrammarBuildingResult> ns::EBNF_Builder::build(
	bool verbose_output,
	unique_ptr<GrammarParsingResult> parsing_result)
{
	EBNF_Builder builder(verbose_output, parsing_result.get());
	return builder.build0(parsing_result.get());
}
