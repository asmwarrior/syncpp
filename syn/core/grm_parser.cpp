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

//Grammar Parser implementation.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bnf.h"
#include "commons.h"
#include "syn.h"
#include "ebnf__imp.h"
#include "grm_parser.h"
#include "grm_parser_impl.h"
#include "lrtables.h"
#include "raw_bnf.h"
#include "util.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace ebnf = ns::ebnf;
namespace prs = ns::grm_parser;
namespace raw = ns::raw_bnf;
namespace util = ns::util;

using std::unique_ptr;

using util::MPtr;
using util::MContainer;
using util::MHeap;
using util::MRoot;

using syn::ProductionStack;

//
//GrammarParsingResult
//

ns::GrammarParsingResult::GrammarParsingResult(
	unique_ptr<MRoot<ebnf::Grammar>> grammar_root,
	unique_ptr<MHeap> const_heap)
	: m_grammar_root(std::move(grammar_root)),
	m_const_heap(std::move(const_heap))
{}

MPtr<ebnf::Grammar> ns::GrammarParsingResult::get_grammar() const {
	return m_grammar_root->ptr();
}

MHeap* ns::GrammarParsingResult::get_const_heap() const {
	return m_const_heap.get();
}

//
//(Parser code)
//

namespace {

	using syn::Shift;
	using syn::Goto;
	using syn::Reduce;
	using syn::State;

	class ActionContext;

	//typedef ActionResult (ActionContext::*SyntaxAction)(ProductionResult& pr);
	enum class SyntaxRule;

	//A derived class is used instead of typedef to avoid "decorated name length exceeded..." warning.
	class RawTraits : public ns::BnfTraits<raw::NullType, prs::Tokens::E, SyntaxRule>{};

	typedef ns::LRTables<RawTraits> LRTbl;
	typedef LRTbl::State LRState;
	typedef LRTbl::Shift LRShift;
	typedef LRTbl::Goto LRGoto;

	typedef raw::RawBnfParser<RawTraits> RawPrs;
	typedef RawPrs::RawTr RawTr;
	typedef RawPrs::RawRule RawRule;

	typedef ns::BnfGrammar<RawTraits> BnfGrm;

	//
	//SyntaxRule
	//

	enum class SyntaxRule {
		NONE,
		Grammar__DeclarationList,
		DeclarationList__Declaration,
		DeclarationList__DeclarationList_Declaration,
		Declaration__TypeDeclaration,
		Declaration__TerminalDeclaration,
		Declaration__NonterminalDeclaration,
		Declaration__CustomTerminalTypeDeclaration,
		TypeDeclaration__KWTYPE_NAME_CHSEMICOLON,
		TerminalDeclaration__KWTOKEN_NAME_TypeOpt_CHSEMICOLON,
		NonterminalDeclaration__AtOpt_NAME_TypeOpt_CHCOLON_SyntaxOrExpression_CHSEMICOLON,
		CustomTerminalTypeDeclaration__KWTOKEN_STRING_Type_CHSEMICOLON,
		AtOpt__CHAT,
		AtOpt__,
		TypeOpt__Type,
		TypeOpt__,
		Type__CHOBRACE_NAME_CHCBRACE,
		SyntaxOrExpression__SyntaxAndExpressionList,
		SyntaxAndExpressionList__SyntaxAndExpression,
		SyntaxAndExpressionList__SyntaxAndExpressionList_CHOR_SyntaxAndExpression,
		SyntaxAndExpression__SyntaxElementListOpt_TypeOpt,
		SyntaxElementListOpt__SyntaxElementList,
		SyntaxElementListOpt__,
		SyntaxElementList__SyntaxElement,
		SyntaxElementList__SyntaxElementList_SyntaxElement,
		SyntaxElement__NameSyntaxElement,
		SyntaxElement__ThisSyntaxElement,
		NameSyntaxElement__NAME_CHEQ_SyntaxTerm,
		NameSyntaxElement__SyntaxTerm,
		ThisSyntaxElement__KWTHIS_CHEQ_SyntaxTerm,
		SyntaxTerm__PrimarySyntaxTerm,
		SyntaxTerm__AdvanvedSyntaxTerm,
		PrimarySyntaxTerm__NameSyntaxTerm,
		PrimarySyntaxTerm__StringSyntaxTerm,
		PrimarySyntaxTerm__NestedSyntaxTerm,
		NameSyntaxTerm__NAME,
		StringSyntaxTerm__STRING,
		NestedSyntaxTerm__TypeOpt_CHOPAREN_SyntaxOrExpression_CHCPAREN,
		AdvanvedSyntaxTerm__ZeroOneSyntaxTerm,
		AdvanvedSyntaxTerm__ZeroManySyntaxTerm,
		AdvanvedSyntaxTerm__OneManySyntaxTerm,
		AdvanvedSyntaxTerm__ConstSyntaxTerm,
		ZeroOneSyntaxTerm__PrimarySyntaxTerm_CHQUESTION,
		ZeroManySyntaxTerm__LoopBody_CHASTERISK,
		OneManySyntaxTerm__LoopBody_CHPLUS,
		LoopBody__SimpleLoopBody,
		LoopBody__AdvancedLoopBody,
		SimpleLoopBody__PrimarySyntaxTerm,
		AdvancedLoopBody__CHOPAREN_SyntaxOrExpression_CHCOLON_SyntaxOrExpression_CHCPAREN,
		AdvancedLoopBody__CHOPAREN_SyntaxOrExpression_CHCPAREN,
		ConstSyntaxTerm__CHLT_ConstExpression_CHGT,
		ConstExpression__IntegerConstExpression,
		ConstExpression__StringConstExpression,
		ConstExpression__BooleanConstExpression,
		ConstExpression__NativeConstExpression,
		IntegerConstExpression__NUMBER,
		StringConstExpression__STRING,
		BooleanConstExpression__KWFALSE,
		BooleanConstExpression__KWTRUE,
		NativeConstExpression__NativeQualificationOpt_NativeName_NativeReferencesOpt,
		NativeQualificationOpt__NativeQualification,
		NativeQualificationOpt__,
		NativeQualification__NAME_CHCOLONCOLON,
		NativeQualification__NativeQualification_NAME_CHCOLONCOLON,
		NativeReferencesOpt__NativeReferences,
		NativeReferencesOpt__,
		NativeReferences__NativeReference,
		NativeReferences__NativeReferences_NativeReference,
		NativeName__NativeVariableName,
		NativeName__NativeFunctionName,
		NativeVariableName__NAME,
		NativeFunctionName__NAME_CHOPAREN_ConstExpressionListOpt_CHCPAREN,
		ConstExpressionListOpt__ConstExpressionList,
		ConstExpressionListOpt__,
		ConstExpressionList__ConstExpression,
		ConstExpressionList__ConstExpressionList_CHCOMMA_ConstExpression,
		NativeReference__CHDOT_NativeName,
		NativeReference__CHMINUSGT_NativeName,
		LAST
	};

	const syn::InternalAction ACTION_FIRST = static_cast<syn::InternalAction>(SyntaxRule::NONE);
	const syn::InternalAction ACTION_LAST = static_cast<syn::InternalAction>(SyntaxRule::LAST);

	//
	//ActionContext
	//

	class ActionContext {
		NONCOPYABLE(ActionContext);

		MHeap* const m_managed_heap;
		const MPtr<MContainer<ebnf::Object>> m_managed_container;
		MHeap* const m_const_managed_heap;
		const MPtr<MContainer<ebnf::Object>> m_const_managed_container;

		std::vector<const syn::StackElement*> m_stack_vector;

		template<class T>
		MPtr<T> manage(T* object) {
			return m_managed_container->add(object);
		}

		template<class T>
		MPtr<T> manage_spec(T* value) {
			return m_managed_heap->add_object(value);
		}

		template<class T>
		MPtr<T> manage_const(T* value) {
			return m_const_managed_container->add(value);
		}

		template<class T>
		MPtr<T> manage_const_spec(T* value) {
			return m_const_managed_heap->add_object(value);
		}

	private:
		static SyntaxRule syntax_rule(const syn::StackElement_Nt* nt) {
			syn::InternalAction action = nt->action();
			if (action <= ACTION_FIRST || action >= ACTION_LAST) {
				throw std::logic_error("syntax_rule(): Illegal state");
			}
			return static_cast<SyntaxRule>(action);
		}

		static std::exception illegal_state() {
			throw std::logic_error("illegal state");
		}

		static bool is_rule(const ProductionStack& stack, SyntaxRule rule, std::size_t len) {
			if (rule != syntax_rule(stack.get_nt())) return false;
			if (len != stack.size()) throw illegal_state();
			return true;
		}

		static void check_rule(const ProductionStack& stack, SyntaxRule rule, std::size_t len) {
			if (!is_rule(stack, rule, len)) throw illegal_state();
		}

		static ns::FilePos tk_pos(const syn::StackElement* node) {
			const syn::StackElement_Value* val = node->as_value();
			return *static_cast<const ns::FilePos*>(val->value());
		}

		static ns::syntax_number tk_number(const syn::StackElement* node) {
			const syn::StackElement_Value* val = node->as_value();
			return *static_cast<const ns::syntax_number*>(val->value());
		}

		static ns::syntax_string tk_string(const syn::StackElement* node) {
			const syn::StackElement_Value* val = node->as_value();
			return *static_cast<const ns::syntax_string*>(val->value());
		}

	public:
		ActionContext(MHeap* managed_heap, MHeap* const_managed_heap)
			: m_managed_heap(managed_heap),
			m_managed_container(managed_heap->create_container<ebnf::Object>()),
			m_const_managed_heap(const_managed_heap),
			m_const_managed_container(const_managed_heap->create_container<ebnf::Object>())
		{}

	private:
		typedef std::vector<MPtr<ebnf::Declaration>> DeclVector;
		typedef std::vector<MPtr<ebnf::SyntaxExpression>> SyntaxExprVector;
		typedef std::vector<MPtr<const ebnf::ConstExpression>> ConstExprVector;
		typedef std::vector<MPtr<const ebnf::NativeReference>> NativeRefVector;
		typedef std::vector<ns::syntax_string> StrVector;

		MPtr<ebnf::RawType> nt_Type(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::Type__CHOBRACE_NAME_CHCBRACE, 3);

			ns::syntax_string name = tk_string(stack[1]);
			return manage(new ebnf::RawType(name));
		}

		MPtr<ebnf::RawType> nt_TypeOpt(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::TypeOpt__Type == rule) {
				assert(1 == stack.size());
				return nt_Type(stack[0]);
			} else if (SyntaxRule::TypeOpt__ == rule) {
				assert(0 == stack.size());
				return MPtr<ebnf::RawType>();
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::IntegerConstExpression> nt_IntegerConstExpression(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::IntegerConstExpression__NUMBER, 1);

			const ns::syntax_number number = tk_number(stack[0]);
			return manage_const(new ebnf::IntegerConstExpression(number));
		}

		MPtr<ebnf::StringConstExpression> nt_StringConstExpression(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::StringConstExpression__STRING, 1);

			ns::syntax_string string = tk_string(stack[0]);
			return manage_const(new ebnf::StringConstExpression(string));
		}

		MPtr<ebnf::BooleanConstExpression> nt_BooleanConstExpression(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());

			bool value;
			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::BooleanConstExpression__KWTRUE == rule) {
				value = true;
			} else if (SyntaxRule::BooleanConstExpression__KWFALSE == rule) {
				value = false;
			} else {
				throw illegal_state();
			}

			return manage_const(new ebnf::BooleanConstExpression(value));
		}

		void nt_NativeQualification(const syn::StackElement* node, MPtr<StrVector> lst) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::NativeQualification__NAME_CHCOLONCOLON == rule) {
				assert(2 == stack.size());
				lst->push_back(tk_string(stack[0]));
			} else if (SyntaxRule::NativeQualification__NativeQualification_NAME_CHCOLONCOLON == rule) {
				assert(3 == stack.size());
				nt_NativeQualification(stack[0], lst);
				lst->push_back(tk_string(stack[1]));
			} else {
				throw illegal_state();
			}
		}

		MPtr<const StrVector> nt_NativeQualificationOpt(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);

			MPtr<StrVector> lst = manage_const_spec(new StrVector());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::NativeQualificationOpt__NativeQualification == rule) {
				assert(1 == stack.size());
				nt_NativeQualification(stack[0], lst);
			} else if (SyntaxRule::NativeQualificationOpt__ == rule) {
				assert(0 == stack.size());
			} else {
				throw illegal_state();
			}

			return lst;
		}

		MPtr<ebnf::NativeReference> nt_NativeReference(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::NativeReference__CHDOT_NativeName == rule) {
				assert(2 == stack.size());
				MPtr<const ebnf::NativeName> name = nt_NativeName(stack[1]);
				return manage_const(new ebnf::NativeReferenceReference(name));
			} else if (SyntaxRule::NativeReference__CHMINUSGT_NativeName == rule) {
				assert(2 == stack.size());
				MPtr<const ebnf::NativeName> name = nt_NativeName(stack[1]);
				return manage_const(new ebnf::NativePointerReference(name));
			} else {
				throw illegal_state();
			}
		}

		void nt_NativeReferences(const syn::StackElement* node, MPtr<NativeRefVector> lst) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			
			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::NativeReferences__NativeReference == rule) {
				assert(1 == stack.size());
				lst->push_back(nt_NativeReference(stack[0]));
			} else if (SyntaxRule::NativeReferences__NativeReferences_NativeReference == rule) {
				assert(3 == stack.size());
				nt_NativeReferences(stack[0], lst);
				lst->push_back(nt_NativeReference(stack[2]));
			} else {
				throw illegal_state();
			}
		}

		MPtr<const NativeRefVector> nt_NativeReferencesOpt(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);

			MPtr<NativeRefVector> lst = manage_const_spec(new NativeRefVector());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::NativeReferencesOpt__NativeReferences == rule) {
				assert(1 == stack.size());
				nt_NativeReferences(stack[0], lst);
			} else if (SyntaxRule::NativeReferencesOpt__ == rule) {
				assert(0 == stack.size());
			} else {
				throw illegal_state();
			}

			return lst;
		}

		MPtr<ebnf::NativeVariableName> nt_NativeVariableName(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::NativeVariableName__NAME, 1);

			const ns::syntax_string name = tk_string(stack[0]);
			return manage_const(new ebnf::NativeVariableName(name));
		}

		MPtr<ebnf::NativeFunctionName> nt_NativeFunctionName(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::NativeFunctionName__NAME_CHOPAREN_ConstExpressionListOpt_CHCPAREN, 4);

			const ns::syntax_string name = tk_string(stack[0]);
			MPtr<const ConstExprVector> expressions = nt_ConstExpressionListOpt(stack[2]);
			return manage_const(new ebnf::NativeFunctionName(name, expressions));
		}

		MPtr<ebnf::NativeName> nt_NativeName(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::NativeName__NativeVariableName == rule) {
				return nt_NativeVariableName(stack[0]);
			} else if (SyntaxRule::NativeName__NativeFunctionName == rule) {
				return nt_NativeFunctionName(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		void nt_ConstExpressionList(const syn::StackElement* node, MPtr<ConstExprVector> lst) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			
			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::ConstExpressionList__ConstExpression == rule) {
				assert(1 == stack.size());
				lst->push_back(nt_ConstExpression(stack[0]));
			} else if (SyntaxRule::ConstExpressionList__ConstExpressionList_CHCOMMA_ConstExpression == rule) {
				assert(3 == stack.size());
				nt_ConstExpressionList(stack[0], lst);
				lst->push_back(nt_ConstExpression(stack[2]));
			} else {
				throw illegal_state();
			}
		}

		MPtr<const ConstExprVector> nt_ConstExpressionListOpt(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			
			MPtr<ConstExprVector> lst = manage_const_spec(new ConstExprVector());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::ConstExpressionListOpt__ConstExpressionList == rule) {
				assert(1 == stack.size());
				nt_ConstExpressionList(stack[0], lst);
			} else if (SyntaxRule::ConstExpressionListOpt__ == rule) {
				assert(0 == stack.size());
			} else {
				throw illegal_state();
			}

			return lst;
		}

		MPtr<ebnf::NativeConstExpression> nt_NativeConstExpression(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::NativeConstExpression__NativeQualificationOpt_NativeName_NativeReferencesOpt, 3);

			MPtr<const StrVector> qualifications = nt_NativeQualificationOpt(stack[0]);
			MPtr<const ebnf::NativeName> name = nt_NativeName(stack[1]);
			MPtr<const NativeRefVector> references = nt_NativeReferencesOpt(stack[2]);
			return manage_const(new ebnf::NativeConstExpression(qualifications, name, references));
		}

		MPtr<ebnf::ConstExpression> nt_ConstExpression(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::ConstExpression__IntegerConstExpression == rule) {
				return nt_IntegerConstExpression(stack[0]);
			} else if (SyntaxRule::ConstExpression__StringConstExpression == rule) {
				return nt_StringConstExpression(stack[0]);
			} else if (SyntaxRule::ConstExpression__BooleanConstExpression == rule) {
				return nt_BooleanConstExpression(stack[0]);
			} else if (SyntaxRule::ConstExpression__NativeConstExpression == rule) {
				return nt_NativeConstExpression(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::NameSyntaxExpression> nt_NameSyntaxTerm(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::NameSyntaxTerm__NAME, 1);

			ns::syntax_string name = tk_string(stack[0]);
			return manage(new ebnf::NameSyntaxExpression(name));
		}

		MPtr<ebnf::StringSyntaxExpression> nt_StringSyntaxTerm(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::StringSyntaxTerm__STRING, 1);

			ns::syntax_string name = tk_string(stack[0]);
			return manage(new ebnf::StringSyntaxExpression(name));
		}

		MPtr<ebnf::SyntaxExpression> nt_NestedSyntaxTerm(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::NestedSyntaxTerm__TypeOpt_CHOPAREN_SyntaxOrExpression_CHCPAREN, 4);

			MPtr<const ebnf::RawType> type = nt_TypeOpt(stack[0]);
			MPtr<ebnf::SyntaxExpression> expression = nt_SyntaxOrExpression(stack[2]);
			if (type.get()) expression = manage(new ebnf::CastSyntaxExpression(type, expression));
			return expression;
		}

		MPtr<ebnf::SyntaxExpression> nt_PrimarySyntaxTerm(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::PrimarySyntaxTerm__NameSyntaxTerm == rule) {
				return nt_NameSyntaxTerm(stack[0]);
			} else if (SyntaxRule::PrimarySyntaxTerm__StringSyntaxTerm == rule) {
				return nt_StringSyntaxTerm(stack[0]);
			} else if (SyntaxRule::PrimarySyntaxTerm__NestedSyntaxTerm == rule) {
				return nt_NestedSyntaxTerm(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::SyntaxExpression> nt_ZeroOneSyntaxTerm(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::ZeroOneSyntaxTerm__PrimarySyntaxTerm_CHQUESTION, 2);

			MPtr<ebnf::SyntaxExpression> sub_expression = nt_PrimarySyntaxTerm(stack[0]);
			return manage(new ebnf::ZeroOneSyntaxExpression(sub_expression));
		}

		MPtr<ebnf::LoopBody> nt_SimpleLoopBody(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::SimpleLoopBody__PrimarySyntaxTerm, 1);

			MPtr<ebnf::SyntaxExpression> expression = nt_PrimarySyntaxTerm(stack[0]);
			return manage(new ebnf::LoopBody(expression, MPtr<ebnf::SyntaxExpression>(), ns::FilePos()));
		}

		MPtr<ebnf::LoopBody> nt_AdvancedLoopBody(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::AdvancedLoopBody__CHOPAREN_SyntaxOrExpression_CHCOLON_SyntaxOrExpression_CHCPAREN == rule) {
				assert(5 == stack.size());
				MPtr<ebnf::SyntaxExpression> expression = nt_SyntaxOrExpression(stack[1]);
				ns::FilePos separator_pos = tk_pos(stack[2]);
				MPtr<ebnf::SyntaxExpression> separator = nt_SyntaxOrExpression(stack[3]);
				return manage(new ebnf::LoopBody(expression, separator, separator_pos));
			} else if (SyntaxRule::AdvancedLoopBody__CHOPAREN_SyntaxOrExpression_CHCPAREN == rule) {
				assert(3 == stack.size());
				MPtr<ebnf::SyntaxExpression> expression = nt_SyntaxOrExpression(stack[1]);
				return manage(new ebnf::LoopBody(expression, MPtr<ebnf::SyntaxExpression>(), ns::FilePos()));
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::LoopBody> nt_LoopBody(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::LoopBody__SimpleLoopBody == rule) {
				return nt_SimpleLoopBody(stack[0]);
			} else if (SyntaxRule::LoopBody__AdvancedLoopBody == rule) {
				return nt_AdvancedLoopBody(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::ZeroManySyntaxExpression> nt_ZeroManySyntaxTerm(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::ZeroManySyntaxTerm__LoopBody_CHASTERISK, 2);

			MPtr<ebnf::LoopBody> body = nt_LoopBody(stack[0]);
			return manage(new ebnf::ZeroManySyntaxExpression(body));
		}

		MPtr<ebnf::OneManySyntaxExpression> nt_OneManySyntaxTerm(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::OneManySyntaxTerm__LoopBody_CHPLUS, 2);

			MPtr<ebnf::LoopBody> body = nt_LoopBody(stack[0]);
			return manage(new ebnf::OneManySyntaxExpression(body));
		}

		MPtr<ebnf::ConstSyntaxExpression> nt_ConstSyntaxTerm(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::ConstSyntaxTerm__CHLT_ConstExpression_CHGT, 3);

			MPtr<const ebnf::ConstExpression> const_expression = nt_ConstExpression(stack[1]);
			return manage(new ebnf::ConstSyntaxExpression(const_expression));
		}

		MPtr<ebnf::SyntaxExpression> nt_AdvanvedSyntaxTerm(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::AdvanvedSyntaxTerm__ZeroOneSyntaxTerm == rule) {
				return nt_ZeroOneSyntaxTerm(stack[0]);
			} else if (SyntaxRule::AdvanvedSyntaxTerm__ZeroManySyntaxTerm == rule) {
				return nt_ZeroManySyntaxTerm(stack[0]);
			} else if (SyntaxRule::AdvanvedSyntaxTerm__OneManySyntaxTerm == rule) {
				return nt_OneManySyntaxTerm(stack[0]);
			} else if (SyntaxRule::AdvanvedSyntaxTerm__ConstSyntaxTerm == rule) {
				return nt_ConstSyntaxTerm(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::SyntaxExpression> nt_SyntaxTerm(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::SyntaxTerm__PrimarySyntaxTerm == rule) {
				return nt_PrimarySyntaxTerm(stack[0]);
			} else if (SyntaxRule::SyntaxTerm__AdvanvedSyntaxTerm == rule) {
				return nt_AdvanvedSyntaxTerm(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::SyntaxExpression> nt_NameSyntaxElement(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::NameSyntaxElement__NAME_CHEQ_SyntaxTerm == rule) {
				assert(3 == stack.size());
				const ns::syntax_string name = tk_string(stack[0]);
				MPtr<ebnf::SyntaxExpression> expression = nt_SyntaxTerm(stack[2]);
				return manage(new ebnf::NameSyntaxElement(expression, name));
			} else if (SyntaxRule::NameSyntaxElement__SyntaxTerm == rule) {
				assert(1 == stack.size());
				return nt_SyntaxTerm(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::ThisSyntaxElement> nt_ThisSyntaxElement(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::ThisSyntaxElement__KWTHIS_CHEQ_SyntaxTerm, 3);

			ns::FilePos pos = tk_pos(stack[0]);
			MPtr<ebnf::SyntaxExpression> expression = nt_SyntaxTerm(stack[2]);
			return manage(new ebnf::ThisSyntaxElement(pos, expression));
		}

		MPtr<ebnf::SyntaxExpression> nt_SyntaxElement(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::SyntaxElement__NameSyntaxElement == rule) {
				return nt_NameSyntaxElement(stack[0]);
			} else if (SyntaxRule::SyntaxElement__ThisSyntaxElement == rule) {
				return nt_ThisSyntaxElement(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		void nt_SyntaxElementList(const syn::StackElement* node, MPtr<SyntaxExprVector> lst) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			
			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::SyntaxElementList__SyntaxElement == rule) {
				assert(1 == stack.size());
				lst->push_back(nt_SyntaxElement(stack[0]));
			} else if (SyntaxRule::SyntaxElementList__SyntaxElementList_SyntaxElement == rule) {
				assert(2 == stack.size());
				nt_SyntaxElementList(stack[0], lst);
				lst->push_back(nt_SyntaxElement(stack[1]));
			} else {
				throw illegal_state();
			}
		}

		MPtr<SyntaxExprVector> nt_SyntaxElementListOpt(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			
			MPtr<SyntaxExprVector> lst = manage_const_spec(new SyntaxExprVector());

			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::SyntaxElementListOpt__SyntaxElementList == rule) {
				assert(1 == stack.size());
				nt_SyntaxElementList(stack[0], lst);
			} else if (SyntaxRule::SyntaxElementListOpt__ == rule) {
				assert(0 == stack.size());
			} else {
				throw illegal_state();
			}

			return lst;
		}

		MPtr<ebnf::SyntaxExpression> nt_SyntaxAndExpression(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::SyntaxAndExpression__SyntaxElementListOpt_TypeOpt, 2);

			MPtr<SyntaxExprVector> expressions = nt_SyntaxElementListOpt(stack[0]);
			MPtr<const ebnf::RawType> type = nt_TypeOpt(stack[1]);

			MPtr<ebnf::SyntaxExpression> expression;
			if (expressions->empty() && !type.get()) {
				expression = manage(new ebnf::EmptySyntaxExpression());
			} else if (1 == expressions->size() && !type.get()) {
				SyntaxExprVector& vector = *expressions;
				MPtr<ebnf::SyntaxExpression> expr = (*expressions)[0];
				expression = (*expressions)[0];
				expressions->clear();
			} else {
				expression = manage(new ebnf::SyntaxAndExpression(expressions, type));
			}

			return expression;
		}

		void nt_SyntaxAndExpressionList(const syn::StackElement* node, MPtr<SyntaxExprVector> lst) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			
			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::SyntaxAndExpressionList__SyntaxAndExpression == rule) {
				assert(1 == stack.size());
				lst->push_back(nt_SyntaxAndExpression(stack[0]));
			} else if (SyntaxRule::SyntaxAndExpressionList__SyntaxAndExpressionList_CHOR_SyntaxAndExpression == rule) {
				assert(3 == stack.size());
				nt_SyntaxAndExpressionList(stack[0], lst);
				lst->push_back(nt_SyntaxAndExpression(stack[2]));
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::SyntaxExpression> nt_SyntaxOrExpression(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::SyntaxOrExpression__SyntaxAndExpressionList, 1);

			MPtr<SyntaxExprVector> expressions = manage_const_spec(new SyntaxExprVector());
			nt_SyntaxAndExpressionList(stack[0], expressions);

			MPtr<ebnf::SyntaxExpression> expression;
			if (expressions->empty()) {
				expression = manage(new ebnf::EmptySyntaxExpression());
			} else if (1 == expressions->size()) {
				expression = (*expressions)[0];
				expressions->clear();
			} else {
				expression = manage(new ebnf::SyntaxOrExpression(expressions));
			}

			return expression;
		}

		MPtr<ebnf::TypeDeclaration> nt_TypeDeclaration(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::TypeDeclaration__KWTYPE_NAME_CHSEMICOLON, 3);

			const ns::syntax_string name = tk_string(stack[1]);
			return manage(new ebnf::TypeDeclaration(name));
		}

		MPtr<ebnf::TerminalDeclaration> nt_TerminalDeclaration(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::TerminalDeclaration__KWTOKEN_NAME_TypeOpt_CHSEMICOLON, 4);

			const ns::syntax_string name = tk_string(stack[1]);
			MPtr<ebnf::RawType> raw_type = nt_TypeOpt(stack[2]);
			return manage(new ebnf::TerminalDeclaration(name, raw_type));
		}

		bool nt_AtOpt(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			
			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::AtOpt__CHAT == rule) {
				assert(1 == stack.size());
				return true;
			} else if (SyntaxRule::AtOpt__ == rule) {
				assert(0 == stack.size());
				return false;
			} else {
				throw illegal_state();
			}
		}

		MPtr<ebnf::NonterminalDeclaration> nt_NonterminalDeclaration(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::NonterminalDeclaration__AtOpt_NAME_TypeOpt_CHCOLON_SyntaxOrExpression_CHSEMICOLON, 6);

			bool start = nt_AtOpt(stack[0]);
			const ns::syntax_string name = tk_string(stack[1]);
			MPtr<const ebnf::RawType> type = nt_TypeOpt(stack[2]);
			MPtr<ebnf::SyntaxExpression> expression = nt_SyntaxOrExpression(stack[4]);
			return manage(new ebnf::NonterminalDeclaration(start, name, expression, type));
		}

		MPtr<ebnf::Declaration> nt_CustomTerminalTypeDeclaration(const syn::StackElement* node) {
			ProductionStack stack(m_stack_vector, node);
			check_rule(stack, SyntaxRule::CustomTerminalTypeDeclaration__KWTOKEN_STRING_Type_CHSEMICOLON, 4);

			const ns::syntax_string str = tk_string(stack[1]);
			MPtr<ebnf::RawType> raw_type = nt_Type(stack[2]);

			if (str.get_string().str() != "") {
				throw prs::ParserException("Empty string literal is expected", str.pos());
			}

			return manage(new ebnf::CustomTerminalTypeDeclaration(raw_type));
		}

		MPtr<ebnf::Declaration> nt_Declaration(const syn::StackElement* node) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			assert(1 == stack.size());
			
			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::Declaration__TypeDeclaration == rule) {
				return nt_TypeDeclaration(stack[0]);
			} else if (SyntaxRule::Declaration__TerminalDeclaration == rule) {
				return nt_TerminalDeclaration(stack[0]);
			} else if (SyntaxRule::Declaration__NonterminalDeclaration == rule) {
				return nt_NonterminalDeclaration(stack[0]);
			} else if (SyntaxRule::Declaration__CustomTerminalTypeDeclaration == rule) {
				return nt_CustomTerminalTypeDeclaration(stack[0]);
			} else {
				throw illegal_state();
			}
		}

		void nt_DeclarationList(const syn::StackElement* node, MPtr<DeclVector> lst) {
			const syn::StackElement_Nt* nt = node->as_nt();
			ProductionStack stack(m_stack_vector, nt);
			
			const SyntaxRule rule = syntax_rule(nt);
			if (SyntaxRule::DeclarationList__Declaration == rule) {
				assert(1 == stack.size());
				lst->push_back(nt_Declaration(stack[0]));
			} else if (SyntaxRule::DeclarationList__DeclarationList_Declaration == rule) {
				assert(2 == stack.size());
				nt_DeclarationList(stack[0], lst);
				lst->push_back(nt_Declaration(stack[1]));
			} else {
				throw illegal_state();
			}
		}

	public:
		MPtr<ebnf::Grammar> nt_Grammar(const syn::StackElement_Nt* nt) {
			ProductionStack stack(m_stack_vector, nt);
			check_rule(stack, SyntaxRule::Grammar__DeclarationList, 1);

			MPtr<DeclVector> declarations = manage_const_spec(new DeclVector());
			nt_DeclarationList(stack[0], declarations);
			return manage(new ebnf::Grammar(declarations));
		}
	};

	const RawTr g_raw_tokens[] = {
		{ "NAME", prs::Tokens::NAME },
		{ "NUMBER", prs::Tokens::NUMBER },
		{ "STRING", prs::Tokens::STRING },
		{ "KW_CLASS", prs::Tokens::KW_CLASS },
		{ "KW_THIS", prs::Tokens::KW_THIS },
		{ "KW_TOKEN", prs::Tokens::KW_TOKEN },
		{ "KW_TYPE", prs::Tokens::KW_TYPE },
		{ "KW_FALSE", prs::Tokens::KW_FALSE },
		{ "KW_TRUE", prs::Tokens::KW_TRUE },
		{ "CH_SEMICOLON", prs::Tokens::CH_SEMICOLON },
		{ "CH_AT", prs::Tokens::CH_AT },
		{ "CH_COLON", prs::Tokens::CH_COLON },
		{ "CH_OBRACE", prs::Tokens::CH_OBRACE },
		{ "CH_CBRACE", prs::Tokens::CH_CBRACE },
		{ "CH_OR", prs::Tokens::CH_OR },
		{ "CH_EQ", prs::Tokens::CH_EQ },
		{ "CH_OPAREN", prs::Tokens::CH_OPAREN },
		{ "CH_CPAREN", prs::Tokens::CH_CPAREN },
		{ "CH_QUESTION", prs::Tokens::CH_QUESTION },
		{ "CH_ASTERISK", prs::Tokens::CH_ASTERISK },
		{ "CH_PLUS", prs::Tokens::CH_PLUS },
		{ "CH_LT", prs::Tokens::CH_LT },
		{ "CH_GT", prs::Tokens::CH_GT },
		{ "CH_COLON_COLON", prs::Tokens::CH_COLON_COLON },
		{ "CH_COMMA", prs::Tokens::CH_COMMA },
		{ "CH_DOT", prs::Tokens::CH_DOT },
		{ "CH_MINUS_GT", prs::Tokens::CH_MINUS_GT },
		{ 0, prs::Tokens::E(0) }
	};

	const RawRule g_raw_rules[] =
	{
		{ "Grammar", SyntaxRule::NONE },
		{ "DeclarationList", SyntaxRule::Grammar__DeclarationList },
		
		{ "DeclarationList", SyntaxRule::NONE },
		{ "Declaration", SyntaxRule::DeclarationList__Declaration },
		{ "DeclarationList Declaration", SyntaxRule::DeclarationList__DeclarationList_Declaration },
		
		{ "Declaration", SyntaxRule::NONE },
		{ "TypeDeclaration", SyntaxRule::Declaration__TypeDeclaration },
		{ "TerminalDeclaration", SyntaxRule::Declaration__TerminalDeclaration },
		{ "NonterminalDeclaration", SyntaxRule::Declaration__NonterminalDeclaration },
		{ "CustomTerminalTypeDeclaration", SyntaxRule::Declaration__CustomTerminalTypeDeclaration },

		{ "TypeDeclaration", SyntaxRule::NONE },
		{ "KW_TYPE NAME CH_SEMICOLON", SyntaxRule::TypeDeclaration__KWTYPE_NAME_CHSEMICOLON },
		
		{ "TerminalDeclaration", SyntaxRule::NONE },
		{ "KW_TOKEN NAME TypeOpt CH_SEMICOLON", SyntaxRule::TerminalDeclaration__KWTOKEN_NAME_TypeOpt_CHSEMICOLON },
		
		{ "NonterminalDeclaration", SyntaxRule::NONE },
		{ "AtOpt NAME TypeOpt CH_COLON SyntaxOrExpression CH_SEMICOLON",
			SyntaxRule::NonterminalDeclaration__AtOpt_NAME_TypeOpt_CHCOLON_SyntaxOrExpression_CHSEMICOLON },

		{ "CustomTerminalTypeDeclaration", SyntaxRule::NONE },
		{ "KW_TOKEN STRING Type CH_SEMICOLON", SyntaxRule::CustomTerminalTypeDeclaration__KWTOKEN_STRING_Type_CHSEMICOLON },

		{ "AtOpt", SyntaxRule::NONE },
		{ "CH_AT", SyntaxRule::AtOpt__CHAT },
		{ "", SyntaxRule::AtOpt__ },
		
		{ "TypeOpt", SyntaxRule::NONE },
		{ "Type", SyntaxRule::TypeOpt__Type },
		{ "", SyntaxRule::TypeOpt__ },

		{ "Type", SyntaxRule::NONE },
		{ "CH_OBRACE NAME CH_CBRACE", SyntaxRule::Type__CHOBRACE_NAME_CHCBRACE },

		{ "SyntaxOrExpression", SyntaxRule::NONE },
		{ "SyntaxAndExpressionList", SyntaxRule::SyntaxOrExpression__SyntaxAndExpressionList },
		
		{ "SyntaxAndExpressionList", SyntaxRule::NONE },
		{ "SyntaxAndExpression", SyntaxRule::SyntaxAndExpressionList__SyntaxAndExpression },
		{ "SyntaxAndExpressionList CH_OR SyntaxAndExpression",
			SyntaxRule::SyntaxAndExpressionList__SyntaxAndExpressionList_CHOR_SyntaxAndExpression },
		
		{ "SyntaxAndExpression", SyntaxRule::NONE },
		{ "SyntaxElementListOpt TypeOpt", SyntaxRule::SyntaxAndExpression__SyntaxElementListOpt_TypeOpt },
		
		{ "SyntaxElementListOpt", SyntaxRule::NONE },
		{ "SyntaxElementList", SyntaxRule::SyntaxElementListOpt__SyntaxElementList },
		{ "", SyntaxRule::SyntaxElementListOpt__ },

		{ "SyntaxElementList", SyntaxRule::NONE },
		{ "SyntaxElement", SyntaxRule::SyntaxElementList__SyntaxElement },
		{ "SyntaxElementList SyntaxElement", SyntaxRule::SyntaxElementList__SyntaxElementList_SyntaxElement },
		
		{ "SyntaxElement", SyntaxRule::NONE },
		{ "NameSyntaxElement", SyntaxRule::SyntaxElement__NameSyntaxElement },
		{ "ThisSyntaxElement", SyntaxRule::SyntaxElement__ThisSyntaxElement },
		
		{ "NameSyntaxElement", SyntaxRule::NONE },
		{ "NAME CH_EQ SyntaxTerm", SyntaxRule::NameSyntaxElement__NAME_CHEQ_SyntaxTerm },
		{ "SyntaxTerm", SyntaxRule::NameSyntaxElement__SyntaxTerm },
		
		{ "ThisSyntaxElement", SyntaxRule::NONE },
		{ "KW_THIS CH_EQ SyntaxTerm", SyntaxRule::ThisSyntaxElement__KWTHIS_CHEQ_SyntaxTerm },
		
		{ "SyntaxTerm", SyntaxRule::NONE },
		{ "PrimarySyntaxTerm", SyntaxRule::SyntaxTerm__PrimarySyntaxTerm },
		{ "AdvanvedSyntaxTerm", SyntaxRule::SyntaxTerm__AdvanvedSyntaxTerm },
		
		{ "PrimarySyntaxTerm", SyntaxRule::NONE },
		{ "NameSyntaxTerm", SyntaxRule::PrimarySyntaxTerm__NameSyntaxTerm },
		{ "StringSyntaxTerm", SyntaxRule::PrimarySyntaxTerm__StringSyntaxTerm },
		{ "NestedSyntaxTerm", SyntaxRule::PrimarySyntaxTerm__NestedSyntaxTerm },
		
		{ "NameSyntaxTerm", SyntaxRule::NONE },
		{ "NAME", SyntaxRule::NameSyntaxTerm__NAME },
		
		{ "StringSyntaxTerm", SyntaxRule::NONE },
		{ "STRING", SyntaxRule::StringSyntaxTerm__STRING },
		
		{ "NestedSyntaxTerm", SyntaxRule::NONE },
		{ "TypeOpt CH_OPAREN SyntaxOrExpression CH_CPAREN",
			SyntaxRule::NestedSyntaxTerm__TypeOpt_CHOPAREN_SyntaxOrExpression_CHCPAREN },
		
		{ "AdvanvedSyntaxTerm", SyntaxRule::NONE },
		{ "ZeroOneSyntaxTerm", SyntaxRule::AdvanvedSyntaxTerm__ZeroOneSyntaxTerm },
		{ "ZeroManySyntaxTerm", SyntaxRule::AdvanvedSyntaxTerm__ZeroManySyntaxTerm },
		{ "OneManySyntaxTerm", SyntaxRule::AdvanvedSyntaxTerm__OneManySyntaxTerm },
		{ "ConstSyntaxTerm", SyntaxRule::AdvanvedSyntaxTerm__ConstSyntaxTerm },
		
		{ "ZeroOneSyntaxTerm", SyntaxRule::NONE },
		{ "PrimarySyntaxTerm CH_QUESTION", SyntaxRule::ZeroOneSyntaxTerm__PrimarySyntaxTerm_CHQUESTION },
		
		{ "ZeroManySyntaxTerm", SyntaxRule::NONE },
		{ "LoopBody CH_ASTERISK", SyntaxRule::ZeroManySyntaxTerm__LoopBody_CHASTERISK },
		
		{ "OneManySyntaxTerm", SyntaxRule::NONE },
		{ "LoopBody CH_PLUS", SyntaxRule::OneManySyntaxTerm__LoopBody_CHPLUS },
		
		{ "LoopBody", SyntaxRule::NONE },
		{ "SimpleLoopBody", SyntaxRule::LoopBody__SimpleLoopBody },
		{ "AdvancedLoopBody", SyntaxRule::LoopBody__AdvancedLoopBody },
		
		{ "SimpleLoopBody", SyntaxRule::NONE },
		{ "PrimarySyntaxTerm", SyntaxRule::SimpleLoopBody__PrimarySyntaxTerm },
		
		{ "AdvancedLoopBody", SyntaxRule::NONE },
		{ "CH_OPAREN SyntaxOrExpression CH_COLON SyntaxOrExpression CH_CPAREN",
			SyntaxRule::AdvancedLoopBody__CHOPAREN_SyntaxOrExpression_CHCOLON_SyntaxOrExpression_CHCPAREN },
		{ "CH_OPAREN SyntaxOrExpression CH_CPAREN", SyntaxRule::AdvancedLoopBody__CHOPAREN_SyntaxOrExpression_CHCPAREN },
		
		{ "ConstSyntaxTerm", SyntaxRule::NONE },
		{ "CH_LT ConstExpression CH_GT", SyntaxRule::ConstSyntaxTerm__CHLT_ConstExpression_CHGT },
		
		{ "ConstExpression", SyntaxRule::NONE },
		{ "IntegerConstExpression", SyntaxRule::ConstExpression__IntegerConstExpression },
		{ "StringConstExpression", SyntaxRule::ConstExpression__StringConstExpression },
		{ "BooleanConstExpression", SyntaxRule::ConstExpression__BooleanConstExpression },
		{ "NativeConstExpression", SyntaxRule::ConstExpression__NativeConstExpression },
		
		{ "IntegerConstExpression", SyntaxRule::NONE },
		{ "NUMBER", SyntaxRule::IntegerConstExpression__NUMBER },
		
		{ "StringConstExpression", SyntaxRule::NONE },
		{ "STRING", SyntaxRule::StringConstExpression__STRING },
		
		{ "BooleanConstExpression", SyntaxRule::NONE },
		{ "KW_FALSE", SyntaxRule::BooleanConstExpression__KWFALSE },
		{ "KW_TRUE", SyntaxRule::BooleanConstExpression__KWTRUE },
		
		{ "NativeConstExpression", SyntaxRule::NONE },
		{ "NativeQualificationOpt NativeName NativeReferencesOpt",
			SyntaxRule::NativeConstExpression__NativeQualificationOpt_NativeName_NativeReferencesOpt },
		
		{ "NativeQualificationOpt", SyntaxRule::NONE },
		{ "NativeQualification", SyntaxRule::NativeQualificationOpt__NativeQualification },
		{ "", SyntaxRule::NativeQualificationOpt__ },
		
		{ "NativeQualification", SyntaxRule::NONE },
		{ "NAME CH_COLON_COLON", SyntaxRule::NativeQualification__NAME_CHCOLONCOLON },
		{ "NativeQualification NAME CH_COLON_COLON",
			SyntaxRule::NativeQualification__NativeQualification_NAME_CHCOLONCOLON },
		
		{ "NativeReferencesOpt", SyntaxRule::NONE },
		{ "NativeReferences", SyntaxRule::NativeReferencesOpt__NativeReferences },
		{ "", SyntaxRule::NativeReferencesOpt__ },
		
		{ "NativeReferences", SyntaxRule::NONE },
		{ "NativeReference", SyntaxRule::NativeReferences__NativeReference },
		{ "NativeReferences NativeReference", SyntaxRule::NativeReferences__NativeReferences_NativeReference },
		
		{ "NativeName", SyntaxRule::NONE },
		{ "NativeVariableName", SyntaxRule::NativeName__NativeVariableName },
		{ "NativeFunctionName", SyntaxRule::NativeName__NativeFunctionName },
		
		{ "NativeVariableName", SyntaxRule::NONE },
		{ "NAME", SyntaxRule::NativeVariableName__NAME },
		
		{ "NativeFunctionName", SyntaxRule::NONE },
		{ "NAME CH_OPAREN ConstExpressionListOpt CH_CPAREN",
			SyntaxRule::NativeFunctionName__NAME_CHOPAREN_ConstExpressionListOpt_CHCPAREN },
		
		{ "ConstExpressionListOpt", SyntaxRule::NONE },
		{ "ConstExpressionList", SyntaxRule::ConstExpressionListOpt__ConstExpressionList },
		{ "", SyntaxRule::ConstExpressionListOpt__ },
		
		{ "ConstExpressionList", SyntaxRule::NONE },
		{ "ConstExpression", SyntaxRule::ConstExpressionList__ConstExpression },
		{ "ConstExpressionList CH_COMMA ConstExpression",
			SyntaxRule::ConstExpressionList__ConstExpressionList_CHCOMMA_ConstExpression },
		
		{ "NativeReference", SyntaxRule::NONE },
		{ "CH_DOT NativeName", SyntaxRule::NativeReference__CHDOT_NativeName },
		{ "CH_MINUS_GT NativeName", SyntaxRule::NativeReference__CHMINUSGT_NativeName },
		
		{ nullptr, SyntaxRule::NONE }
	};

	//
	//CoreTables
	//

	class CoreTables {
		NONCOPYABLE(CoreTables);

		std::vector<State> m_states;
		std::vector<Shift> m_shifts;
		std::vector<Goto> m_gotos;
		std::vector<Reduce> m_reduces;
		State* m_start_state;

		MHeap m_managed_heap;

	public:
		CoreTables(
			std::vector<State>::size_type state_count,
			std::vector<Shift>::size_type shift_count,
			std::vector<Goto>::size_type goto_count,
			std::vector<Reduce>::size_type reduce_count,
			const LRState* lr_start_state)
			: m_states(state_count),
			m_shifts(shift_count),
			m_gotos(goto_count),
			m_reduces(reduce_count)
		{
			m_start_state = &m_states[lr_start_state->get_index()];
		}

		std::vector<State>::iterator get_states_begin() {
			return m_states.begin();
		}

		std::vector<Shift>::iterator get_shifts_begin() {
			return m_shifts.begin();
		}

		std::vector<Goto>::iterator get_gotos_begin() {
			return m_gotos.begin();
		}

		std::vector<Reduce>::iterator get_reduces_begin() {
			return m_reduces.begin();
		}

		const std::vector<State>& get_states() const {
			return m_states;
		}

		const std::vector<Reduce>& get_reduces() const {
			return m_reduces;
		}

		const State* get_start_state() const {
			return m_start_state;
		}

		MHeap& get_managed_heap() {
			return m_managed_heap;
		}
	};

	//
	//TransformShift
	//
	
	class TransformShift : public std::unary_function<const LRShift, Shift> {
		const std::vector<State>& m_states;
	public:
		explicit TransformShift(const std::vector<State>& states)
			: m_states(states)
		{}

		Shift operator()(const LRShift lr_shift) const {
			Shift core_shift;
			core_shift.assign(&m_states[lr_shift.get_state()->get_index()], lr_shift.get_tr()->get_tr_obj());
			return core_shift;
		}
	};

	//
	//TransformGoto
	//

	class TransformGoto : public std::unary_function<const LRGoto, Goto> {
		const std::vector<State>& m_states;
	public:
		explicit TransformGoto(const std::vector<State>& states)
			: m_states(states)
		{}

		Goto operator()(const LRGoto lr_goto) const {
			Goto core_goto;
			core_goto.assign(&m_states[lr_goto.get_state()->get_index()], lr_goto.get_nt()->get_nt_index());
			return core_goto;
		}
	};

	//
	//TransformReduce
	//

	class TransformReduce : public std::unary_function<const BnfGrm::Pr*, Reduce> {
	public:
		TransformReduce(){}

		Reduce operator()(const BnfGrm::Pr* pr) {
			Reduce core_reduce;
			if (pr) {
				syn::InternalAction action = static_cast<syn::InternalAction>(pr->get_pr_obj());
				core_reduce.assign(pr->get_elements().size(), pr->get_nt()->get_nt_index(), action);
			} else {
				core_reduce.assign(0, 0, syn::ACCEPT_ACTION);
			}
			return core_reduce;
		}
	};

	//
	//TransformState
	//

	//This class is implemented not like a standard functor, because it has to be used by pointer, not by value.
	class TransformState {
		const TransformShift m_transform_shift;
		const TransformGoto m_transform_goto;
		const TransformReduce m_transform_reduce;

		Shift m_null_shift;
		Goto m_null_goto;
		Reduce m_null_reduce;

		std::vector<Shift>::iterator m_shift_it;
		std::vector<Goto>::iterator m_goto_it;
		std::vector<Reduce>::iterator m_reduce_it;

	public:
		explicit TransformState(CoreTables* core_tables)
			: m_transform_shift(core_tables->get_states()),
			m_transform_goto(core_tables->get_states())
		{
			m_null_shift.assign(nullptr, 0);
			m_null_goto.assign(nullptr, 0);
			m_null_reduce.assign(0, 0, syn::NULL_ACTION);
			m_shift_it = core_tables->get_shifts_begin();
			m_goto_it = core_tables->get_gotos_begin();
			m_reduce_it = core_tables->get_reduces_begin();
		}

		State::SymType get_sym_type(const BnfGrm::Sym* sym) {
			//If the symbol is NULL, the type does not matter, because this is either a start state,
			//or a final one (goto by the extended nonterminal), 
			if (!sym) return State::sym_none;

			if (const BnfGrm::Tr* tr = sym->as_tr()) {
				prs::Tokens::E token = tr->get_tr_obj();
				if (prs::Tokens::NAME == token || prs::Tokens::STRING == token || prs::Tokens::NUMBER == token) {
					//This set of tokens must correspond to the set of tokens for which a value is created
					//in InternalScanner.
					return State::sym_tk_value;
				} else {
					return State::sym_none;
				}
			} else {
				//Assuming that if the symbol is not a nonterminal, then it is a terminal.
				return State::sym_nt;
			}
		}

		State transform(const LRState* lrstate) {
			State core_state;
			
			const BnfGrm::Sym* sym = lrstate->get_sym();
			State::SymType sym_type = get_sym_type(sym);
			bool is_nt = sym && sym->as_nt();
			core_state.assign(lrstate->get_index(), &*m_shift_it, &*m_goto_it, &*m_reduce_it, sym_type);

			m_shift_it = std::transform(
				lrstate->get_shifts().begin(),
				lrstate->get_shifts().end(),
				m_shift_it,
				m_transform_shift);
			*m_shift_it++ = m_null_shift;
			
			m_goto_it = std::transform(
				lrstate->get_gotos().begin(),
				lrstate->get_gotos().end(),
				m_goto_it,
				m_transform_goto);
			*m_goto_it++ = m_null_goto;
			
			m_reduce_it = std::transform(
				lrstate->get_reduces().begin(),
				lrstate->get_reduces().end(),
				m_reduce_it,
				m_transform_reduce);
			*m_reduce_it++ = m_null_reduce;

			return core_state;
		}
	};

	unique_ptr<CoreTables> create_core_tables(const BnfGrm* bnf_grammar, const LRTbl* lrtables) {
		const std::vector<const LRState*>& lrstates = lrtables->get_states();

		//Calculate counts.
		std::vector<Shift>::size_type shift_count = 0;
		std::vector<Goto>::size_type goto_count = 0;
		std::vector<Goto>::size_type reduce_count = 0;
		for (const LRState* lrstate : lrstates) {
			shift_count += lrstate->get_shifts().size() + 1;
			goto_count += lrstate->get_gotos().size() + 1;
			reduce_count += lrstate->get_reduces().size() + 1;
		}

		//Determine start state.
		//There must be one and only one start state.
		const LRState* lr_start_state = lrtables->get_start_states()[0].second;

		//Create empty tables.
		unique_ptr<CoreTables> core_tables(new CoreTables(
			lrstates.size(),
			shift_count,
			goto_count,
			reduce_count,
			lr_start_state));
	
		//Transform states.
		//Here the function object is not passed directly to std::transform(), because the object must be
		//used by pointer, not by value (because the object has a state).
		TransformState transform_state(core_tables.get());
		std::transform(
			lrtables->get_states().begin(),
			lrtables->get_states().end(),
			core_tables->get_states_begin(),
			std::bind1st(std::mem_fun(&TransformState::transform), &transform_state));
		
		return core_tables;
	}

	//
	//InternalScanner
	//

	class InternalScanner : public syn::ScannerInterface {
		NONCOPYABLE(InternalScanner);

		syn::Pool<ns::FilePos> m_pos_pool;
		syn::Pool<ns::syntax_number> m_number_pool;
		syn::Pool<ns::syntax_string> m_string_pool;

		prs::Scanner& m_scanner;
		prs::TokenRecord m_token_record;

	public:
		InternalScanner(
			prs::Scanner& scanner)
			: m_scanner(scanner),
			m_number_pool(200),
			m_string_pool(100)
		{}

		std::pair<syn::InternalTk, const void*> scan() override {
			m_scanner.scan_token(&m_token_record);
			const void* value = nullptr;
			if (prs::Tokens::NAME == m_token_record.token || prs::Tokens::STRING == m_token_record.token) {
				value = m_string_pool.allocate(m_token_record.v_string);
			} else if (prs::Tokens::NUMBER == m_token_record.token) {
				value = m_number_pool.allocate(m_token_record.v_number);
			} else {
				value = m_pos_pool.allocate(ns::FilePos(m_scanner.file_name(), m_token_record.pos));
			}
			return std::make_pair(m_token_record.token, value);
		}

		bool fire_syntax_error(const char* message) {
			throw prs::ParserException(message, ns::FilePos(m_scanner.file_name(), m_token_record.pos));
		}
	};

}

//
//parse_grammar()
//

unique_ptr<ns::GrammarParsingResult> prs::parse_grammar(std::istream& in, const util::String& file_name) {

	//Create BNF grammar.
	unique_ptr<const BnfGrm> bnf_grammar = RawPrs::raw_grammar_to_bnf(g_raw_tokens, g_raw_rules, SyntaxRule::NONE);

	//Create LR tables.
	std::vector<const BnfGrm::Nt*> start_nts;
	start_nts.push_back(bnf_grammar->get_nonterminals()[0]);
	unique_ptr<const LRTbl> lrtables = ns::create_LR_tables(*bnf_grammar.get(), start_nts, false);

	//Create managed heap.
	unique_ptr<MHeap> managed_heap(new MHeap());
	unique_ptr<MHeap> const_managed_heap(new MHeap());

	//Create core tables.
	unique_ptr<const CoreTables> core_tables(create_core_tables(bnf_grammar.get(), lrtables.get()));

	//Parse.
	prs::Scanner scanner(in, file_name);
	InternalScanner internal_scanner(scanner);

	MPtr<ebnf::Grammar> grammar_mptr;
	try {
		std::unique_ptr<syn::ParserInterface> parser = syn::ParserInterface::create();
		syn::StackElement_Nt* root_element = parser->parse(
			core_tables->get_start_state(),
			internal_scanner,
			static_cast<syn::InternalTk>(Tokens::END_OF_FILE)
		);

		ActionContext action_context(managed_heap.get(), const_managed_heap.get());
		grammar_mptr = action_context.nt_Grammar(root_element);
	} catch (const syn::SynSyntaxError&) {
		throw internal_scanner.fire_syntax_error("Syntax error");
	} catch (const syn::SynLexicalError&) {
		throw internal_scanner.fire_syntax_error("Lexical error");
	} catch (const syn::SynError&) {
		throw std::runtime_error("Unable to parse grammar");
	}

	//Return result.
	std::unique_ptr<util::MRoot<ebnf::Grammar>> grammar_root(new util::MRoot<ebnf::Grammar>(grammar_mptr));
	grammar_root->add_heap(std::move(managed_heap));

	return make_unique1<GrammarParsingResult>(std::move(grammar_root), std::move(const_managed_heap));
}
