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

//Semantic actions code generator implementation.

#include <algorithm>
#include <cassert>
#include <functional>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "action.h"
#include "codegen_action.h"
#include "concrete_lr.h"
#include "ebnf__imp.h"
#include "util.h"
#include "util_mptr.h"
#include "util_string.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace types = ns::types;
namespace util = ns::util;

using util::IndexedSet;
using util::MPtr;
using util::String;

//
//Misc.
//

namespace {
	MPtr<const ns::SymDescriptor> get_sym_descriptor(const ns::ConcreteBNF::Sym* sym) {
		if (const ns::ConcreteBNF::Nt* nt = sym->as_nt()) return nt->get_nt_obj();

		const ns::ConcreteBNF::Tr* tr = sym->as_tr();
		assert(tr);
		return tr->get_tr_obj();
	}

	MPtr<const ns::TypeDescriptor> get_element_type(const ns::ConcreteBNF::Pr* pr, std::size_t ofs) {
		const std::vector<const ns::ConcreteBNF::Sym*>& elements = pr->get_elements();
		assert(ofs < elements.size());
		const ns::ConcreteBNF::Sym* sym = elements[ofs];
		MPtr<const ns::SymDescriptor> desc = get_sym_descriptor(sym);
		MPtr<const ns::TypeDescriptor> type = desc->get_type();
		assert(!!type);
		assert(!type->is_void());
		return type;
	}

	struct ClassTypeIndexFn {
		std::size_t operator()(MPtr<const ns::ClassTypeDescriptor> class_type) const {
			return class_type->get_index();
		}
	};

	void generate_return_statement(std::ostream& out, const char* value_str) {
		out << "\treturn NodePtr(" << value_str << ", &syn::null_deallocator);\n";
	}

	template<class T>
	bool contains(const std::vector<T>& vec, const T& value) {
		return std::find(vec.begin(), vec.end(), value) != vec.end();
	}
}//namespace

//
//ActionCodeGenerator
//

ns::ActionCodeGenerator::ActionCodeGenerator(
	const CommandLine& command_line,
	const std::string& type_namespace,
	const std::string& class_namespace,
	const std::string& code_namespace,
	const std::string& native_namespace,
	const ConcreteLRResult* lr_result)
	: m_command_line(command_line),
	m_type_namespace(type_namespace),
	m_class_namespace(class_namespace),
	m_code_namespace(code_namespace),
	m_native_namespace(native_namespace),
	m_lr_result(lr_result),
	m_class_type_count(lr_result->get_class_type_count()),
	m_pr_action_map(lr_result->get_bnf_grammar()->get_productions().size()),
	m_indent("")
{
	const ConcreteBNF* bnf = lr_result->get_bnf_grammar();

	//Fill production map and vector.
	const std::vector<const ConcreteLRNt*>& bnf_nts = bnf->get_nonterminals();
	for (std::size_t i_nt = 0, n_nts = bnf_nts.size(); i_nt < n_nts; ++i_nt) {
		const ConcreteLRNt* nt = bnf_nts[i_nt];
		const std::vector<const ConcreteLRPr*>& prs = nt->get_productions();
		for (std::size_t i_pr = 0, n_prs = prs.size(); i_pr < n_prs; ++i_pr) {
			const ConcreteLRPr* pr = prs[i_pr];
			const ActionInfo action_info(pr, m_action_vector.size(), i_pr);
			m_pr_action_map.put(pr, action_info);
			m_action_vector.push_back(action_info);
		}
	}

	//Ensure that all productions have been tracked.
	const std::vector<const ConcreteLRPr*>& bnf_prs = bnf->get_productions();
	assert(m_action_vector.size() == bnf_prs.size());
	for (std::size_t i_pr = 0, n_prs = bnf_prs.size(); i_pr < n_prs; ++i_pr) {
		const ConcreteLRPr* pr = bnf_prs[i_pr];
		assert(m_pr_action_map.contains(pr));
	}

	//Find used primitive types.
	for (const ConcreteLRTr* tr : bnf->get_terminals()) {
		MPtr<const TypeDescriptor> type = tr->get_tr_obj()->get_type();
		const PrimitiveTypeDescriptor* ptype = type->as_primitive_type();
		if (ptype && !contains(m_used_primitive_types, ptype)) {
			m_used_primitive_types.push_back(ptype);
		}
	}
}

const ns::ActionInfo& ns::ActionCodeGenerator::get_action_info(const ns::ConcreteLRPr* pr) const {
	const PrActionMap::const_iterator it = m_pr_action_map.find(pr);
	assert(m_pr_action_map.end() != it);
	return it.value();
}

void ns::ActionCodeGenerator::generate_action_declarations(std::ostream& out) {
	out << "\tstruct Actions {\n";

	out << "\t\tenum Productions {\n";
	for (std::size_t i = 0, n = m_action_vector.size(); i < n; ++i) {
		out << "\t\t\t";
		generate_production_constant_name(out, m_action_vector[i]);
		out << ((i < n - 1) ? "," : "") << '\n';
	}
	out << "\t\t};\n\n";

	out << "\t\tstd::vector<const StackEl*> m_stack_vector;\n\n";

	if (!m_used_primitive_types.empty()) {
		for (const PrimitiveTypeDescriptor* type : m_used_primitive_types) {
			out << "\t\t";
			generate_type__primitive(out, type);
			out << " ";
			generate_primitive_type_function_name(out, type);
			out << "(const StackEl* node);\n";
		}
		out << '\n';
	}

	const ConcreteBNF* bnf = m_lr_result->get_bnf_grammar();
	for (const ConcreteLRNt* nt : bnf->get_nonterminals()) {
		MPtr<const TypeDescriptor> type = nt->get_nt_obj()->get_type();
		if (!type->is_void()) {
			out << "\t\t";
			generate_nt_function_type(out, type);
			out << " ";
			generate_nonterminal_function_name(out, nt);
			out << "(const StackEl* node";
			generate_nt_function_parameters(out, type);
			out << ");\n";
		}
	}

	out << "\t};\n";
}

void ns::ActionCodeGenerator::generate_actions(std::ostream& out) {
	if (!m_used_primitive_types.empty()) {
		for (const PrimitiveTypeDescriptor* type : m_used_primitive_types) {
			generate_type__primitive(out, type);
			out << " " << m_code_namespace << "::Actions::";
			generate_primitive_type_function_name(out, type);
			out << "(const StackEl* node) {\n";
			out << "\tconst StackValue* val_node = node->as_value();\n";
			out << "\tconst void* value = val_node->value();\n";
			out << "\treturn *static_cast<const ";
			generate_type__primitive(out, type);
			out << "*>(value);\n";
			out << "}\n";
		}
		out << '\n';
	}

	const ConcreteBNF* bnf = m_lr_result->get_bnf_grammar();
	for (const ConcreteLRNt* nt : bnf->get_nonterminals()) {
		MPtr<const TypeDescriptor> type = nt->get_nt_obj()->get_type();
		if (!type->is_void()) {
			generate_nt_function_type(out, type);
			out << " " << m_code_namespace << "::Actions::";
			generate_nonterminal_function_name(out, nt);
			out << "(const StackEl* node";
			generate_nt_function_parameters(out, type);
			out << ") {\n";
			generate_action_code_nt(out, nt);
			out << "}\n\n";
		}
	}
}

void ns::ActionCodeGenerator::generate_primitive_type(std::ostream& out, const PrimitiveTypeDescriptor* type) const {
	const types::PrimitiveType* type0 = type->get_primitive_type();
	bool system = type0->is_system();
	out << (system ? "syn" : m_type_namespace);
	out << "::" << type0->get_name();
}

namespace {
	void generate_pool_member_base_name(std::ostream& out, const types::PrimitiveType* type) {
		out << (type->is_system() ? "s" : "v");
		out << "_" << type->get_name();
	}
}//namespace

void ns::ActionCodeGenerator::generate_value_pool_member_name(std::ostream& out, const types::PrimitiveType* type) const {
	out << "m_";
	generate_pool_member_base_name(out, type);
	out << "_pool";
}

void ns::ActionCodeGenerator::generate_value_pool_allocator_name(std::ostream& out, const types::PrimitiveType* type) const {
	out << "alloc";
	generate_pool_member_base_name(out, type);
}

void ns::ActionCodeGenerator::generate_action_code_nt(std::ostream& out, const ConcreteLRNt* nt) {
	out << "\tProductionStack stack(m_stack_vector, node);\n";

	const char* old_indent = m_indent;

	const std::vector<const ConcreteLRPr*>& prs = nt->get_productions();
	if (1 == prs.size()) {
		const ConcreteLRPr* pr = prs[0];
		const ActionInfo& action_info = get_action_info(pr);
		std::size_t len = pr->get_elements().size();
		out << "\tcheck_production(stack, ";
		generate_production_constant_name(out, action_info);
		out << ", " << len << ");\n";
		m_indent = "\t";
		generate_action_code_pr(out, pr);
	} else {
		assert(prs.size() > 1);
		const char* separator = "";
		for (const ConcreteLRPr* pr : prs) {
			//TODO Generate production range checking code.
			const ActionInfo& action_info = get_action_info(pr);
			std::size_t len = pr->get_elements().size();
			out << "\t" << separator << "if (is_production(stack, ";
			generate_production_constant_name(out, action_info);
			out << ", " << len << ")) {\n";
			m_indent = "\t\t";
			generate_action_code_pr(out, pr);
			separator = "} else ";
		}

		out << "\t} else {\n";
		out << "\t\tthrow syn::illegal_state();\n";
		out << "\t}\n";
	}

	m_indent = old_indent;
}

void ns::ActionCodeGenerator::generate_action_code_pr(std::ostream& out, const ConcreteLRPr* pr) {
	class ActVisitor : public ActionVisitor {
		std::ostream& m_out;
		const ConcreteBNF::Pr* const m_pr;
		ActionCodeGenerator* const m_generator;
	public:
		ActVisitor(ActionCodeGenerator *generator, std::ostream& out, const ConcreteBNF::Pr* pr)
			: m_generator(generator), m_out(out), m_pr(pr)
		{}
		
		void visit_VoidAction(const VoidAction* action) override {
			m_generator->generate_action_code__void(m_out, m_pr, action);
		}
		void visit_CopyAction(const CopyAction* action) override {
			m_generator->generate_action_code__copy(m_out, m_pr, action);
		}
		void visit_CastAction(const CastAction* action) override {
			m_generator->generate_action_code__cast(m_out, m_pr, action);
		}
		void visit_ClassAction(const ClassAction* action) override {
			m_generator->generate_action_code__class(m_out, m_pr, action);
		}
		void visit_PartClassAction(const PartClassAction* action) override {
			m_generator->generate_action_code__part_class(m_out, m_pr, action);
		}
		void visit_ResultAndAction(const ResultAndAction* action) override {
			m_generator->generate_action_code__result_and(m_out, m_pr, action);
		}
		void visit_FirstListAction(const FirstListAction* action) override {
			m_generator->generate_action_code__first_list(m_out, m_pr, action);
		}
		void visit_NextListAction(const NextListAction* action) override {
			m_generator->generate_action_code__next_list(m_out, m_pr, action);
		}
		void visit_ConstAction(const ConstAction* action) override {
			m_generator->generate_action_code__const(m_out, m_pr, action);
		}
	};

	MPtr<const ns::Action> action = pr->get_pr_obj()->get_action();
	ActVisitor visitor(this, out, pr);
	action->visit(&visitor);
}

void ns::ActionCodeGenerator::generate_action_code__void(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const VoidAction* action)
{
	class Visitor : public TypeDescriptorVisitor {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
		const MPtr<const TypeDescriptor> m_type;
	public:
		Visitor(ActionCodeGenerator* generator, std::ostream& out, MPtr<const TypeDescriptor> type)
			: m_generator(generator), m_out(out), m_type(type)
		{}

		void general_type() {
			m_out << m_generator->m_indent << "return ";
			m_generator->generate_type_external(m_out, m_type);
			m_out << "();\n";
		}
		
		void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) override {
			general_type();
		}
		void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) override {
			//Nothing.
		}
		void visit_ListTypeDescriptor(const ListTypeDescriptor* type) override {
			m_out << m_generator->m_indent << "return InAlloc::list_null<";
			m_generator->generate_type_internal(m_out, type->get_element_type());
			m_out << ">();\n";
		}
		void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) override {
			general_type();
		}
	};

	MPtr<const TypeDescriptor> type = pr->get_nt()->get_nt_obj()->get_type();
	Visitor visitor(this, out, type);
	type->visit(&visitor);
}

void ns::ActionCodeGenerator::generate_action_code__copy(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const CopyAction* action)
{
	assert(1 == pr->get_elements().size());
	generate_action_code__result_and0(out, pr, 0);
}

void ns::ActionCodeGenerator::generate_action_code__cast(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const CastAction* action)
{
	assert(1 == pr->get_elements().size());
	const ConcreteLRSym* sym = pr->get_elements()[0];
	const ConcreteLRNt* nt = sym->as_nt();
	if (!nt) throw std::logic_error("illegal state");

	MPtr<const ClassTypeDescriptor> cast_type = action->get_concrete_result_type();
	MPtr<const ClassTypeDescriptor> expr_type = action->get_actual_type();

	const String& expr_class_name = expr_type->get_class_name();
	const String& cast_class_name = cast_type->get_class_name();

	out << m_indent << "return ";
	generate_nonterminal_function_name(out, nt);
	out << "(stack[0]);\n";
}

void ns::ActionCodeGenerator::generate_action_code__class(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const ClassAction* action)
{
	MPtr<const ClassTypeDescriptor> class_type = action->get_class_type();
	const String& class_name = class_type->get_class_name();

	out << m_indent << "ExAlloc::Ptr<";
	generate_class_name(out, class_name, m_class_namespace);
	out << "> obj = ExAlloc::create<";
	generate_class_name(out, class_name, m_class_namespace);
	out << ">();\n";

	generate_action_code__abstract_class(out, pr, action);

	out << m_indent << "return obj;\n";
}

void ns::ActionCodeGenerator::generate_action_code__part_class(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const PartClassAction* action)
{
	generate_action_code__abstract_class(out, pr, action);
}

void ns::ActionCodeGenerator::generate_action_code__result_and(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const ResultAndAction* action)
{
	generate_action_code__result_and0(out, pr, action->get_result_index());
}

void ns::ActionCodeGenerator::generate_action_code__result_and0(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	std::size_t ofs)
{
	out << m_indent << "return ";
	generate_get_stack(out, pr, ofs);
	out << ";\n";
}

void ns::ActionCodeGenerator::generate_action_code__first_list(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const FirstListAction* action)
{
	assert(1 == pr->get_elements().size());
	MPtr<const TypeDescriptor> elem_type = get_element_type(pr, 0);

	out << m_indent;
	generate_type_internal(out, elem_type);
	out << " elem = ";
	generate_get_stack(out, pr, 0);
	out << ";\n";

	out << m_indent << "return InAlloc::list_first(elem);\n";
}

void ns::ActionCodeGenerator::generate_action_code__next_list(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const NextListAction* action)
{
	std::size_t elem_ofs = action->is_separator() ? 2 : 1;
	assert((elem_ofs + 1) == pr->get_elements().size());
	MPtr<const TypeDescriptor> elem_type = get_element_type(pr, elem_ofs);

	out << m_indent << "InAlloc::ListPtr<";
	generate_type_internal(out, elem_type);
	out << "> list = ";
	generate_get_stack(out, pr, 0);
	out << ";\n";

	out << m_indent;
	generate_type_internal(out, elem_type);
	out << " elem = ";
	generate_get_stack(out, pr, elem_ofs);
	out << ";\n";

	out << m_indent << "InAlloc::list_next(list, elem);\n";
	out << m_indent << "return InAlloc::list_move(list);\n";
}

void ns::ActionCodeGenerator::generate_action_code__const(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const ConstAction* action)
{
	MPtr<const PrimitiveTypeDescriptor> type = action->get_concrete_result_type();
	const types::PrimitiveType* type0 = type->get_primitive_type();

	out << m_indent << "return ";
	const ebnf::ConstExpression* expr = action->get_const_expr();
	generate_const_expression(out, expr);
	out << ";\n";
}

void ns::ActionCodeGenerator::generate_action_code__abstract_class(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	const AbstractClassAction* action)
{
	const ActionDefs::AttributeVector& attributes = action->get_attributes();
	const ActionDefs::PartClassVector& part_classes = action->get_part_classes();

	//Generate attributes settings code.
	for (std::size_t i = 0, n = attributes.size(); i < n; ++i) {
		const ActionDefs::AttributeField& attribute = attributes[i];
		out << m_indent << "obj->";
		fmt_attr_set_begin(out, attribute.m_name);
		generate_get_stack_attribute(out, pr, attribute.m_offset);
		fmt_attr_set_end(out);
		out << ";\n";
	}

	//Generate pert-classes setting code.
	for (const ActionDefs::PartClassField& part_class : part_classes) {
		std::size_t ofs = part_class.m_offset;
		const ConcreteLRNt* sub_nt = pr->get_elements()[ofs]->as_nt();
		if (!sub_nt) throw std::logic_error("illegal state");

		out << m_indent;
		generate_nonterminal_function_name(out, sub_nt);
		out << "(stack[" << ofs << "], obj);\n";
	}
}

bool ns::ActionCodeGenerator::generate_internal_to_external_conversion(
	std::ostream& out,
	util::MPtr<const TypeDescriptor> type)
{
	struct Visitor : public TypeDescriptorVisitor {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
		bool m_result;

		Visitor(ActionCodeGenerator* generator, std::ostream& out)
			: m_generator(generator), m_out(out), m_result(false)
		{}

		void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) override {
			//Nothing.
		}
		void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ListTypeDescriptor(const ListTypeDescriptor* type) override {
			m_generator->generate_internal_to_external_conversion__list(m_out, type);
			m_result = true;
		}
		void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) override {
			//Nothing.
		}
	};

	Visitor visitor(this, out);
	type->visit(&visitor);
	return visitor.m_result;
}

void ns::ActionCodeGenerator::generate_internal_to_external_conversion__list(
	std::ostream& out,
	const ListTypeDescriptor* type)
{
	struct Visitor : public TypeDescriptorVisitor {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
		bool m_result;

		Visitor(ActionCodeGenerator* generator, std::ostream& out)
			: m_generator(generator), m_out(out), m_result(false)
		{}

		void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) override {
			m_out << "ExAlloc::node_list";
		}
		void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ListTypeDescriptor(const ListTypeDescriptor* type) override {
			m_out << "ExAlloc::node_list";
		}
		void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) override {
			m_out << "ExAlloc::list";
		}
	};

	Visitor visitor(this, out);
	type->get_element_type()->visit(&visitor);
}

void ns::ActionCodeGenerator::generate_nonterminal_function_name(std::ostream& out, const ConcreteLRNt* nt) {
	out << "nt__" << nt->get_name();
}

void ns::ActionCodeGenerator::generate_primitive_type_function_name(
	std::ostream& out,
	const PrimitiveTypeDescriptor* type)
{
	out << "tr__" << type->get_primitive_type()->get_name();
}

void ns::ActionCodeGenerator::generate_nt_function_type(std::ostream& out, MPtr<const TypeDescriptor> type) {
	class Visitor : public TypeDescriptorVisitor {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
		const MPtr<const TypeDescriptor> m_type;
	public:
		Visitor(ActionCodeGenerator* generator, std::ostream& out, MPtr<const TypeDescriptor> type)
			: m_generator(generator), m_out(out), m_type(type)
		{}
		
		void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) override {
			m_generator->generate_type_internal(m_out, m_type);
		}
		void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) override {
			m_generator->generate_type_internal(m_out, m_type);
		}
		void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) override {
			m_out << "void";
		}
		void visit_ListTypeDescriptor(const ListTypeDescriptor* type) override {
			m_generator->generate_type_internal(m_out, m_type);
		}
		void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) override {
			m_generator->generate_type_internal(m_out, m_type);
		}
	};

	Visitor visitor(this, out, type);
	type->visit(&visitor);
}

void ns::ActionCodeGenerator::generate_nt_function_parameters(std::ostream& out, MPtr<const TypeDescriptor> type) {
	class Visitor : public TypeDescriptorVisitor {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
	public:
		Visitor(ActionCodeGenerator* generator, std::ostream& out) : m_generator(generator), m_out(out){}

		void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) override {
			throw err_illegal_state();
		}
		void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) override {
			//Nothing.
		}
		void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) override {
			m_out << ", const ";
			m_generator->generate_type_internal(m_out, type->get_class_type());
			m_out << "& obj";
		}
		void visit_ListTypeDescriptor(const ListTypeDescriptor* type) override {
			//Nothing.
		}
		void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) override {
			//Nothing.
		}
	};

	Visitor visitor(this, out);
	type->visit(&visitor);
}

void ns::ActionCodeGenerator::generate_production_constant_name(
	std::ostream& out,
	const ns::ActionInfo& action_info) const
{
	out << "Pr_" << action_info.m_action_index << "__" << action_info.m_pr->get_nt()->get_name()
		<< "_" << action_info.m_nt_local_index;
}

void ns::ActionCodeGenerator::generate_class_name(
	std::ostream& out,
	const String& class_name,
	const std::string& name_space) const
{
	if (!name_space.empty()) out << name_space << "::";
	out << class_name;
}

void ns::ActionCodeGenerator::generate_type_external(std::ostream& out, MPtr<const TypeDescriptor> type) {
	class Visitor : public TypeDescriptorVisitor {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
	public:
		Visitor(ActionCodeGenerator* generator, std::ostream& out) : m_generator(generator), m_out(out){}
		
		void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) override {
			throw err_illegal_state();
		}
		void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) override {
			m_generator->generate_type__class(m_out, type);
		}
		void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) override {
			throw err_illegal_state();
		}
		void visit_ListTypeDescriptor(const ListTypeDescriptor* type) override {
			m_generator->generate_type_external__list(m_out, type);
		}
		void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) override {
			m_generator->generate_type__primitive(m_out, type);
		}
	};

	Visitor visitor(this, out);
	type->visit(&visitor);
}

void ns::ActionCodeGenerator::generate_type__class(std::ostream& out, const ClassTypeDescriptor* type) {
	out << "ExAlloc::Ptr<";
	generate_type__class_no_ptr(out, type);
	out << ">";
}

void ns::ActionCodeGenerator::generate_type__class_no_ptr(std::ostream& out, const ClassTypeDescriptor* type) {
	generate_class_name(out, type->get_class_name(), m_class_namespace);
}

void ns::ActionCodeGenerator::generate_type_external__list(std::ostream& out, const ListTypeDescriptor* type) {
	out << "ExAlloc::Ptr<";
	generate_type_external__list_no_ptr(out, type);
	out << ">";
}

void ns::ActionCodeGenerator::generate_type_external__list_no_ptr(
	std::ostream& out,
	const ListTypeDescriptor* type)
{
	class Visitor : public TypeDescriptorVisitor {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
	public:
		Visitor(ActionCodeGenerator* generator, std::ostream& out) : m_generator(generator), m_out(out){}
		
		void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) override {
			m_out << "ExAlloc::NodeList<";
			m_generator->generate_type__class_no_ptr(m_out, type);
			m_out << ">";
		}
		void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ListTypeDescriptor(const ListTypeDescriptor* type) override {
			m_out << "ExAlloc::NodeList<";
			m_generator->generate_type_external__list_no_ptr(m_out, type);
			m_out << ">";
		}
		void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) override {
			m_out << "ExAlloc::List<";
			m_generator->generate_type__primitive(m_out, type);
			m_out << ">";
		}
	};

	Visitor visitor(this, out);
	type->get_element_type()->visit(&visitor);
}

void ns::ActionCodeGenerator::generate_type_internal(std::ostream& out, util::MPtr<const TypeDescriptor> type) {
	class Visitor : public TypeDescriptorVisitor {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
	public:
		Visitor(ActionCodeGenerator* generator, std::ostream& out) : m_generator(generator), m_out(out){}
		
		void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) override {
			m_generator->generate_type__class(m_out, type);
		}
		void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) override {
			throw std::logic_error("illegal state");
		}
		void visit_ListTypeDescriptor(const ListTypeDescriptor* type) override {
			m_generator->generate_type_internal__list(m_out, type);
		}
		void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) override {
			m_generator->generate_type__primitive(m_out, type);
		}
	};

	Visitor visitor(this, out);
	type->visit(&visitor);
}

void ns::ActionCodeGenerator::generate_type_internal__list(std::ostream& out, const ListTypeDescriptor* type) {
	out << "InAlloc::ListPtr<";
	generate_type_internal(out, type->get_element_type());
	out << ">";
}

void ns::ActionCodeGenerator::generate_type__primitive(std::ostream& out, const PrimitiveTypeDescriptor* type) {
	const types::PrimitiveType* type0 = type->get_primitive_type();
	if (type0->is_system()) {
		out << "syn";
	} else {
		out << m_type_namespace;
	}
	out << "::" << type0->get_name();
}

void ns::ActionCodeGenerator::generate_get_stack(std::ostream& out, const ConcreteBNF::Pr* pr, std::size_t ofs) {
	const ConcreteLRSym* sym = pr->get_elements()[ofs];
	const ConcreteLRNt* nt = sym->as_nt();
	if (nt) {
		generate_nonterminal_function_name(out, nt);
	} else {
		const ConcreteLRTr* tr = sym->as_tr();
		if (!tr) throw std::logic_error("illegal state");
		MPtr<const TypeDescriptor> type = tr->get_tr_obj()->get_type();
		if (type->is_void()) throw std::logic_error("illegal state");
		const PrimitiveTypeDescriptor* ptype = type->as_primitive_type();
		if (!ptype) throw std::logic_error("illegal state");
		generate_primitive_type_function_name(out, ptype);
	}

	out << "(stack[" << ofs << "])";
}

void ns::ActionCodeGenerator::generate_get_stack_attribute(
	std::ostream& out,
	const ConcreteBNF::Pr* pr,
	std::size_t ofs)
{
	MPtr<const TypeDescriptor> type = get_element_type(pr, ofs);
	bool conv = generate_internal_to_external_conversion(out, type);
	out << (conv ? "(" : "");
	generate_get_stack(out, pr, ofs);
	out << (conv ? ")" : "");
}

void ns::ActionCodeGenerator::generate_const_expression(std::ostream& out, const ebnf::ConstExpression* expr) {
	class Visitor : public ns::ConstExpressionVisitor<void> {
		ActionCodeGenerator* const m_generator;
		std::ostream& m_out;
	public:
		Visitor(ActionCodeGenerator* generator, std::ostream& out) : m_generator(generator), m_out(out){}

		void visit_ConstExpression(const ebnf::ConstExpression* expr) override {
			throw err_illegal_state();
		}
		void visit_IntegerConstExpression(const ebnf::IntegerConstExpression* expr) override {
			m_out << expr->get_value();
		}
		void visit_StringConstExpression(const ebnf::StringConstExpression* expr) override {
			m_generator->generate_const_expr__string(m_out, expr);
		}
		void visit_BooleanConstExpression(const ebnf::BooleanConstExpression* expr) override {
			m_out << expr->get_value() ? "true" : "false";
		}
		void visit_NativeConstExpression(const ebnf::NativeConstExpression* expr) override {
			m_generator->generate_const_expr__native(m_out, expr);
		}
	};

	Visitor visitor(this, out);
	expr->visit(&visitor);
}

namespace {
	bool is_valid_literal_character(char c) {
		//TODO Make this function platform-independent.
		return c >= 0x20 && c < 0x80;
	}
}//namespace

void ns::ActionCodeGenerator::generate_const_expr__string(std::ostream& out, const ebnf::StringConstExpression* expr) {
	out << "\"";

	const std::string& str = expr->get_value().str();
	for (char c : str) {
		if ('\r' == c) {
			out << "\\r";
		} else if ('\n' == c) {
			out << "\\n";
		} else if ('\t' == c) {
			out << "\\t";
		} else if (is_valid_literal_character(c)) {
			out << c;
		} else {
			out << "\\" << std::setw(3) << std::setfill('0') << std::oct << (int)c;
		}
	}

	out << "\"";
}

void ns::ActionCodeGenerator::generate_const_expr__native(std::ostream& out, const ebnf::NativeConstExpression* expr) {
	out << m_native_namespace << "::";

	//Qualifiers.
	const std::vector<ns::syntax_string>& qualifiers = expr->get_qualifiers();
	for (const ns::syntax_string& str : qualifiers) out << str << "::";

	//Name.
	const ebnf::NativeName* base_name = expr->get_name();
	generate_const_expr__native_name(out, base_name);

	//References.
	const std::vector<MPtr<const ebnf::NativeReference>>& references = expr->get_references();
	for (MPtr<const ebnf::NativeReference> reference : references) {
		out << reference->is_pointer() ? "->" : ".";
		const ebnf::NativeName* ref_name = reference->get_native_name();
		generate_const_expr__native_name(out, ref_name);
	}
}

void ns::ActionCodeGenerator::generate_const_expr__native_name(std::ostream& out, const ebnf::NativeName* name) {
	const ebnf::NativeVariableName* variable = name->as_variable();
	if (variable) {
		out << variable->get_name();
	} else {
		const ebnf::NativeFunctionName* function = name->as_function();
		assert(function);
		generate_const_expr__native_name__function(out, function);
	}
}

void ns::ActionCodeGenerator::generate_const_expr__native_name__function(
	std::ostream& out,
	const ebnf::NativeFunctionName* function)
{
	out << function->get_name() << "(";

	const std::vector<MPtr<const ebnf::ConstExpression>>& args = function->get_arguments();
	for (std::size_t i = 0, n = args.size(); i < n; ++i) {
		out << i ? ", " : "";
		MPtr<const ebnf::ConstExpression> arg = args[i];
		generate_const_expression(out, arg.get());
	}

	out << ")";
}

void ns::ActionCodeGenerator::fmt_attr_set_begin(std::ostream& out, const util::String& name) {
	const std::string& pattern = m_command_line.get_attr_name_pattern();
	if (pattern.empty()) {
		out << name;
	} else {
		for (char c : pattern) {
			if ('^' == c) {
				out << name;
			} else {
				out << c;
			}
		}
	}

	out << (m_command_line.is_use_attr_setters() ? "(" : " = ");
}

void ns::ActionCodeGenerator::fmt_attr_set_end(std::ostream& out) {
	if (m_command_line.is_use_attr_setters()) out << ")";
}
