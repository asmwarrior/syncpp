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

//Code generator.

#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "action.h"
#include "cmdline.h"
#include "codegen.h"
#include "codegen_action.h"
#include "commons.h"
#include "concrete_lr.h"
#include "concretelrgen_res.h"
#include "concretescan.h"
#include "syn.h"
#include "descriptor.h"
#include "descriptor_type.h"
#include "types.h"
#include "util.h"
#include "util_mptr.h"
#include "util_string.h"

namespace ns = synbin;
namespace types = ns::types;
namespace util = ns::util;

using std::unique_ptr;

using util::MPtr;
using util::String;

//
//(Miscellaneous)
//

namespace {
	bool str_ends_with(const std::string& str, const std::string& suffix) {
		if (str.size() >= suffix.size()) {
			return str.find(suffix, str.size() - suffix.size()) != std::string::npos;
		}
		return false;
	}

	const std::string g_system_tokens[] = {
		std::string("SYS_ERROR"),
		std::string("SYS_EOF"),
		std::string("")
	};

	//TODO Implement character category check in a more platform-independent way.
	bool is_c_letter(char c) {
		return std::isalpha(c) || c == '_';
	}

	bool is_c_letter_or_digit(char c) {
		return std::isalnum(c) || c == '_';
	}

	std::string get_include_guard_name(const std::string& h_file_name) {
		std::string guard_name;
		
		for (std::size_t i = 0, n = h_file_name.size(); i < n; ++i) {
			char c = h_file_name[i];
			char validc = is_c_letter_or_digit(c) ? std::toupper(c) : '_';
			guard_name += validc;
		}
		
		guard_name += "_INCLUDED";
		return guard_name;
	}

}//namespace

//
//CodeGenerator : definition
//

namespace {
	typedef std::pair<const ns::ConcreteLRNt*, const ns::ConcreteLRState*> StartStatePair;
	typedef const std::vector<StartStatePair> StartStateVec;

	//
	//StateInfo
	//

	struct StateInfo {
		const ns::ConcreteLRState* m_state;
		std::size_t m_shift_ofs;
		std::size_t m_shift_count;
		std::size_t m_goto_ofs;
		std::size_t m_goto_count;
		std::size_t m_reduce_ofs;
		std::size_t m_reduce_count;

		StateInfo(
			const ns::ConcreteLRState* state,
			std::size_t shift_ofs,
			std::size_t shift_count,
			std::size_t goto_ofs,
			std::size_t goto_count,
			std::size_t reduce_ofs,
			std::size_t reduce_count)
			: m_state(state),
			m_shift_ofs(shift_ofs),
			m_shift_count(shift_count),
			m_goto_ofs(goto_ofs),
			m_goto_count(goto_count),
			m_reduce_ofs(reduce_ofs),
			m_reduce_count(reduce_count)
		{}
	};

	//
	//CodeGenerator
	//

	class CodeGenerator {
		NONCOPYABLE(CodeGenerator);

		const ns::CommandLine& m_command_line;
		const std::string& m_common_namespace;
		const std::string& m_code_namespace;
		const std::string& m_type_namespace;
		const std::string& m_class_namespace;
		const std::string& m_native_namespace;
		const std::string& m_allocator_name;

		const std::string& m_h_file_name;
		const std::string& m_c_file_name;

		const ns::ConcreteBNF* const m_bnf;
		const ns::ConcreteLRTables* const m_lr_tables;
		const std::vector<const ns::NtDescriptor*>* const m_nts;
		const std::vector<const ns::NameTrDescriptor*>* const m_name_tokens;
		const std::vector<const ns::StrTrDescriptor*>* const m_str_tokens;
		const std::vector<const ns::PrimitiveTypeDescriptor*>* const m_primitive_types;
		const ns::PrimitiveTypeDescriptor* const m_string_literal_type;

		std::vector<const ns::TrDescriptor*> m_all_tokens;

		std::size_t m_total_shift_count;
		std::size_t m_total_goto_count;
		std::size_t m_total_reduce_count;

		ns::ActionCodeGenerator m_action_generator;

	public:
		CodeGenerator(
			const ns::CommandLine& command_line,
			const ns::ConcreteLRResult* lr_result,
			const std::string& common_namespace,
			const std::string& code_namespace,
			const std::string& type_namespace,
			const std::string& class_namespace,
			const std::string& native_namespace,
			const std::string& h_file_name,
			const std::string& c_file_name,
			const std::string& allocator_name);

		void generate_result_files();

	private:
		void generate_tabs(std::ostream& out, std::size_t n);

		void generate_result_file(const std::string& file_name, void (CodeGenerator::*fn)(std::ostream&));
		
		void generate_h_file(std::ostream& out);

		void generate_includes_h(std::ostream& out);

		void generate_tokens_enum(std::ostream& out);
		void generate_token_descriptors_h(std::ostream& out);

		void generate_set_result_token(std::ostream& out, std::size_t level, const ns::StrTrDescriptor* token);
		void generate_concrete_scan_node(std::ostream& out, std::size_t level, const ns::ConcreteScanNode* node);
		void generate_scan_concrete_token(std::ostream& out);
		void generate_keyword_table_h(std::ostream& out);
		void generate_token_value(std::ostream& out);
		void generate_value_pool_h(std::ostream& out);
		void generate_syn_parser_h(std::ostream& out);
		void generate_parse_function(std::ostream& out, const ns::ConcreteLRNt* nt);

		void generate_cpp_file(std::ostream& out);

		void generate_includes_cpp(std::ostream& out);
		void generate_token_descriptors_cpp(std::ostream& out);
		void generate_keyword_table_cpp(std::ostream& out);
		void generate_value_allocation(std::ostream& out, const ns::PrimitiveTypeDescriptor* type);
		void generate_value_pool_cpp(std::ostream& out);
		void generate_typedefs_cpp(std::ostream& out);
		void generate_nonterminals_enum_cpp(std::ostream& out);
		void generate_tables_declaration_cpp(std::ostream& out);
		
		void collect_state_infos(std::vector<StateInfo>& vector);
		void generate_shifts_cpp(std::ostream& out, const std::vector<StateInfo>& states);
		void generate_gotos_cpp(std::ostream& out, const std::vector<StateInfo>& states);
		void generate_reduces_cpp(std::ostream& out, const std::vector<StateInfo>& states);
		void generate_states_cpp(std::ostream& out, const std::vector<StateInfo>& states);
		void generate_start_states_cpp(std::ostream& out);
		void generate_start_nt_functions_cpp(std::ostream& out);

		void generate_namespace_start(std::ostream& out, const std::string& name) const;
		void generate_namespace_end(std::ostream& out, const std::string& name) const;
	};
}//namespace

//
//CodeGenerator : implementation
//

CodeGenerator::CodeGenerator(
	const ns::CommandLine& command_line,
	const ns::ConcreteLRResult* lr_result,
	const std::string& common_namespace,
	const std::string& code_namespace,
	const std::string& type_namespace,
	const std::string& class_namespace,
	const std::string& native_namespace,
	const std::string& h_file_name,
	const std::string& c_file_name,
	const std::string& allocator_name)
	: m_command_line(command_line),
	m_common_namespace(common_namespace),
	m_code_namespace(code_namespace),
	m_type_namespace(type_namespace),
	m_class_namespace(class_namespace),
	m_native_namespace(native_namespace),
	m_h_file_name(h_file_name),
	m_c_file_name(c_file_name),
	m_allocator_name(allocator_name),
	m_bnf(lr_result->get_bnf_grammar()),
	m_lr_tables(lr_result->get_lr_tables()),
	m_nts(&lr_result->get_nts()),
	m_name_tokens(&lr_result->get_name_tokens()),
	m_str_tokens(&lr_result->get_str_tokens()),
	m_primitive_types(&lr_result->get_primitive_types()),
	m_string_literal_type(lr_result->get_string_literal_type().get()),
	m_action_generator(command_line, type_namespace, class_namespace, code_namespace, native_namespace, lr_result)
{
	m_all_tokens.insert(m_all_tokens.end(), m_name_tokens->begin(), m_name_tokens->end());
	m_all_tokens.insert(m_all_tokens.end(), m_str_tokens->begin(), m_str_tokens->end());
}

void CodeGenerator::generate_result_files() {
	//Create .h file.
	generate_result_file(m_h_file_name, &CodeGenerator::generate_h_file);

	//Create .cpp file.
	generate_result_file(m_c_file_name, &CodeGenerator::generate_cpp_file);
}

void CodeGenerator::generate_tabs(std::ostream& out, std::size_t n) {
	while (n) {
		out << "\t";
		--n;
	}
}

void CodeGenerator::generate_result_file(const std::string& file_name, void (CodeGenerator::*fn)(std::ostream&)) {
	std::ofstream out(file_name.c_str(), std::ios_base::out | std::ios_base::trunc);
	out.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
	
	(this->*fn)(out);
	
	out.close();
}

void CodeGenerator::generate_h_file(std::ostream& out) {
	const std::string guard_name = get_include_guard_name(m_h_file_name);
	out << "#ifndef " << guard_name << '\n';
	out << "#define " << guard_name << '\n';
	out << '\n';

	generate_includes_h(out);

	generate_namespace_start(out, m_code_namespace);
	out << '\n';

	generate_tokens_enum(out);
	generate_token_descriptors_h(out);
	generate_scan_concrete_token(out);
	generate_keyword_table_h(out);
	generate_token_value(out);
	generate_value_pool_h(out);
	generate_syn_parser_h(out);

	generate_namespace_end(out, m_code_namespace);
	out << '\n';

	out << "#endif//" << guard_name << '\n';
}

void CodeGenerator::generate_includes_h(std::ostream& out) {
	out << "#include \"syn.h\"" << '\n';

	const std::vector<ns::IncludeFile>& include_files = m_command_line.get_include_files();
	if (!include_files.empty()) {
		out << '\n';
		for (std::size_t i = 0, n = include_files.size(); i < n; ++i) {
			const ns::IncludeFile& file = include_files[i];
			out << "#include " << (file.is_system() ? "<" : "\"") << file.get_name() << (file.is_system() ? ">" : "\"") << '\n';
		}
	}

	out << '\n';
}

void CodeGenerator::generate_tokens_enum(std::ostream& out) {
	out << "\tstruct Tokens {" << '\n';
	out << "\t\tenum E {" << '\n';
	
	for (const std::string* p = g_system_tokens; !p->empty(); ++p) {
		bool last = m_all_tokens.empty() && (p+1)->empty();
		out << "\t\t\t" << (*p) << (last ? "" : ",") << '\n';
	}

	for (std::size_t i = 0, n = m_all_tokens.size(); i < n; ++i) {
		const ns::TrDescriptor* token = m_all_tokens[i];
		out << "\t\t\t";
		token->generate_constant_name(out);
		out << (i + 1 < n ? "," : "");
		token->generate_constant_comment(out);
		out << '\n';
	}

	out << "\t\t};" << '\n';
	out << "\t};" << '\n';
	out << '\n';
	out << "\ttypedef Tokens::E Token;" << '\n';
	out << '\n';
}

void CodeGenerator::generate_token_descriptors_h(std::ostream& out) {
	out << "\textern const syn::TokenDescriptor g_token_descriptors[];" << '\n';
	out << '\n';
}

void CodeGenerator::generate_set_result_token(std::ostream& out, std::size_t level, const ns::StrTrDescriptor* token) {
	generate_tabs(out, level + 2);
	out << "token = Tokens::";
	token->generate_constant_name(out);
	out << ";";
	token->generate_constant_comment(out);
	out << '\n';
}

void CodeGenerator::generate_concrete_scan_node(std::ostream& out, std::size_t level, const ns::ConcreteScanNode* node) {
	const ns::ConcreteScanNode::EdgeList& edges = node->get_edges();
	if (edges.empty()) {
		generate_set_result_token(out, level, node->get_token());
		return;
	}

	if (level > 0) {
		generate_tabs(out, level + 2);
		out << "c = end == ++cur ? 0 : Conv(*cur);" << '\n';
	}
		
	const char* sep = "";
	for (const ns::ConcreteScanEdge& edge : edges) {
		generate_tabs(out, level + 2);

		out << sep << "if ('" << edge.get_ch() << "' == c) {" << '\n';
		sep = "} else ";
			
		generate_concrete_scan_node(out, level + 1, edge.get_node());
	}

	generate_tabs(out, level + 2);
	out << "} else {" << '\n';
	if (node->get_token()) {
		generate_set_result_token(out, level + 1, node->get_token());
		generate_tabs(out, level + 3);
		out << "inc = false;\n";
	} else {
		generate_tabs(out, level + 3);
		out << "throw syn::SynLexicalError();" << '\n';
	}
			
	generate_tabs(out, level + 2);
	out << "}" << '\n';
}

void CodeGenerator::generate_scan_concrete_token(std::ostream& out) {
	unique_ptr<const ns::ConcreteScanNode> concrete_scan_tree = ns::build_concrete_scan_tree(*m_str_tokens);

	out << "\ttemplate<class Ch, class In, char Conv(Ch)>\n";
	out << "\tinline Tokens::E scan_concrete_token(In* cur_ref, const In end) {\n";
	out << "\t\tIn cur = *cur_ref;\n";
	out << "\t\tif (end == cur) {\n";
	out << "\t\t\tthrow syn::SynLexicalError();\n";
	out << "\t\t}\n";
	out << '\n';
	out << "\t\tchar c = Conv(*cur);\n";
	out << "\t\tbool inc = true;\n";
	out << '\n';
	out << "\t\tToken token;\n";

	generate_concrete_scan_node(out, 0, concrete_scan_tree.get());

	out << '\n';
	out << "\t\tif (inc) {\n";
	out << "\t\t\t++cur;\n";
	out << "\t\t}\n";
	out << "\t\t*cur_ref = cur;\n";
	out << "\t\treturn token;\n";
	out << "\t}\n";

	out << '\n';

	out << "\ttemplate<class In>\n";
	out << "\tinline Tokens::E scan_concrete_token_basic(In* cur_ref, const In end) {\n";
	out << "\t\treturn scan_concrete_token<char, In, syn::default_char_convertor<char>>(cur_ref, end);\n";
	out << "\t}\n";

	out << '\n';
}

void CodeGenerator::generate_keyword_table_h(std::ostream& out) {
	out << "\tstruct Keyword {\n";
	out << "\t\tconst std::string keyword;\n";
	out << "\t\tconst Token token;\n";
	out << "\t};\n";
	out << '\n';

	out << "\textern const Keyword g_keyword_table[];\n";
	out << '\n';
}

namespace {
	void generate_token_value_member(std::ostream& out, const types::PrimitiveType* type) {
		assert(!type->is_system());
		out << "v_" << type->get_name();
	}
}//namespace

void CodeGenerator::generate_token_value(std::ostream& out) {
	out << "\tstruct TokenValue {\n";
	for (const ns::PrimitiveTypeDescriptor* type : *m_primitive_types) {
		const types::PrimitiveType* type0 = type->get_primitive_type();
		if (!type0->is_system()) {
			out << "\t\t";
			m_action_generator.generate_primitive_type(out, type);
			out << " ";
			generate_token_value_member(out, type0);
			out << ";\n";
		}
	}
	out << "\t};\n";
	out << '\n';
}

void CodeGenerator::generate_value_pool_h(std::ostream& out) {
	out << "\tclass ValuePool {\n";
	out << "\t\tValuePool(const ValuePool&) = delete;\n";
	out << "\t\tValuePool(ValuePool&&) = delete;\n";
	out << "\t\tValuePool& operator=(const ValuePool&) = delete;\n";
	out << "\t\tValuePool& operator=(ValuePool&&) = delete;\n";
	out << '\n';

	//Members.
	for (const ns::PrimitiveTypeDescriptor* type : *m_primitive_types) {
		const types::PrimitiveType* type0 = type->get_primitive_type();
		out << "\t\tsyn::Pool<";
		m_action_generator.generate_primitive_type(out, type);
		out << "> ";
		m_action_generator.generate_value_pool_member_name(out, type0);
		out << ";\n";
	}
	out << '\n';

	//Constructor.
	out << "\tpublic:\n";
	out << "\t\tValuePool();\n";
	out << '\n';

	//General allocator.
	out << "\t\tconst void* allocate_value(syn::InternalTk token, const TokenValue& token_value);\n";
	out << '\n';

	//Typed allocators.
	for (const ns::PrimitiveTypeDescriptor* type : *m_primitive_types) {
		const types::PrimitiveType* type0 = type->get_primitive_type();
		out << "\t\t";
		m_action_generator.generate_primitive_type(out, type);
		out << "* ";
		m_action_generator.generate_value_pool_allocator_name(out, type0);
		out << "(const ";
		m_action_generator.generate_primitive_type(out, type);
		out << "& value);\n";
	}

	out << "\t};\n";
	out << '\n';
}

namespace {
	void generate_start_state_constant_name(std::ostream& out, const ns::ConcreteLRNt* nt) {
		const ns::UserNtDescriptor* nt_desc = nt->get_nt_obj().get()->as_user_nt();
		assert(nt_desc);
		out << "s_start_" << nt_desc->get_name();
	}
}//namesapce

void CodeGenerator::generate_syn_parser_h(std::ostream& out) {
	out << "\tclass SynParser {\n";
	out << "\t\ttypedef syn::State State;\n";
	out << "\t\ttypedef syn::StackElement_Nt StackNt;\n";
	out << "\t\ttypedef " << m_allocator_name << " ExAlloc;\n";
	out << '\n';

	const StartStateVec& start_states = m_lr_tables->get_start_states();
	
	//Start state constants.
	for (const StartStatePair& pair : start_states) {
		const ns::ConcreteLRNt* nt = pair.first;
		out << "\t\tstatic const State* const ";
		generate_start_state_constant_name(out, nt);
		out << ";\n";
	}
	out << '\n';

	//Start nonterminal functions.
	for (const StartStatePair& pair : start_states) {
		const ns::ConcreteLRNt* nt = pair.first;
		const ns::UserNtDescriptor* nt_desc = nt->get_nt_obj().get()->as_user_nt();
		assert(nt_desc);
		MPtr<const ns::TypeDescriptor> type = nt_desc->get_type();
		if (!type->is_void()) {
			out << "\t\tstatic ";
			m_action_generator.generate_type_external(out, type);
			out << " ";
			m_action_generator.generate_nonterminal_function_name(out, nt);
			out << "(StackNt* nt);\n";
		}
	}
	out << '\n';

	out << "\tpublic:\n";

	//Parse functions.
	for (std::size_t i = 0, n = start_states.size(); i < n; ++i) {
		if (i) out << '\n';
		const ns::ConcreteLRNt* nt = start_states[i].first;
		generate_parse_function(out, nt);
	}
	
	out << "\t};\n";
	out << '\n';
}

void CodeGenerator::generate_parse_function(std::ostream& out, const ns::ConcreteLRNt* nt) {
	const ns::UserNtDescriptor* nt_desc = nt->get_nt_obj().get()->as_user_nt();
	assert(nt_desc);

	out << "\t\ttemplate<class Scanner>\n";
	out << "\t\tstatic ";

	MPtr<const ns::TypeDescriptor> type = nt_desc->get_type();
	if (type->is_void()) {
		out << "void";
	} else {
		m_action_generator.generate_type_external(out, type);
	}

	out << " parse_" << nt_desc->get_name() << "(Scanner& scanner) {\n";
		
	out << "\t\t\tsyn::BasicSynParser<Scanner, ValuePool, TokenValue, Tokens::SYS_EOF> "
		<< "basic_parser(scanner);\n";

	out << "\t\t\tStackNt* root_nt = basic_parser.parse(";
	generate_start_state_constant_name(out, nt);
	out << ");\n";

	out << "\t\t\t";
	if (!type->is_void()) {
		out << "return ";
		m_action_generator.generate_nonterminal_function_name(out, nt);
		out << "(root_nt);\n";
	}

	out << "\t\t}\n";
}

void CodeGenerator::generate_cpp_file(std::ostream& out) {
	generate_includes_cpp(out);
	
	out << "namespace {\n";
	generate_typedefs_cpp(out);
	out << "}//namespace\n\n";

	generate_namespace_start(out, m_code_namespace);
	generate_nonterminals_enum_cpp(out);
	generate_tables_declaration_cpp(out);
	m_action_generator.generate_action_declarations(out);
	generate_namespace_end(out, m_code_namespace);
	out << '\n';

	generate_token_descriptors_cpp(out);
	generate_keyword_table_cpp(out);
	generate_value_pool_cpp(out);
	
	std::vector<StateInfo> state_infos;
	collect_state_infos(state_infos);

	generate_shifts_cpp(out, state_infos);
	generate_gotos_cpp(out, state_infos);
	generate_reduces_cpp(out, state_infos);
	generate_states_cpp(out, state_infos);
	generate_start_states_cpp(out);
	generate_start_nt_functions_cpp(out);

	m_action_generator.generate_actions(out);
}

void CodeGenerator::collect_state_infos(std::vector<StateInfo>& vector) {
	const std::vector<const ns::ConcreteLRState*>& states = m_lr_tables->get_states();
	std::size_t shift_ofs = 0;
	std::size_t goto_ofs = 0;
	std::size_t reduce_ofs = 0;

	for (const ns::ConcreteLRState* state : states) {
		std::size_t shift_count = state->get_shifts().size();
		std::size_t goto_count = state->get_gotos().size();
		std::size_t reduce_count = state->get_reduces().size();
		vector.push_back(StateInfo(state, shift_ofs, shift_count, goto_ofs, goto_count, reduce_ofs, reduce_count));
		shift_ofs += !shift_count ? 0 : shift_count + 1;
		goto_ofs += !goto_count ? 0 : goto_count + 1;
		reduce_ofs += !reduce_count ? 0 : reduce_count + 1;
	}

	m_total_shift_count = shift_ofs;
	m_total_goto_count = goto_ofs;
	m_total_reduce_count = reduce_ofs;
}

void CodeGenerator::generate_includes_cpp(std::ostream& out) {
	//Find out the name of the .h file.
	std::size_t name_pos = 0;
	for (std::size_t i = 0, n = m_h_file_name.size(); i < n; ++i) {
		char c = m_h_file_name[i];
		//TODO Avoid hardcoding of slash, it is implemendation-dependent.
		if ('/' == c || '\\' == c) {
			name_pos = i + 1;
		}
	}
	assert(name_pos < m_h_file_name.size());
	std::string name = m_h_file_name.substr(name_pos, m_h_file_name.size() - name_pos);
	
	//Write include(s).
	out << "#include \"" << name << "\"\n";
	out << '\n';
}

void CodeGenerator::generate_token_descriptors_cpp(std::ostream& out) {
	std::size_t n_tokens = m_all_tokens.size();
	for (const std::string* p = g_system_tokens; !p->empty(); ++p) ++n_tokens;

	out << "const syn::TokenDescriptor " << m_code_namespace << "::"
		<< "g_token_descriptors[" << n_tokens << "] = {\n";

	for (const std::string* p = g_system_tokens; !p->empty(); ++p) {
		bool last = m_all_tokens.empty() && (p+1)->empty();
		out << "\t{ std::string(\"" << (*p) << "\"), std::string(\"\") }" << (last ? "" : ",") << '\n';
	}

	for (std::size_t i = 0, n = m_all_tokens.size(); i < n; ++i) {
		const ns::TrDescriptor* token = m_all_tokens[i];
		out << "\t{ std::string(\"";
		token->generate_constant_name(out);
		out << "\"), std::string(\"";
		token->generate_token_str(out);
		out << "\") }" << (i + 1 < n ? "," : "");
		out << '\n';
	}

	out << "};\n";
	out << '\n';
}

namespace {
	bool compare_str_tokens(const ns::StrTrDescriptor* t1, const ns::StrTrDescriptor* t2) {
		return t1->get_str() < t2->get_str();
	}
}//namespace

void CodeGenerator::generate_keyword_table_cpp(std::ostream& out) {
	//Get keyword list.
	std::vector<const ns::StrTrDescriptor*> keywords;
	for (const ns::StrTrDescriptor* tr : *m_str_tokens) {
		if (tr->is_name()) keywords.push_back(tr);
	}
	std::sort(keywords.begin(), keywords.end(), &compare_str_tokens);

	//Generate the code.
	
	out << "const " << m_code_namespace << "::Keyword " << m_code_namespace
		<< "::g_keyword_table[" << (keywords.size() + 1) << "] = {\n";
	
	for (const ns::StrTrDescriptor* kw : keywords) {
		out << "\t{ std::string(\"" << kw->get_str() << "\"), " << m_code_namespace << "::Tokens::";
		kw->generate_constant_name(out);
		out << " },\n";
	}
	
	out << "\t{ std::string(\"\"), " << m_code_namespace << "::Token() }\n";
	
	out << "};\n";
	out << '\n';
}

void CodeGenerator::generate_value_allocation(std::ostream& out, const ns::PrimitiveTypeDescriptor* type) {
	const types::PrimitiveType* type0 = type->get_primitive_type();
	out << "\t\treturn ";
	m_action_generator.generate_value_pool_member_name(out, type0);
	out << ".allocate(token_value.";
	generate_token_value_member(out, type0);
	out << ");\n";
}

void CodeGenerator::generate_value_pool_cpp(std::ostream& out) {
	//Find typed tokens.
	std::vector<const ns::NameTrDescriptor*> tokens;
	for (const ns::NameTrDescriptor* tr : *m_name_tokens) {
		if (!!tr->get_type() && !tr->get_type()->is_void()) tokens.push_back(tr);
	}

	//Generate the code.

	out << "//\n";
	out << "//ValuePool\n";
	out << "//\n";
	out << '\n';
	
	//Constructor.
	out << m_code_namespace << "::ValuePool::ValuePool()\n";
	out << "{}\n";
	out << '\n';

	//Token value allocator.
	out << "const void* " << m_code_namespace << "::ValuePool::allocate_value("
		<< "syn::InternalTk token, const TokenValue& token_value) {\n";

	for (std::size_t i = 0, n = tokens.size(); i < n; ++i) {
		const ns::NameTrDescriptor* tr = tokens[i];
		const MPtr<const ns::TypeDescriptor> type = tr->get_type();
		const ns::PrimitiveTypeDescriptor* primitive_type = type->as_primitive_type();
		assert(primitive_type);

		out << "\t";
		if (i) out << "} else ";
		out << "if (Tokens::";
		tr->generate_constant_name(out);
		out << " == token) {\n";
		generate_value_allocation(out, primitive_type);
	}

	if (m_string_literal_type) {
		out << "\t";
		if (!tokens.empty()) out << "} else ";
		out << "if (token > Tokens::";
		tokens.back()->generate_constant_name(out);
		out << ") {\n";
		generate_value_allocation(out, m_string_literal_type);
	}

	out << "\t}\n";

	out << "\treturn nullptr;\n";
	out << "}\n";

	//Typed value allocators.
	for (const ns::PrimitiveTypeDescriptor* type : *m_primitive_types) {
		const types::PrimitiveType* type0 = type->get_primitive_type();
		out << "";
		m_action_generator.generate_primitive_type(out, type);
		out << "* " << m_code_namespace << "::ValuePool::";
		m_action_generator.generate_value_pool_allocator_name(out, type0);
		out << "(const ";
		m_action_generator.generate_primitive_type(out, type);
		out << "& value) {\n";

		out << "\treturn ";
		m_action_generator.generate_value_pool_member_name(out, type0);
		out << ".allocate(value);\n";

		out << "}\n";
		
		out << '\n';
	}
}

void CodeGenerator::generate_typedefs_cpp(std::ostream& out) {
	out << "\tusing syn::ProductionStack;\n";
	out << "\tusing " << m_code_namespace << "::Tokens;\n";
	out << "\tusing " << m_code_namespace << "::ValuePool;\n";
	out << "\tusing " << m_code_namespace << "::Token;\n";
	out << "\tusing syn::Shift;\n";
	out << "\tusing syn::Goto;\n";
	out << "\tusing syn::Reduce;\n";
	out << "\tusing syn::State;\n";
	out << '\n';
	out << "\ttypedef syn::StackElement StackEl;\n";
	out << "\ttypedef syn::StackElement_Nt StackNt;\n";
	out << "\ttypedef syn::StackElement_Value StackValue;\n";
	out << "\ttypedef syn::InternalAllocator InAlloc;\n";
	out << "\ttypedef " << m_allocator_name << " ExAlloc;\n";
	out << '\n';
}

void CodeGenerator::generate_nonterminals_enum_cpp(std::ostream& out) {
	out << "\tstruct Nts {\n";
	out << "\t\tenum E {\n";

	//TODO Sort enum constants by name.
	for (std::size_t i = 0, n = m_nts->size(); i < n; ++i) {
		const ns::NtDescriptor* nt = (*m_nts)[i];
		out << "\t\t\t" << nt->get_bnf_name() << (i + 1 < n ? "," : "") << '\n';
	}

	out << "\t\t};\n";
	out << "\t};\n";
	out << "\ttypedef Nts::E Nt;\n";
	out << '\n';
}

void CodeGenerator::generate_tables_declaration_cpp(std::ostream& out) {
	out << "\tstruct Tables {\n";
	out << "\t\tstatic const Shift shifts[];\n";
	out << "\t\tstatic const Goto gotos[];\n";
	out << "\t\tstatic const Reduce reduces[];\n";
	out << "\t\tstatic const State states[];\n";
	out << "\t};\n";
	out << '\n';
}

namespace {
	template<class T, class Handler>
	void generate_state_elements0(
		std::ostream& out,
		const Handler& handler,
		const std::vector<T>& elements)
	{
		for (const T& elem : elements) {
			out << "\t{ ";
			handler.generate_element(out, elem);
			out << " },";
			handler.generate_comment(out, elem);
			out << '\n';
		}
	}

	template<class T, class Handler>
	void generate_state_elements(
		std::ostream& out,
		const std::string& code_namespace,
		const std::vector<StateInfo>& states,
		std::size_t total_count,
		const char* element_type_name,
		const char* element_array_name,
		const Handler& handler)
	{
		out << "const " << element_type_name << " " << code_namespace << "::Tables::" << element_array_name << "[] = {\n";

		std::size_t ctr = 0;

		for (const StateInfo& state_info : states) {
			const ns::ConcreteLRState* state = state_info.m_state;
		
			const std::vector<T>& elements = handler.get_elements(state);
			if (elements.size()) {
				out << "\t//State " << state->get_index() << '\n';
				generate_state_elements0(out, handler, elements);
				ctr += elements.size();

				out << "\t{ ";
				handler.generate_terminator(out);
				out << " }" << (ctr < total_count ? "," : "") << '\n';
			}
		}
	
		out << "};\n";
		out << '\n';
	}
}//namespace

void CodeGenerator::generate_shifts_cpp(std::ostream& out, const std::vector<StateInfo>& states) {
	struct ShiftHandler {
		const std::vector<ns::ConcreteLRShift>& get_elements(const ns::ConcreteLRState* state) const {
			return state->get_shifts();
		}

		void generate_element(std::ostream& out, const ns::ConcreteLRShift& shift) const {
			MPtr<const ns::TrDescriptor> token = shift.get_tr()->get_tr_obj();
			out << "&states[" << shift.get_state()->get_index() << "], Tokens::";
			token->generate_constant_name(out);
		}

		void generate_comment(std::ostream& out, const ns::ConcreteLRShift& shift) const {
			MPtr<const ns::TrDescriptor> token = shift.get_tr()->get_tr_obj();
			token->generate_constant_comment(out);
		}

		void generate_terminator(std::ostream& out) const {
			out << "nullptr, Token()";
		}
	};

	generate_state_elements<ns::ConcreteLRShift>(
		out,
		m_code_namespace,
		states,
		m_total_shift_count,
		"Shift",
		"shifts",
		ShiftHandler());
}

void CodeGenerator::generate_gotos_cpp(std::ostream& out, const std::vector<StateInfo>& states) {
	struct GotoHandler {
		const std::vector<ns::ConcreteLRGoto>& get_elements(const ns::ConcreteLRState* state) const {
			return state->get_gotos();
		}

		void generate_element(std::ostream& out, const ns::ConcreteLRGoto& got) const {
			const ns::ConcreteLRNt* nt = got.get_nt();
			out << "&states[" << got.get_state()->get_index() << "], Nts::" << nt->get_name();
		}

		void generate_comment(std::ostream& out, const ns::ConcreteLRGoto& got) const {}

		void generate_terminator(std::ostream& out) const {
			out << "nullptr, Nt()";
		}
	};

	generate_state_elements<ns::ConcreteLRGoto>(
		out,
		m_code_namespace,
		states,
		m_total_goto_count,
		"Goto",
		"gotos",
		GotoHandler());
}

void CodeGenerator::generate_reduces_cpp(std::ostream& out, const std::vector<StateInfo>& states) {
	struct ReduceHandler {
		CodeGenerator* const m_generator;

		ReduceHandler(CodeGenerator* generator) : m_generator(generator){}

		const std::vector<const ns::ConcreteLRPr*>& get_elements(const ns::ConcreteLRState* state) const {
			return state->get_reduces();
		}

		void generate_element(std::ostream& out, const ns::ConcreteLRPr* reduce) const {
			if (reduce) {
				std::size_t length = reduce->get_elements().size();
				const ns::ConcreteLRNt* nt = reduce->get_nt();
				const ns::ActionInfo& action_info = m_generator->m_action_generator.get_action_info(reduce);
				
				std::size_t index = action_info.m_action_index;
				assert(syn::NULL_ACTION != index);
				assert(syn::ACCEPT_ACTION != index);
				syn::InternalAction action = index;
				
				out << length << ", Nts::" << nt->get_name() << ", " << "Actions::";
				m_generator->m_action_generator.generate_production_constant_name(out, action_info);
			} else {
				out << "0, Nt(), syn::ACCEPT_ACTION";
			}
		}

		void generate_comment(std::ostream& out, const ns::ConcreteLRPr* reduce) const {}

		void generate_terminator(std::ostream& out) const {
			out << "0, Nt(), syn::NULL_ACTION";
		}
	};

	generate_state_elements<const ns::ConcreteLRPr*>(
		out,
		m_code_namespace,
		states,
		m_total_reduce_count,
		"Reduce",
		"reduces",
		ReduceHandler(this));
}

namespace {
	void write_table_element_ptr(std::ostream& out, std::size_t count, std::size_t ofs, const char* table_name) {
		if (count) {
			out << "&" << table_name << "[" << ofs << "]";
		} else {
			out << "nullptr";
		}
	}

	const char* get_sym_type_str(const ns::ConcreteBNF::Sym* sym) {
		if (sym) {
			if (sym->as_nt()) return "sym_nt";

			const ns::ConcreteBNF::Tr* tr = sym->as_tr();
			assert(tr);
			MPtr<const ns::TypeDescriptor> type = tr->get_tr_obj()->get_type();
			if (!type->is_void()) return "sym_tk_value";
		}

		return "sym_none";
	}
}

void CodeGenerator::generate_states_cpp(std::ostream& out, const std::vector<StateInfo>& states) {
	out << "const State " << m_code_namespace << "::Tables::states[] = {\n";

	for (std::size_t i_state = 0, n_states = states.size(); i_state < n_states; ++i_state) {
		const StateInfo& info = states[i_state];
		const ns::ConcreteLRState* state = info.m_state;
		assert(state->get_index() == i_state);
		out << "\t{ " << i_state << ", ";
		
		write_table_element_ptr(out, info.m_shift_count, info.m_shift_ofs, "shifts");
		out << ", ";
		
		write_table_element_ptr(out, info.m_goto_count, info.m_goto_ofs, "gotos");
		out << ", ";
		
		write_table_element_ptr(out, info.m_reduce_count, info.m_reduce_ofs, "reduces");
		out << ", State::";

		const ns::ConcreteBNF::Sym* sym = state->get_sym();
		const char* sym_type_str = get_sym_type_str(sym);

		out << sym_type_str;
		out << " }" << (i_state + 1 < n_states ? "," : "");
		out << " //State " << i_state << '\n';
	}

	out << "};\n";
	out << '\n';
}

void CodeGenerator::generate_start_states_cpp(std::ostream& out) {
	const StartStateVec& start_states = m_lr_tables->get_start_states();
	
	for (const StartStatePair& pair : start_states) {
		const ns::ConcreteLRNt* nt = pair.first;
		out << "const State* const " << m_code_namespace << "::SynParser::";
		generate_start_state_constant_name(out, nt);
		out << " = &Tables::states[" << pair.second->get_index() << "];\n";
	}
	out << '\n';
}

void CodeGenerator::generate_start_nt_functions_cpp(std::ostream& out) {
	const StartStateVec& start_states = m_lr_tables->get_start_states();

	for (const StartStatePair& pair : start_states) {
		const ns::ConcreteLRNt* nt = pair.first;
		const ns::UserNtDescriptor* nt_desc = nt->get_nt_obj().get()->as_user_nt();
		assert(nt_desc);

		MPtr<const ns::TypeDescriptor> type = nt_desc->get_type();
		if (!type->is_void()) {
			m_action_generator.generate_type_external(out, type);
			out << m_code_namespace << "::SynParser::";
			m_action_generator.generate_nonterminal_function_name(out, nt);
			out << "(StackNt* nt) {\n";
			out << "\treturn ";
			
			bool conv = m_action_generator.generate_internal_to_external_conversion(out, type);
			out << (conv ? "(" : "");
			out << "Actions().";
			m_action_generator.generate_nonterminal_function_name(out, nt);
			out << "(nt)";
			out << (conv ? ")" : "");

			out << ";\n";
			out << "}\n\n";
		}
	}

	out << '\n';
}

namespace {
	//Splitting a namespace name into a vector of strings is not the most efficient solution,
	//but it must not have any significant influence on the overall performance.
	std::vector<std::string> split_namespace_name(const std::string& name) {
		std::vector<std::string> vec;

		const std::size_t len = name.length();
		std::size_t start_pos = 0;

		for (;;) {
			std::size_t find_pos = name.find("::", start_pos);
			std::size_t end_pos = find_pos == std::string::npos ? len : find_pos;
			vec.push_back(name.substr(start_pos, end_pos - start_pos));
			if (end_pos == len) break;
			start_pos = end_pos + 2;
		}

		return vec;
	}

	void format_namespace_tag(
		std::ostream& out,
		const std::vector<std::string>& name_parts,
		const char* prefix,
		const char* suffix)
	{
		for (const std::string& str : name_parts) out << prefix << str << suffix << '\n';
	}
}

void CodeGenerator::generate_namespace_start(std::ostream& out, const std::string& name) const {
	std::vector<std::string> name_parts = split_namespace_name(name);
	format_namespace_tag(out, name_parts, "namespace ", " {");
}

void CodeGenerator::generate_namespace_end(std::ostream& out, const std::string& name) const {
	std::vector<std::string> name_parts = split_namespace_name(name);
	std::reverse(name_parts.begin(), name_parts.end());
	format_namespace_tag(out, name_parts, "}//namespace ", "");
}

//
//generate_result_files()
//

namespace {
	void get_result_file_names(
		const ns::CommandLine& command_line,
		std::string* c_file_name,
		std::string* h_file_name)
	{
		const std::string c_suffix = ".cpp";
		const std::string h_suffix = ".h";

		const std::string& out_file_name = command_line.get_out_file();

		if (out_file_name.empty()) {
			const std::string base_file_name = "syngen";
			*c_file_name = base_file_name + c_suffix;
			*h_file_name = base_file_name + h_suffix;
		} else if (str_ends_with(out_file_name, c_suffix)) {
			*c_file_name = out_file_name;
			*h_file_name = out_file_name.substr(out_file_name.size() - c_suffix.size(), c_suffix.size()) + h_suffix;
		} else {
			*c_file_name = out_file_name + c_suffix;
			*h_file_name = out_file_name + h_suffix;
		}
	}

	const std::string& get_effective_str(const std::string& option_str, const std::string& default_str) {
		return !option_str.empty() ? option_str : default_str;
	}
}//namespace

void ns::generate_result_files(
	const CommandLine& command_line,
	unique_ptr<const ConcreteLRResult> lr_result)
{
	const std::string default_namespace = "syngen";
	const std::string& common_namespace = get_effective_str(command_line.get_namespace(), default_namespace);
	const std::string& code_namespace = get_effective_str(command_line.get_namespace_code(), common_namespace);
	const std::string& type_namespace = get_effective_str(command_line.get_namespace_types(), common_namespace);
	const std::string& class_namespace = get_effective_str(command_line.get_namespace_classes(), common_namespace);
	const std::string& native_namespace = get_effective_str(command_line.get_namespace_native(), common_namespace);

	std::string c_file_name;
	std::string h_file_name;
	get_result_file_names(command_line, &c_file_name, &h_file_name);

	const std::string default_allocator_name = "syn::ExternalAllocator";
	const std::string& allocator_name = get_effective_str(command_line.get_allocator(), default_allocator_name);

	CodeGenerator code_generator(
		command_line,
		lr_result.get(),
		common_namespace,
		code_namespace,
		type_namespace,
		class_namespace,
		native_namespace,
		h_file_name,
		c_file_name,
		allocator_name);
	
	code_generator.generate_result_files();
}
