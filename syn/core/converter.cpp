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

//Implementation of EBNF-to-BNF converter.

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "action.h"
#include "action_factory.h"
#include "bnf.h"
#include "commons.h"
#include "conversion.h"
#include "concrete_bnf.h"
#include "converter.h"
#include "descriptor.h"
#include "descriptor_type.h"
#include "ebnf__imp.h"
#include "ebnf_builder_res.h"
#include "ebnf_extension__imp.h"
#include "ebnf_visitor__imp.h"
#include "types.h"
#include "util.h"
#include "util_mptr.h"
#include "util_string.h"

namespace ns = synbin;
namespace conv = ns::conv;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

using std::unique_ptr;

using util::MContainer;
using util::MRoot;
using util::MHeap;
using util::MPtr;

using util::String;

///////////////////////////////////////////////////////////////////////////////////////////////////
//BNF Traits, etc.
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

	typedef MPtr<const ns::Action> ActionMPtr;

	typedef ns::ConcreteBNF BnfGrm;
	typedef BnfGrm::Sym BnfSym;
	typedef BnfGrm::Nt BnfNt;
	typedef BnfGrm::Tr BnfTr;
	typedef BnfGrm::Pr BnfPr;

	typedef ns::BnfGrammarBuilder<ns::ConcreteBNFTraits> BnfBld;

}//namespace

//
//ConvSym
//

//Instead of using BnfSym and its subclasses, wrappers like ConvSym are used - in order to hide
//the dependency from BNF.
class conv::ConvSym {
	NONCOPYABLE(ConvSym);

	const BnfSym* const m_bnf_sym;

protected:
	ConvSym(const BnfSym* bnf_sym)
		: m_bnf_sym(bnf_sym)
	{
		assert(bnf_sym);
	}

public:
	virtual ~ConvSym(){}

	const BnfSym* get_sym() const {
		return m_bnf_sym;
	}

	virtual MPtr<const SymDescriptor> get_descriptor() const = 0;
};

//
//ConvNt
//

class conv::ConvNt : public ConvSym {
	NONCOPYABLE(ConvNt);

public:
	ConvNt(const BnfNt* bnf_nt) : ConvSym(bnf_nt){}

	const BnfNt* get_nt() const {
		return static_cast<const BnfNt*>(get_sym());
	}

	MPtr<const SymDescriptor> get_descriptor() const override {
		return get_nt()->get_nt_obj();
	}
};

//
//ConvTr
//

class conv::ConvTr : public ConvSym {
	NONCOPYABLE(ConvTr);

public:
	ConvTr(const BnfTr* bnf_tr) : ConvSym(bnf_tr){}

	const BnfTr* get_tr() const {
		return static_cast<const BnfTr*>(get_sym());
	}

	MPtr<const SymDescriptor> get_descriptor() const override {
		return get_tr()->get_tr_obj();
	}
};

//
//ConvPr
//

class conv::ConvPr {
	NONCOPYABLE(ConvPr);

	const BnfPr* const m_bnf_pr;
	const ActionMPtr m_action;

public:
	ConvPr(const BnfPr* bnf_pr) : m_bnf_pr(bnf_pr){}

	const BnfPr* get_pr() const {
		assert(m_bnf_pr);
		return m_bnf_pr;
	}
};

//
//ConvPrBuilder : definition
//

namespace synbin {
	class ConvPrBuilder;
}

class ns::ConvPrBuilder : public ns::ConvPrBuilderFacade {
	NONCOPYABLE(ConvPrBuilder);

	ns::Converter* const m_converter;
	std::vector<const BnfSym*> m_elements;
	ActionMPtr m_action;

public:
	ConvPrBuilder(ns::Converter* converter);

	void add_element(util::MPtr<conv::ConvSym> sym) override;
	void set_action_factory(ns::ActionFactory& action_factory) override;

	const std::vector<const BnfSym*>& get_elements() const;
	ActionMPtr get_action() const;
};

//
//(Misc.)
//

namespace {

	const std::string g_auto_nt_name_prefix = "A_";
	const std::string g_user_nt_name_prefix = "N_";
	const std::string g_str_tr_name_prefix = "S_";
	const std::string g_name_tr_name_prefix = "T_";

	const String g_auto_empty_nt_name(g_auto_nt_name_prefix + "Empty");

	struct TrDeclIndexFn {
		std::size_t operator()(const ebnf::TerminalDeclaration* tr) const {
			return tr->tr_index();
		}
	};

	struct NtDeclIndexFn {
		std::size_t operator()(const ebnf::NonterminalDeclaration* nt) const {
			return nt->nt_index();
		}
	};

}

//
//ConversionResult
//

ns::ConversionResult::ConversionResult(
	GrammarBuildingResult* building_result,
	unique_ptr<const ConcreteBNF> bnf_grammar,
	unique_ptr<const std::vector<const ConcreteBNF::Nt*>> start_nts,
	unique_ptr<const std::vector<const NtDescriptor*>> nts,
	unique_ptr<const std::vector<const NameTrDescriptor*>> name_tokens,
	unique_ptr<const std::vector<const StrTrDescriptor*>> str_tokens,
	unique_ptr<const std::vector<const PrimitiveTypeDescriptor*>> primitive_types,
	util::MPtr<const PrimitiveTypeDescriptor> string_literal_type,
	std::size_t class_type_count)
	: m_common_heap(std::move(building_result->m_common_heap)),
	m_bnf_grammar(std::move(bnf_grammar)),
	m_start_nts(std::move(start_nts)),
	m_nts(std::move(nts)),
	m_name_tokens(std::move(name_tokens)),
	m_str_tokens(std::move(str_tokens)),
	m_primitive_types(std::move(primitive_types)),
	m_string_literal_type(string_literal_type),
	m_class_type_count(class_type_count)
{}

const ns::ConcreteBNF* ns::ConversionResult::get_bnf_grammar() const {
	return m_bnf_grammar.get();
}

const std::vector<const ns::ConcreteBNF::Nt*>& ns::ConversionResult::get_start_nts() const {
	return *m_start_nts;
}

const std::vector<const ns::NameTrDescriptor*>& ns::ConversionResult::get_name_tokens() const {
	return *m_name_tokens;
}

const std::vector<const ns::StrTrDescriptor*>& ns::ConversionResult::get_str_tokens() const {
	return *m_str_tokens;
}

const std::vector<const ns::PrimitiveTypeDescriptor*>& ns::ConversionResult::get_primitive_types() const {
	return *m_primitive_types;
}

//
//Converter : definition
//

namespace {
	typedef std::map<String, MPtr<const ns::PrimitiveTypeDescriptor>> PrimitiveTypeMap;
}

class ns::Converter : public ConverterFacade {
	NONCOPYABLE(Converter);

	unique_ptr<MHeap> m_managed_heap;

	BnfBld m_bnf_builder;

	std::size_t m_auto_nt_name_index;
	MPtr<conv::ConvNt> m_empty_conv_nt;

	PrimitiveTypeMap m_system_primitive_type_map;
	PrimitiveTypeMap m_user_primitive_type_map;

	std::map<String, MPtr<conv::ConvTr>> m_str_tr_to_bnf_map;
	util::IndexedMap<const ebnf::TerminalDeclaration*, MPtr<conv::ConvTr>, TrDeclIndexFn> m_tr_to_bnf_map;
	util::IndexedMap<const ebnf::NonterminalDeclaration*, MPtr<conv::ConvNt>, NtDeclIndexFn> m_nt_to_conv_map;

	std::map<String, MPtr<const ClassTypeDescriptor>> m_class_type_map;

	const types::Type* const m_string_literal_type;

	MPtr<const VoidTypeDescriptor> m_void_type;
	MPtr<const TypeDescriptor> m_string_literal_type_desc;

	unique_ptr<std::vector<const ConcreteBNF::Nt*>> m_start_bnf_nts;
	unique_ptr<std::vector<const NtDescriptor*>> m_nts;

	unique_ptr<std::vector<const NameTrDescriptor*>> m_name_tokens;
	unique_ptr<std::vector<const StrTrDescriptor*>> m_str_tokens;
	
	const unique_ptr<MContainer<conv::ConvSym>> m_managed_conv_syms;
	
	const MPtr<MContainer<Action>> m_managed_actions;
	const MPtr<MContainer<SymDescriptor>> m_managed_sym_descriptors;
	const MPtr<MContainer<PrDescriptor>> m_managed_pr_descriptors;
	const MPtr<MContainer<TypeDescriptor>> m_managed_types;

public:
	Converter(
		unique_ptr<MHeap> managed_heap,
		MHeap* managed_heap_ref,
		const types::Type* string_literal_type,
		std::size_t tr_count,
		std::size_t nt_count)
		: m_managed_heap(std::move(managed_heap)),
		m_string_literal_type(string_literal_type),
		m_auto_nt_name_index(0),
		m_tr_to_bnf_map(tr_count),
		m_nt_to_conv_map(nt_count),
		m_start_bnf_nts(new std::vector<const ns::ConcreteBNF::Nt*>()),
		m_nts(new std::vector<const ns::NtDescriptor*>()),
		m_name_tokens(new std::vector<const NameTrDescriptor*>()),
		m_str_tokens(new std::vector<const StrTrDescriptor*>()),
		m_managed_conv_syms(new MContainer<conv::ConvSym>()),
		m_managed_actions(managed_heap_ref->create_container<Action>()),
		m_managed_sym_descriptors(managed_heap_ref->create_container<SymDescriptor>()),
		m_managed_pr_descriptors(managed_heap_ref->create_container<PrDescriptor>()),
		m_managed_types(managed_heap_ref->create_container<TypeDescriptor>())
	{}

	MPtr<conv::ConvNt> get_empty_nonterminal() override;
	MPtr<conv::ConvSym> cast_nt_to_sym(MPtr<conv::ConvNt> conv_nt) const override;

	MPtr<conv::ConvSym> convert_expression_to_nonterminal(ebnf::SyntaxExpression* expr, MPtr<const TypeDescriptor> type) override;
	MPtr<conv::ConvNt> convert_nonterminal(ebnf::NonterminalDeclaration* nt);
	void convert_terminal_init(ebnf::TerminalDeclaration* tr);
	MPtr<conv::ConvTr> convert_terminal(ebnf::TerminalDeclaration* tr);
	void convert_expression_to_production(MPtr<conv::ConvNt> conv_nt, const ebnf::SyntaxExpression* expr) override;
	MPtr<conv::ConvSym> convert_symbol_to_symbol(ebnf::SymbolDeclaration* sym) override;
	MPtr<conv::ConvSym> convert_expression_to_symbol(ebnf::SyntaxExpression* expr) override;
	MPtr<conv::ConvSym> convert_string_to_symbol(const syntax_string& str) override;

	MPtr<conv::ConvNt> create_auto_nonterminal(MPtr<const TypeDescriptor> type) override;

	void create_production(
		MPtr<conv::ConvNt> conv_nt,
		const std::vector<MPtr<conv::ConvSym>>& conv_elements,
		ActionFactory& action_factory) override;

	MPtr<const TypeDescriptor> convert_type(const types::Type* type) override;
	MPtr<const ListTypeDescriptor> create_list_type(MPtr<const TypeDescriptor> element_type) override;
	void convert_primitive_type_init(const types::PrimitiveType* primitive_type);
	MPtr<const PrimitiveTypeDescriptor> convert_primitive_type(const types::PrimitiveType* primitive_type) override;
	MPtr<const PrimitiveTypeDescriptor> convert_string_literal_type(const types::Type* type);
	MPtr<const ClassTypeDescriptor> convert_class_type(const types::ClassType* class_type) override;
	
	MPtr<const PartClassTypeDescriptor> convert_part_class_type(
		MPtr<const ClassTypeDescriptor> class_type,
		const PartClassTag& part_class_tag) override;

	MPtr<Action> manage_action(std::unique_ptr<Action> action) override;
	MPtr<const TypeDescriptor> manage_type(TypeDescriptor* type) override;

	MPtr<const VoidTypeDescriptor> get_void_type() override;
	MPtr<const TypeDescriptor> get_symbol_type(MPtr<conv::ConvSym> conv_sym) const override;

	ActionMPtr create_action(ActionFactory& action_factory, const std::vector<const BnfSym*>& elements);

	unique_ptr<ConversionResult> convert_EBNF(bool verbose_output, GrammarBuildingResult* building_result);

private:
	MPtr<conv::ConvNt> create_nonterminal(const String& name, MPtr<const NtDescriptor> descriptor);

	void create_production0(
		MPtr<conv::ConvNt> conv_nt,
		const std::vector<const BnfSym*>& elements,
		MPtr<const Action> action);

	void create_implicitly_casted_production(
		MPtr<conv::ConvNt> conv_nt,
		const std::vector<const BnfSym*>& elements,
		MPtr<const Action> action,
		MPtr<const TypeDescriptor> nt_type,
		MPtr<const TypeDescriptor> pr_type);

	MPtr<const Action> create_implicit_cast_action(
		MPtr<const TypeDescriptor> cast_type,
		MPtr<const TypeDescriptor> source_type);

	MPtr<const NtDescriptor> manage_nt(NtDescriptor* nt);
	
	String generate_auto_nt_name();

	MPtr<const TypeDescriptor> get_string_literal_type_desc();
};

//
//ConvPrBuilder : implementation
//

ns::ConvPrBuilder::ConvPrBuilder(ns::Converter* converter)
	: m_converter(converter)
{}

void ns::ConvPrBuilder::add_element(MPtr<conv::ConvSym> conv_sym) {
	assert(conv_sym.get());
	assert(!m_action.get());
	m_elements.push_back(conv_sym->get_sym());
}

void ns::ConvPrBuilder::set_action_factory(ns::ActionFactory& action_factory) {
	assert(!m_action);
	m_action = m_converter->create_action(action_factory, m_elements);
	assert(!!m_action);
}

const std::vector<const BnfSym*>& ns::ConvPrBuilder::get_elements() const {
	return m_elements;
}

ActionMPtr (ns::ConvPrBuilder::get_action)() const {
	assert(m_action.get());
	return m_action;
}

//
//Converter : implementation
//

MPtr<conv::ConvNt> ns::Converter::get_empty_nonterminal() {
	if (!m_empty_conv_nt.get()) {
		MPtr<const VoidTypeDescriptor> void_type = get_void_type();

		MPtr<const NtDescriptor> descriptor = manage_nt(new AutoNtDescriptor(void_type, g_auto_empty_nt_name));
		m_empty_conv_nt = create_nonterminal(g_auto_empty_nt_name, descriptor);
		
		ActionMPtr action = m_managed_actions->add(new VoidAction(void_type));
		std::vector<const BnfSym*> elements;
		create_production0(m_empty_conv_nt, elements, action);
	}
	return m_empty_conv_nt;
}

MPtr<conv::ConvSym> ns::Converter::cast_nt_to_sym(MPtr<conv::ConvNt> conv_nt) const {
	return conv_nt;
}

MPtr<conv::ConvSym> ns::Converter::convert_expression_to_nonterminal(
	ebnf::SyntaxExpression* expr,
	MPtr<const TypeDescriptor> type)
{
	assert(!!type);

	//Convert.
	String nt_name = generate_auto_nt_name();
	MPtr<const NtDescriptor> descriptor = manage_nt(new AutoNtDescriptor(type, nt_name));
	MPtr<conv::ConvNt> conv_nt = create_nonterminal(nt_name, descriptor);
	expr->get_extension()->get_conversion()->convert_nt(this, conv_nt);
	return conv_nt;
}

void ns::Converter::convert_expression_to_production(MPtr<conv::ConvNt> conv_nt, const ebnf::SyntaxExpression* expr) {
	ConvPrBuilder pr_builder(this);
	expr->get_extension()->get_conversion()->convert_pr(this, &pr_builder);

	ActionMPtr action = pr_builder.get_action();
	const std::vector<const BnfSym*>& elements = pr_builder.get_elements();
	create_production0(conv_nt, elements, action);
}

MPtr<conv::ConvSym> ns::Converter::convert_symbol_to_symbol(ebnf::SymbolDeclaration* sym) {
	class SymConvertingVisitor : public ns::SymbolDeclarationVisitor<MPtr<conv::ConvSym>> {
		Converter* const m_converter;
	public:
		SymConvertingVisitor(Converter* converter) : m_converter(converter){}

		MPtr<conv::ConvSym> visit_SymbolDeclaration(ebnf::SymbolDeclaration* sym) override {
			throw err_illegal_state();
		}
			
		MPtr<conv::ConvSym> visit_TerminalDeclaration(ebnf::TerminalDeclaration* tr) override {
			MPtr<conv::ConvTr> conv_tr = m_converter->convert_terminal(tr);
			return conv_tr;
		}
			
		MPtr<conv::ConvSym> visit_NonterminalDeclaration(ebnf::NonterminalDeclaration* nt) override {
			MPtr<conv::ConvNt> conv_nt = m_converter->convert_nonterminal(nt);
			return conv_nt;
		}
	};

	SymConvertingVisitor visitor(this);
	MPtr<conv::ConvSym> conv_sym = sym->visit(&visitor);
	return conv_sym;
}

MPtr<conv::ConvSym> ns::Converter::convert_expression_to_symbol(ebnf::SyntaxExpression* expr) {
	ns::SyntaxExpressionExtension* ext = expr->get_extension();
	MPtr<conv::ConvSym> conv_sym = ext->get_conversion()->convert_sym(this);
	return conv_sym;
}

namespace {
	bool is_str_name_start(int k) {
		return 0 != std::isalpha(k) || '_' == k;
	}

	bool is_str_name_part(int k) {
		return 0 != std::isalnum(k) || '_' == k;
	}

	bool is_str_name(const ns::syntax_string& str) {
		const std::string& s = str.str();
		assert(!s.empty());
		bool is_name = is_str_name_start(s[0]);
		for (std::size_t i = 1, n = s.size(); i < n; ++i) {
			if (is_name != is_str_name_part(s[i])) {
				throw ns::raise_error(str, "Mixing identifier and non-identifier in a string literal");
			}
		}
		return is_name;
	}
}//namespace

MPtr<conv::ConvSym> ns::Converter::convert_string_to_symbol(const syntax_string& synstr) {
	const String& str = synstr.get_string();
	MPtr<conv::ConvTr> conv_tr = m_str_tr_to_bnf_map[str];
	if (!conv_tr.get()) {
		std::size_t idx = m_str_tr_to_bnf_map.size() - 1; //The size has just been incremented by the operator[].
		const String name(g_name_tr_name_prefix + std::to_string(idx));
		
		MPtr<const TypeDescriptor> type = get_string_literal_type_desc();

		std::size_t token_id = m_str_tokens->size();
		bool is_name = is_str_name(synstr);//TODO Determine is_name (and report mixing error) earlier.
		MPtr<const StrTrDescriptor> descriptor = m_managed_sym_descriptors->add(new StrTrDescriptor(type, str, token_id, is_name));
		m_str_tokens->push_back(descriptor.get());
		
		const BnfTr* bnf_tr = m_bnf_builder.create_terminal(name, descriptor);
		conv_tr = m_managed_conv_syms->add(new conv::ConvTr(bnf_tr));
		m_str_tr_to_bnf_map[str] = conv_tr;
	}
	return conv_tr;
}

MPtr<conv::ConvNt> ns::Converter::convert_nonterminal(ebnf::NonterminalDeclaration* nt) {
	MPtr<conv::ConvNt> conv_nt = m_nt_to_conv_map.get(nt);
	if (!conv_nt.get()) {
		//Determine the type.
		MPtr<const types::Type> concrete_type = nt->get_extension()->get_concrete_type();
		assert(!!concrete_type);
		MPtr<const TypeDescriptor> type = convert_type(concrete_type.get());

		//Create BNF nonterminal.
		const String& original_name = nt->get_name().get_string();
		String name = String(g_user_nt_name_prefix + original_name.str());
		MPtr<const NtDescriptor> descriptor = manage_nt(new UserNtDescriptor(type, name, original_name));
		conv_nt = create_nonterminal(name, descriptor);

		//The new nonterminal must be put into the map before the conversion of the expression, to
		//avoid infinite recursion.
		m_nt_to_conv_map.put(nt, conv_nt);

		//Convert the expression.
		ebnf::SyntaxExpression* expr = nt->get_expression();
		ns::SyntaxExpressionExtension* expr_ext = expr->get_extension();
		expr_ext->get_conversion()->convert_nt(this, conv_nt);

		if (nt->is_start()) m_start_bnf_nts->push_back(conv_nt->get_nt());
	}
	return conv_nt;
}

void ns::Converter::convert_terminal_init(ebnf::TerminalDeclaration* tr) {
	MPtr<conv::ConvTr> conv_tr = m_tr_to_bnf_map.get(tr);
	assert(!conv_tr);
		
	const util::String& original_name = tr->get_name().get_string();

	//Define type.
	MPtr<const TypeDescriptor> type;
	const types::PrimitiveType* concrete_type0 = tr->get_type();
	if (concrete_type0) {
		MPtr<const types::Type> concrete_type = MPtr<const types::Type>::unsafe_cast(concrete_type0);
		type = convert_type(concrete_type.get());
	} else {
		type = get_void_type();
	}

	//Generate BNF name.
	std::ostringstream name_out;
	name_out << g_name_tr_name_prefix;
	name_out << original_name;
	const String name(name_out.str());
		
	//Create BNF terminal.
	MPtr<const NameTrDescriptor> descriptor =
		m_managed_sym_descriptors->add(new NameTrDescriptor(type, original_name));
	m_name_tokens->push_back(descriptor.get());
		
	const BnfTr* bnf_tr = m_bnf_builder.create_terminal(name, descriptor);
	conv_tr = m_managed_conv_syms->add(new conv::ConvTr(bnf_tr));
	m_tr_to_bnf_map.put(tr, conv_tr);
}

MPtr<conv::ConvTr> ns::Converter::convert_terminal(ebnf::TerminalDeclaration* tr) {
	MPtr<conv::ConvTr> conv_tr = m_tr_to_bnf_map.get(tr);
	assert(!!conv_tr);
	return conv_tr;
}

MPtr<conv::ConvNt> ns::Converter::create_auto_nonterminal(MPtr<const TypeDescriptor> type) {
	const String name = generate_auto_nt_name();
	const MPtr<const NtDescriptor> descriptor = manage_nt(new AutoNtDescriptor(type, name));
	MPtr<conv::ConvNt> conv_nt = create_nonterminal(name, descriptor);
	return conv_nt;
}

void ns::Converter::create_production(
	MPtr<conv::ConvNt> conv_nt,
	const std::vector<MPtr<conv::ConvSym>>& conv_elements,
	ActionFactory& action_factory)
{
	std::vector<const BnfSym*> bnf_elements;
	bnf_elements.reserve(conv_elements.size());
	for (MPtr<conv::ConvSym> conv_sym : conv_elements) bnf_elements.push_back(conv_sym->get_sym());
	
	ActionMPtr action = create_action(action_factory, bnf_elements);
	create_production0(conv_nt, bnf_elements, action);
}

MPtr<const ns::TypeDescriptor> ns::Converter::convert_type(const types::Type* type) {
	class ConvTypeVisitor : public TypeVisitor<MPtr<const TypeDescriptor>> {
		Converter* const m_converter;
	public:
		ConvTypeVisitor(Converter* converter) : m_converter(converter){}

		MPtr<const TypeDescriptor> visit_Type(const types::Type* type) override {
			throw err_illegal_state();
		}

		MPtr<const TypeDescriptor> visit_PrimitiveType(const types::PrimitiveType* type) override {
			return m_converter->convert_primitive_type(type);
		}

		MPtr<const TypeDescriptor> visit_ClassType(const types::ClassType* type) override {
			return m_converter->convert_class_type(type);
		}

		MPtr<const TypeDescriptor> visit_VoidType(const types::VoidType* type) override {
			return m_converter->get_void_type();
		}

		MPtr<const TypeDescriptor> visit_ArrayType(const types::ArrayType* type) override {
			MPtr<const TypeDescriptor> element_type_desc = type->get_element_type()->visit(this);
			return m_converter->m_managed_types->add(new ListTypeDescriptor(element_type_desc));
		}
	};

	ConvTypeVisitor visitor(this);
	MPtr<const TypeDescriptor> type_desc = type->visit(&visitor);
	return type_desc;
}

MPtr<const ns::ListTypeDescriptor> ns::Converter::create_list_type(MPtr<const TypeDescriptor> element_type) {
	return m_managed_types->add(new ListTypeDescriptor(element_type));
}

void ns::Converter::convert_primitive_type_init(const types::PrimitiveType* primitive_type) {
	PrimitiveTypeMap& map = primitive_type->is_system() ? m_system_primitive_type_map : m_user_primitive_type_map;

	const String& name = primitive_type->get_name();
	MPtr<const PrimitiveTypeDescriptor> result = map[name];
	assert(!result);
	result = m_managed_types->add(new PrimitiveTypeDescriptor(primitive_type));
	map[name] = result;
}

MPtr<const ns::PrimitiveTypeDescriptor> ns::Converter::convert_primitive_type(const types::PrimitiveType* primitive_type) {
	PrimitiveTypeMap& map = primitive_type->is_system() ? m_system_primitive_type_map : m_user_primitive_type_map;
	const String& name = primitive_type->get_name();
	MPtr<const PrimitiveTypeDescriptor> result = map[name];
	assert(!!result);
	return result;
}

MPtr<const ns::PrimitiveTypeDescriptor> ns::Converter::convert_string_literal_type(const types::Type* type) {
	if (type->is_void()) return MPtr<const ns::PrimitiveTypeDescriptor>();

	const types::PrimitiveType* primitive_type = type->as_primitive();
	assert(primitive_type);
	MPtr<const PrimitiveTypeDescriptor> string_literal_type = convert_primitive_type(primitive_type);
	return string_literal_type;
}

MPtr<const ns::ClassTypeDescriptor> ns::Converter::convert_class_type(const types::ClassType* class_type) {
	const String& name = class_type->get_class_name();
	std::size_t next_index = m_class_type_map.size();
	MPtr<const ClassTypeDescriptor> type = m_class_type_map[name];
	if (!type) {
		type = m_managed_types->add(new ClassTypeDescriptor(next_index, class_type->get_class_name()));
		m_class_type_map[name] = type;
	}
	return type;
}

MPtr<const ns::PartClassTypeDescriptor> ns::Converter::convert_part_class_type(
	MPtr<const ClassTypeDescriptor> class_type,
	const PartClassTag& part_class_tag)
{
	//TODO Cache part class type descriptors, do not create a new one every time.
	return m_managed_types->add(new PartClassTypeDescriptor(class_type, part_class_tag.get_index()));
}

MPtr<ns::Action> ns::Converter::manage_action(std::unique_ptr<Action> action) {
	return m_managed_actions->add(action.release());
}

MPtr<const ns::TypeDescriptor> ns::Converter::manage_type(TypeDescriptor* type) {
	return m_managed_types->add(type);
}

MPtr<const ns::VoidTypeDescriptor> ns::Converter::get_void_type() {
	if (!m_void_type) m_void_type = m_managed_types->add(new VoidTypeDescriptor());
	return m_void_type;
}

MPtr<const ns::TypeDescriptor> ns::Converter::get_symbol_type(MPtr<conv::ConvSym> conv_sym) const {
	MPtr<const SymDescriptor> descriptor = conv_sym->get_descriptor();
	MPtr<const TypeDescriptor> type = descriptor->get_type();
	assert(!!type);
	return type;
}

MPtr<conv::ConvNt> ns::Converter::create_nonterminal(const String& name, MPtr<const NtDescriptor> descriptor) {
	const BnfNt* bnf_nt = m_bnf_builder.create_nonterminal(name, descriptor);
	MPtr<conv::ConvNt> conv_nt = m_managed_conv_syms->add(new conv::ConvNt(bnf_nt));
	return conv_nt;
}

MPtr<const ns::Action> ns::Converter::create_action(
	ActionFactory& action_factory,
	const std::vector<const BnfSym*>& elements)
{
	class ConvContainer : public ActionFactory::Container {
		Converter* const m_converter;

	public:
		ConvContainer(Converter* converter) : m_converter(converter){}

		MPtr<const VoidTypeDescriptor> get_void_type() override {
			return m_converter->get_void_type();
		}
		
		MPtr<const Action> manage_action(std::unique_ptr<Action> action) override {
			return m_converter->m_managed_actions->add(action.release());
		}
	};

	class ConvTypeProduction : public ActionFactory::TypeProduction {
		const std::vector<const BnfSym*>& m_elements;

	public:
		ConvTypeProduction(const std::vector<const BnfSym*>& elements) : m_elements(elements){}

		std::size_t size() const override {
			return m_elements.size();
		}
		
		MPtr<const TypeDescriptor> get(std::size_t index) const override {
			const BnfSym* bnf_sym = m_elements[index];
			MPtr<const SymDescriptor> descriptor;
			if (const BnfNt* bnf_nt = bnf_sym->as_nt()) {
				descriptor = bnf_nt->get_nt_obj();
			} else {
				const BnfTr* bnf_tr = bnf_sym->as_tr();
				assert(bnf_tr);
				descriptor = bnf_tr->get_tr_obj();
			}
			MPtr<const TypeDescriptor> type = descriptor->get_type();
			return type;
		}
	};

	ConvContainer conv_container(this);
	ConvTypeProduction conv_production(elements);
	ActionMPtr action = action_factory.create_action(&conv_container, &conv_production);
	return action;
}

void ns::Converter::create_production0(
	MPtr<conv::ConvNt> conv_nt,
	const std::vector<const BnfSym*>& elements,
	MPtr<const Action> action)
{
	MPtr<const TypeDescriptor> pr_type = action->get_result_type();
	assert(!!pr_type);
	MPtr<const TypeDescriptor> nt_type = conv_nt->get_descriptor()->get_type();
	assert(!!nt_type);

	if (!pr_type->is_void() && !nt_type->equals(pr_type.get())) {
		//Result type of the production differs from the type of the nonterminal.
		//Create a helper nonterminal which and a production which will adapt the original production's result
		//to the target nonterminal's type.
		create_implicitly_casted_production(conv_nt, elements, action, nt_type, pr_type);
	} else {
		//Result type of the production is the same as the type of the nonterminal (or compatible).
		MPtr<const PrDescriptor> pr_descriptor = m_managed_pr_descriptors->add(new PrDescriptor(action));
		m_bnf_builder.add_production(conv_nt->get_nt(), pr_descriptor, elements);
	}
}

void ns::Converter::create_implicitly_casted_production(
	MPtr<conv::ConvNt> conv_nt,
	const std::vector<const BnfSym*>& elements,
	MPtr<const Action> action,
	MPtr<const TypeDescriptor> nt_type,
	MPtr<const TypeDescriptor> pr_type)
{
	MPtr<const PrDescriptor> pr_descriptor = m_managed_pr_descriptors->add(new PrDescriptor(action));

	MPtr<conv::ConvNt> temp_nt = create_auto_nonterminal(pr_type);
	const BnfNt* temp_bnf_nt = temp_nt->get_nt();
	m_bnf_builder.add_production(temp_bnf_nt, pr_descriptor, elements);

	MPtr<const Action> cast_action = create_implicit_cast_action(nt_type, pr_type);
	assert(nt_type->equals(cast_action->get_result_type().get()));

	std::vector<const BnfSym*> cast_elements;
	cast_elements.push_back(temp_bnf_nt);
	MPtr<const PrDescriptor> cast_pr_descriptor = m_managed_pr_descriptors->add(new PrDescriptor(cast_action));
	m_bnf_builder.add_production(conv_nt->get_nt(), cast_pr_descriptor, cast_elements);
}

MPtr<const ns::Action> ns::Converter::create_implicit_cast_action(
	MPtr<const TypeDescriptor> cast_type,
	MPtr<const TypeDescriptor> source_type)
{
	const ClassTypeDescriptor* class_source_type = source_type->as_class_type();
	assert(class_source_type);
	const ClassTypeDescriptor* class_cast_type = cast_type->as_class_type();
	assert(class_cast_type);

	assert(class_cast_type == cast_type.get());
	MPtr<const ClassTypeDescriptor> class_cast_type_ptr = MPtr<const ClassTypeDescriptor>::unsafe_cast(class_cast_type);
	assert(class_source_type == source_type.get());
	MPtr<const ClassTypeDescriptor> class_source_type_ptr = MPtr<const ClassTypeDescriptor>::unsafe_cast(class_source_type);
	
	return m_managed_actions->add(new CastAction(class_cast_type_ptr, class_source_type_ptr));
}

MPtr<const ns::NtDescriptor> ns::Converter::manage_nt(NtDescriptor* nt) {
	m_nts->push_back(nt);
	return m_managed_sym_descriptors->add(nt);
}

String ns::Converter::generate_auto_nt_name() {
	std::ostringstream outs;
	outs << g_auto_nt_name_prefix;
	outs << m_auto_nt_name_index++;
	return String(outs.str());
}

MPtr<const ns::TypeDescriptor> ns::Converter::get_string_literal_type_desc() {
	if (!m_string_literal_type_desc) m_string_literal_type_desc = convert_type(m_string_literal_type);
	return m_string_literal_type_desc;
}

//
//ConcreteBNFPrinter
//

namespace {

	class ConcreteBNFPrinter {
		std::ostream& m_out;

	private:
		ConcreteBNFPrinter(std::ostream& out);

	public:
		static void print_concrete_bnf(std::ostream& out, const ns::ConcreteBNF* bnf);
	
	private:
		void print_concrete_bnf0(const ns::ConcreteBNF* bnf) const;
		void print_nt(const BnfNt* nt) const;
		void print_pr(const BnfPr* pr) const;
		void print_sym(const BnfSym* sym) const;
	};

}

ConcreteBNFPrinter::ConcreteBNFPrinter(std::ostream& out) : m_out(out){}

void ConcreteBNFPrinter::print_concrete_bnf(std::ostream& out, const ns::ConcreteBNF* bnf) {
	ConcreteBNFPrinter printer(out);
	printer.print_concrete_bnf0(bnf);
}

void ConcreteBNFPrinter::print_concrete_bnf0(const ns::ConcreteBNF* bnf) const {
	typedef std::vector<const BnfNt*> NtVector;
	const NtVector& nts = bnf->get_nonterminals();
	for (const BnfNt* nt : nts) print_nt(nt);
}

void ConcreteBNFPrinter::print_nt(const BnfNt* nt) const {
	m_out << nt->get_name() << " { ";
	MPtr<const ns::NtDescriptor> desc = nt->get_nt_obj();
	desc->get_type()->print(m_out);
	m_out << " }\n";

	const std::vector<const BnfPr*>& prs = nt->get_productions();
	for (std::size_t i = 0, n = prs.size(); i < n; ++i) {
		m_out << "\t" << (i == 0 ? ":" : "|");
		const BnfPr* pr = prs[i];
		print_pr(pr);
		m_out << '\n';
	}

	m_out << '\n';
}

void ConcreteBNFPrinter::print_pr(const BnfPr* pr) const {
	const std::vector<const BnfSym*>& elems = pr->get_elements();
	for (const BnfSym* elem : elems) {
		m_out << " ";
		print_sym(elem);
	}
	m_out << " { ";
	pr->get_pr_obj()->get_action()->print(m_out);
	m_out << " }";
}

void ConcreteBNFPrinter::print_sym(const BnfSym* sym) const {
	if (sym->as_nt()) {
		m_out << sym->get_name();
	} else {
		const BnfTr* tr = sym->as_tr();
		assert(tr);
		tr->get_tr_obj()->print(m_out);
	}
}

//
//convert_EBNF_to_BNF()
//

namespace {
	typedef std::vector<const ns::PrimitiveTypeDescriptor*> PrimitiveTypeVec;

	void fill_primitive_types(const PrimitiveTypeMap& map, PrimitiveTypeVec& vector) {
		for (const PrimitiveTypeMap::value_type& value : map) vector.push_back(value.second.get());
	}
}//namespace

unique_ptr<ns::ConversionResult> ns::Converter::convert_EBNF(
	bool verbose_output,
	GrammarBuildingResult* building_result)
{
	MPtr<ebnf::Grammar> ebnf_grammar = building_result->get_grammar();

	//Convert primitive types (everyone must be converted, even if it is not referenced).
	const std::vector<const types::PrimitiveType*>& primitive_types = building_result->get_primitive_types();
	for (const types::PrimitiveType* type : primitive_types) convert_primitive_type_init(type);

	//Convert every terminal declaration (every declaration must be converted, even if it is not referenced).
	const std::vector<ebnf::TerminalDeclaration*>& ebnf_trs = ebnf_grammar->get_terminals();
	for (ebnf::TerminalDeclaration* tr : ebnf_trs) convert_terminal_init(tr);

	//Convert every nonterminal.
	const std::vector<ebnf::NonterminalDeclaration*>& ebnf_nts = ebnf_grammar->get_nonterminals();
	for (ebnf::NonterminalDeclaration* nt : ebnf_nts) convert_nonterminal(nt);

	//Create BNF Grammar.
	std::unique_ptr<const BnfGrm> bnf_grammar(m_bnf_builder.create_grammar());
	
	if (verbose_output) {
		std::cout << "*** BNF GRAMMAR ***\n";
		std::cout << '\n';
		ConcreteBNFPrinter::print_concrete_bnf(std::cout, bnf_grammar.get());
		std::cout << '\n';
	}

	//Convert the type of custom token.
	MPtr<const types::Type> string_literal_type0 = building_result->get_string_literal_type();
	MPtr<const PrimitiveTypeDescriptor> string_literal_type =
		convert_string_literal_type(string_literal_type0.get());

	//Create ConversionResult.
	building_result->get_common_heap()->add_heap(std::move(m_managed_heap));

	unique_ptr<PrimitiveTypeVec> primitive_type_descriptors = make_unique1<PrimitiveTypeVec>();
	primitive_type_descriptors->reserve(m_system_primitive_type_map.size() + m_user_primitive_type_map.size());
	fill_primitive_types(m_system_primitive_type_map, *primitive_type_descriptors);
	fill_primitive_types(m_user_primitive_type_map, *primitive_type_descriptors);

	unique_ptr<ConversionResult> result = make_unique1<ConversionResult>(
		building_result,
		std::move(bnf_grammar),
		util::const_vector_ptr(std::move(m_start_bnf_nts)),
		util::const_vector_ptr(std::move(m_nts)),
		util::const_vector_ptr(std::move(m_name_tokens)),
		util::const_vector_ptr(std::move(m_str_tokens)),
		util::const_vector_ptr(std::move(primitive_type_descriptors)),
		string_literal_type,
		m_class_type_map.size());
	
	return result;
}

unique_ptr<ns::ConversionResult> ns::convert_EBNF_to_BNF(
	bool verbose_output,
	unique_ptr<GrammarBuildingResult> building_result)
{
	unique_ptr<MHeap> managed_heap = make_unique1<MHeap>();
	MHeap* managed_heap_ref = managed_heap.get();
	MPtr<ebnf::Grammar> ebnf_grammar = building_result->get_grammar();
	std::size_t tr_count = ebnf_grammar->get_tr_count();
	std::size_t nt_count = ebnf_grammar->get_nt_count();
	const types::Type* string_literal_type = building_result->get_string_literal_type().get();
	Converter converter(std::move(managed_heap), managed_heap_ref, string_literal_type, tr_count, nt_count);
	
	return converter.convert_EBNF(verbose_output, building_result.get());
}
