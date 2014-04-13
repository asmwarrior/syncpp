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

//EBNF grammar classes definitions.

#ifndef SYN_CORE_EBNF_DEF_H_INCLUDED
#define SYN_CORE_EBNF_DEF_H_INCLUDED

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "ebnf__dec.h"
#include "ebnf_extension__def.h"
#include "ebnf_visitor__dec.h"
#include "noncopyable.h"
#include "primitives.h"
#include "types.h"
#include "util.h"
#include "util_mptr.h"

namespace synbin {
	namespace ebnf {

		//Syntax operator priorities. The order of constants is important.
		enum SyntaxPriority {
			PRIOR_NT,
			PRIOR_TOP,
			PRIOR_OR,
			PRIOR_AND,
			PRIOR_TERM
		};

		//All classes are direct or indirect subclasses of Object. Object has virtual destructor,
		//therefore making all the hierarchy polymorphic.

		//All the classes are intended to be used by reference (pointer) only, not by value. For this reason
		//copy/move constructors/assignment operators are deleted.
		
		////// Base Classes //////

		//
		//Object
		//

		//Base class for all EBNF grammar components.
		class Object {
			NONCOPYABLE(Object);

		public:
			Object(){}
			virtual ~Object(){}
		};

		//
		//Grammar
		//

		//EBNF Grammar. Constructed by the grammar parser, based on the grammar text.
		class Grammar : public Object {
			NONCOPYABLE(Grammar);

			const util::MPtr<const std::vector<util::MPtr<Declaration>>> m_syn_declarations;

			std::vector<TerminalDeclaration*> m_terminals;
			std::vector<NonterminalDeclaration*> m_nonterminals;

		public:
			Grammar(util::MPtr<const std::vector<util::MPtr<Declaration>>> declarations);
			
			const std::vector<util::MPtr<Declaration>>& get_declarations() const { return *m_syn_declarations.get(); }
			const std::vector<TerminalDeclaration*>& get_terminals() const { return m_terminals; }
			const std::vector<NonterminalDeclaration*>& get_nonterminals() const { return m_nonterminals; }
			std::size_t get_tr_count() const { return m_terminals.size(); }
			std::size_t get_nt_count() const { return m_nonterminals.size(); }

			void print(std::ostream& out) const;
		};

		//
		//Declaration
		//

		class Declaration : public Object {
			NONCOPYABLE(Declaration);

			friend class Grammar;

		public:
			Declaration(){}

		private:
			virtual void enumerate(
				std::vector<TerminalDeclaration*>& terminals,
				std::vector<NonterminalDeclaration*>& nonterminals)
			{}
			
			virtual void visit0(DeclarationVisitor<void>* visitor) = 0;

		public:
			virtual const NonterminalDeclaration* as_nt() const { return nullptr; }
			virtual NonterminalDeclaration* as_nt() { return nullptr; }

			template<class T>
			T visit(DeclarationVisitor<T>* visitor);

			void visit(DeclarationVisitor<void>* visitor) { visit0(visitor); }

			virtual void print(std::ostream& out) const = 0;
		};

		//
		//RegularDeclaration
		//

		class RegularDeclaration : public Declaration {
			NONCOPYABLE(RegularDeclaration);

			const syntax_string m_syn_name;

		public:
			RegularDeclaration(const syntax_string& name);
			const syntax_string& get_name() const { return m_syn_name; }
		};

		//
		//TypeDeclaration
		//

		class TypeDeclaration : public RegularDeclaration {
			NONCOPYABLE(TypeDeclaration);

		public:
			TypeDeclaration(const syntax_string& name);
			
			void print(std::ostream& out) const override;

		private:
			void visit0(DeclarationVisitor<void>* visitor) override;
		};

		//
		//SymbolDeclaration
		//

		class SymbolDeclaration : public RegularDeclaration {
			NONCOPYABLE(SymbolDeclaration);

		public:
			SymbolDeclaration(const syntax_string& name);

		private:
			void visit0(DeclarationVisitor<void>* visitor) override;
			virtual void visit0(SymbolDeclarationVisitor<void>* visitor) = 0;

		public:
			template<class T>
			T visit(SymbolDeclarationVisitor<T>* visitor);

			void visit(SymbolDeclarationVisitor<void>* visitor) { visit0(visitor); }
		};

		//
		//TerminalDeclaration
		//

		class TerminalDeclaration : public SymbolDeclaration {
			NONCOPYABLE(TerminalDeclaration);

			const util::MPtr<const RawType> m_syn_raw_type; //can be 0.
			
			std::size_t m_tr_index;
			const types::PrimitiveType* m_type; //can be 0.

		public:
			TerminalDeclaration(const syntax_string& name, util::MPtr<const RawType> raw_type);
			const RawType* get_raw_type() const { return m_syn_raw_type.get(); }
			void set_type(const types::PrimitiveType* type) { m_type = type; }
			const types::PrimitiveType* get_type() const { return m_type; }
			std::size_t tr_index() const { return m_tr_index; }

			void print(std::ostream& out) const override;

		private:
			void enumerate(
				std::vector<TerminalDeclaration*>& terminals,
				std::vector<NonterminalDeclaration*>& nonterminals) override;
			
			void visit0(SymbolDeclarationVisitor<void>* visitor) override;
		};

		//
		//NonterminalDeclaration
		//

		class NonterminalDeclaration : public SymbolDeclaration {
			NONCOPYABLE(NonterminalDeclaration);

			const bool m_syn_start;
			const util::MPtr<SyntaxExpression> m_syn_expression;
			const util::MPtr<const RawType> m_syn_explicit_raw_type;
			
			util::MPtr<const types::Type> m_explicit_type;
			std::size_t m_nt_index;

			//Some specific data is held in an 'extension'.
			std::unique_ptr<NonterminalDeclarationExtension> m_extension;

		public:
			NonterminalDeclaration(
				bool start,
				const syntax_string& name,
				util::MPtr<SyntaxExpression> expression,
				util::MPtr<const RawType> explicit_raw_type);
			
			bool is_start() const { return m_syn_start; }
			const RawType* get_explicit_raw_type() const { return m_syn_explicit_raw_type.get(); }
			util::MPtr<const types::Type> get_explicit_type() const { return m_explicit_type; }
			void set_explicit_type(util::MPtr<const types::Type> explicit_type) { m_explicit_type = explicit_type; }
			const SyntaxExpression* get_expression() const { return m_syn_expression.get(); }
			SyntaxExpression* get_expression() { return m_syn_expression.get(); }
			const NonterminalDeclaration* as_nt() const override { return this; }
			NonterminalDeclaration* as_nt() override { return this; }
			std::size_t nt_index() const { return m_nt_index; }
			void install_extension(std::unique_ptr<NonterminalDeclarationExtension> extension);
			NonterminalDeclarationExtension* get_extension() const;

			void print(std::ostream& out) const override;

		private:
			void enumerate(
				std::vector<TerminalDeclaration*>& terminals,
				std::vector<NonterminalDeclaration*>& nonterminals) override;
			
			void visit0(SymbolDeclarationVisitor<void>* visitor) override;
		};

		//
		//CustomTerminalTypeDeclaration
		//

		class CustomTerminalTypeDeclaration : public Declaration {
			NONCOPYABLE(CustomTerminalTypeDeclaration);

			const util::MPtr<const RawType> m_syn_raw_type; //CANNOT be 0.

		public:
			CustomTerminalTypeDeclaration(util::MPtr<const RawType> raw_type);
			const RawType* get_raw_type() const { return m_syn_raw_type.get(); }

			void print(std::ostream& out) const override;

		private:
			void visit0(DeclarationVisitor<void>* visitor);
		};

		//
		//RawType
		//

		//'syntax_string' could be used instead of this class. However, the concept 'RawType' is
		//referenced from different classes, so a separate class has been created for it.
		class RawType : public Object {
			NONCOPYABLE(RawType);

			const syntax_string m_syn_name;

		public:
			RawType(const syntax_string& name);
			const syntax_string& get_name() const { return m_syn_name; }
			
			void print(std::ostream& out) const;
		};

		////// Syntax Expressions //////

		//
		//SyntaxExpression
		//

		class SyntaxExpression : public Object {
			NONCOPYABLE(SyntaxExpression);

			std::unique_ptr<SyntaxExpressionExtension> m_extension;

		public:
			SyntaxExpression(){}

		private:
			virtual void visit0(SyntaxExpressionVisitor<void>* visitor) = 0;

		public:
			void install_extension(std::unique_ptr<SyntaxExpressionExtension> extension);
			SyntaxExpressionExtension* get_extension() const;

			virtual void print(std::ostream& out, SyntaxPriority priority) const = 0;

			template<class T>
			T visit(SyntaxExpressionVisitor<T>* visitor);

			void visit(SyntaxExpressionVisitor<void>* visitor) { visit0(visitor); }

			template<class C, class T>
			static void visit_all(const C& container, SyntaxExpressionVisitor<T>* visitor) {
				for (typename C::value_type expr : container) expr->visit(visitor);
			}

			template<class C, class T>
			static void visit_all(
				const C& container,
				SyntaxExpressionVisitor<T>* visitor,
				std::vector<T>& result)
			{
				typedef typename C::value_type Expr;
				for (SyntaxExpression* expr : container) {
					T value = expr->visit(visitor);
					result->push_back(value);
				}
			}
		};

		//
		//EmptySyntaxExpression
		//

		class EmptySyntaxExpression : public SyntaxExpression {
			NONCOPYABLE(EmptySyntaxExpression);

		public:
			EmptySyntaxExpression(){}

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor);
		};

		//
		//CompoundSyntaxExpression
		//

		class CompoundSyntaxExpression : public SyntaxExpression {
			NONCOPYABLE(CompoundSyntaxExpression);

			const util::MPtr<const std::vector<util::MPtr<SyntaxExpression>>> m_syn_sub_expressions;

		public:
			CompoundSyntaxExpression(util::MPtr<const std::vector<util::MPtr<SyntaxExpression>>> sub_expressions);

			const std::vector<util::MPtr<SyntaxExpression>>& get_sub_expressions() const {
				return *m_syn_sub_expressions.get();
			}
		};

		//
		//SyntaxOrExpression
		//

		class SyntaxOrExpression : public CompoundSyntaxExpression {
			NONCOPYABLE(SyntaxOrExpression);

		public:
			SyntaxOrExpression(util::MPtr<const std::vector<util::MPtr<SyntaxExpression>>> sub_expressions);

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor);
		};

		//
		//SyntaxAndExpression
		//

		class SyntaxAndExpression : public CompoundSyntaxExpression {
			NONCOPYABLE(SyntaxAndExpression);

			const util::MPtr<const RawType> m_syn_raw_type; //can be 0.

			const types::ClassType* m_type; //can be 0.
			std::unique_ptr<SyntaxAndExpressionExtension> m_and_extension;

		public:
			SyntaxAndExpression(
				util::MPtr<const std::vector<util::MPtr<SyntaxExpression>>> sub_expressions,
				util::MPtr<const RawType> raw_type);

			void install_and_extension(std::unique_ptr<SyntaxAndExpressionExtension> and_extension);
			SyntaxAndExpressionExtension* get_and_extension() const;

			const RawType* get_raw_type() const { return m_syn_raw_type.get(); }
			const types::ClassType* get_type() const { return m_type; }
			void set_type(const types::ClassType* type);

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor) override;
		};

		//
		//SyntaxElement
		//

		class SyntaxElement : public SyntaxExpression {
			NONCOPYABLE(SyntaxElement);

			const util::MPtr<SyntaxExpression> m_syn_expression;

		public:
			SyntaxElement(util::MPtr<SyntaxExpression> expression);
			const SyntaxExpression* get_expression() const { return m_syn_expression.get(); }
			SyntaxExpression* get_expression() { return m_syn_expression.get(); }
			
			virtual void print(std::ostream& out, SyntaxPriority priority) const = 0;
		};

		//
		//NameSyntaxElement
		//

		class NameSyntaxElement : public SyntaxElement {
			NONCOPYABLE(NameSyntaxElement);

			const syntax_string m_syn_name; //can be empty.

		public:
			NameSyntaxElement(util::MPtr<SyntaxExpression> expression, const syntax_string& name);
			const syntax_string& get_name() const { return m_syn_name; }

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor) override;
		};

		//
		//ThisSyntaxElement
		//

		class ThisSyntaxElement : public SyntaxElement {
			NONCOPYABLE(ThisSyntaxElement);

			FilePos m_syn_pos;

		public:
			ThisSyntaxElement(const FilePos& pos, util::MPtr<SyntaxExpression> expression);
			
			const FilePos& get_pos() const;
			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor);
		};

		//
		//NameSyntaxExpression
		//

		class NameSyntaxExpression : public SyntaxExpression {
			NONCOPYABLE(NameSyntaxExpression);

			const syntax_string m_syn_name;
			
			ebnf::SymbolDeclaration* m_sym;

		public:
			NameSyntaxExpression(const syntax_string& name);
			const syntax_string& get_name() const { return m_syn_name; }
			ebnf::SymbolDeclaration* get_sym() { return m_sym; }

			void set_sym(ebnf::SymbolDeclaration* sym);

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor) override;
		};

		//
		//StringSyntaxExpression
		//
		
		class StringSyntaxExpression : public SyntaxExpression {
			NONCOPYABLE(StringSyntaxExpression);

			const syntax_string m_syn_string;

		public:
			StringSyntaxExpression(const syntax_string& string);
			const syntax_string& get_string() const { return m_syn_string; }

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor) override;
		};

		//
		//CastSyntaxExpression
		//
		
		class CastSyntaxExpression : public SyntaxExpression {
			NONCOPYABLE(CastSyntaxExpression);

			const util::MPtr<const RawType> m_syn_raw_type;
			const util::MPtr<SyntaxExpression> m_syn_expression;

			util::MPtr<const types::Type> m_type;

		public:
			CastSyntaxExpression(util::MPtr<const RawType> raw_type, util::MPtr<SyntaxExpression> expression);
			SyntaxExpression* get_expression() { return m_syn_expression.get(); }
			const RawType* get_raw_type() const { return m_syn_raw_type.get(); }
			const util::MPtr<const types::Type> get_type() const { return m_type; }

			void set_type(util::MPtr<const types::Type> type);

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor) override;
		};

		//
		//ZeroOneSyntaxExpression
		//

		class ZeroOneSyntaxExpression : public SyntaxExpression {
			NONCOPYABLE(ZeroOneSyntaxExpression);

			const util::MPtr<SyntaxExpression> m_syn_sub_expression;

		public:
			ZeroOneSyntaxExpression(util::MPtr<SyntaxExpression> sub_expression);
			const SyntaxExpression* get_sub_expression() const { return m_syn_sub_expression.get(); }
			SyntaxExpression* get_sub_expression() { return m_syn_sub_expression.get(); }

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor);
		};

		//
		//LoopSyntaxExpression
		//

		class LoopSyntaxExpression : public SyntaxExpression {
			NONCOPYABLE(LoopSyntaxExpression);

			util::MPtr<LoopBody> m_syn_body;

		public:
			LoopSyntaxExpression(util::MPtr<LoopBody> body);
			const LoopBody* get_body() const { return m_syn_body.get(); }
		};

		//
		//ZeroManySyntaxExpression
		//
		
		class ZeroManySyntaxExpression : public LoopSyntaxExpression {
			NONCOPYABLE(ZeroManySyntaxExpression);

		public:
			ZeroManySyntaxExpression(util::MPtr<LoopBody> body);

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor) override;
		};

		//
		//OneManySyntaxExpression
		//
		
		class OneManySyntaxExpression : public LoopSyntaxExpression {
			NONCOPYABLE(OneManySyntaxExpression);

		public:
			OneManySyntaxExpression(util::MPtr<LoopBody> body);
			
			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor) override;
		};

		//
		//LoopBody
		//
		
		class LoopBody : public Object {
			NONCOPYABLE(LoopBody);

			const util::MPtr<SyntaxExpression> m_syn_expression;
			const util::MPtr<SyntaxExpression> m_syn_separator; //can be 0.
			const FilePos m_syn_separator_pos;

		public:
			LoopBody(
				util::MPtr<SyntaxExpression> expression,
				util::MPtr<SyntaxExpression> separator,
				const FilePos& separator_pos);

			SyntaxExpression* get_expression() const { return m_syn_expression.get(); }
			SyntaxExpression* get_separator() const { return m_syn_separator.get(); }
			const FilePos& get_separator_pos() const { return m_syn_separator_pos; }
			
			void print(std::ostream& out) const;
		};

		//
		//ConstSyntaxExpression
		//
		
		class ConstSyntaxExpression : public SyntaxExpression {
			NONCOPYABLE(ConstSyntaxExpression);

			const util::MPtr<const ConstExpression> m_syn_expression;

		public:
			ConstSyntaxExpression(util::MPtr<const ConstExpression> expression);
			const ConstExpression* get_expression() const { return m_syn_expression.get(); }

			void print(std::ostream& out, SyntaxPriority priority) const override;

		private:
			void visit0(SyntaxExpressionVisitor<void>* visitor) override;
		};

		////// Constant Expressions //////

		//
		//ConstExpression
		//

		class ConstExpression : public Object {
			NONCOPYABLE(ConstExpression);

		public:
			ConstExpression(){}

			template<class T>
			T visit(ConstExpressionVisitor<T>* visitor) const;
			
			void visit(ConstExpressionVisitor<void>* visitor) const { visit0(visitor); }

			virtual void print(std::ostream& out) const = 0;

		private:
			virtual void visit0(ConstExpressionVisitor<void>* visitor) const = 0;
		};

		//
		//IntegerConstExpression
		//

		class IntegerConstExpression : public ConstExpression {
			NONCOPYABLE(IntegerConstExpression);

			const syntax_number m_syn_value;

		public:
			IntegerConstExpression(syntax_number value);
			const syntax_number& get_value() const { return m_syn_value; }

			void print(std::ostream& out) const override;

		private:
			void visit0(ConstExpressionVisitor<void>* visitor) const override;
		};

		//
		//StringConstExpression
		//
		
		class StringConstExpression : public ConstExpression {
			NONCOPYABLE(StringConstExpression);

			const syntax_string m_syn_value;

		public:
			StringConstExpression(const syntax_string& value);
			const syntax_string& get_value() const { return m_syn_value; }

			void print(std::ostream& out) const override;

		private:
			void visit0(ConstExpressionVisitor<void>* visitor) const override;
		};

		//
		//BooleanConstExpression
		//
		
		class BooleanConstExpression : public ConstExpression {
			NONCOPYABLE(BooleanConstExpression);

			const bool m_syn_value;

		public:
			BooleanConstExpression(bool value);
			bool get_value() const { return m_syn_value; }

			void print(std::ostream& out) const override;

		private:
			void visit0(ConstExpressionVisitor<void>* visitor) const override;
		};

		//
		//NativeConstExpression
		//
		
		class NativeConstExpression : public ConstExpression {
			NONCOPYABLE(NativeConstExpression);

			const util::MPtr<const std::vector<syntax_string>> m_syn_qualifiers; //can be empty.
			const util::MPtr<const NativeName> m_syn_name;
			const util::MPtr<const std::vector<util::MPtr<const NativeReference>>> m_syn_references;  //can be empty.

		public:
			NativeConstExpression(
				util::MPtr<const std::vector<syntax_string>> qualifiers,
				util::MPtr<const NativeName> name,
				util::MPtr<const std::vector<util::MPtr<const NativeReference>>> references);
			
			const std::vector<syntax_string>& get_qualifiers() const { return *m_syn_qualifiers.get(); }
			const NativeName* get_name() const { return m_syn_name.get(); }

			const std::vector<util::MPtr<const NativeReference>>& get_references() const {
				return *m_syn_references.get();
			}

			void print(std::ostream& out) const override;

		private:
			void visit0(ConstExpressionVisitor<void>* visitor) const override;
		};

		//
		//NativeName
		//

		class NativeName : public Object {
			NONCOPYABLE(NativeName);
			
			const syntax_string m_syn_name;

		public:
			NativeName(const syntax_string& name);
			const syntax_string& get_name() const { return m_syn_name; }

			virtual const NativeVariableName* as_variable() const { return nullptr; }
			virtual const NativeFunctionName* as_function() const { return nullptr; }

			virtual void print(std::ostream& out) const = 0;
		};

		//
		//NativeVariableName
		//

		class NativeVariableName : public NativeName {
			NONCOPYABLE(NativeVariableName);

		public:
			NativeVariableName(const syntax_string& name);

			const NativeVariableName* as_variable() const override { return this; }

			void print(std::ostream& out) const override;
		};

		//
		//NativeFunctionName
		//

		class NativeFunctionName : public NativeName {
			NONCOPYABLE(NativeFunctionName);

			const util::MPtr<const std::vector<util::MPtr<const ConstExpression>>> m_syn_arguments; //can be empty.

		public:
			NativeFunctionName(
				const syntax_string& name,
				util::MPtr<const std::vector<util::MPtr<const ConstExpression>>> arguments);

			const std::vector<util::MPtr<const ConstExpression>>& get_arguments() const {
				return *m_syn_arguments.get();
			}

			const NativeFunctionName* as_function() const override { return this; }

			void print(std::ostream& out) const override;
		};

		//
		//NativeReference
		//

		class NativeReference : public Object {
			NONCOPYABLE(NativeReference);

			const util::MPtr<const NativeName> m_syn_name;

		public:
			NativeReference(util::MPtr<const NativeName> name);
			const NativeName* get_native_name() const { return m_syn_name.get(); }

			virtual bool is_pointer() const = 0;
			
			virtual void print(std::ostream& out) const = 0;
		};

		//
		//NativePointerReference
		//

		class NativePointerReference : public NativeReference {
			NONCOPYABLE(NativePointerReference);

		public:
			explicit NativePointerReference(util::MPtr<const NativeName> name);

			bool is_pointer() const override { return true; }

			void print(std::ostream& out) const override;
		};

		//
		//NativeReferenceReference
		//

		class NativeReferenceReference : public NativeReference {
			NONCOPYABLE(NativeReferenceReference);

		public:
			explicit NativeReferenceReference(util::MPtr<const NativeName> name);

			bool is_pointer() const override { return false; }

			void print(std::ostream& out) const override;
		};

	}
}

#endif//SYN_CORE_EBNF_DEF_H_INCLUDED
