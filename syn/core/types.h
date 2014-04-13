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

//Classes representing data types.

#ifndef SYN_CORE_TYPES_H_INCLUDED
#define SYN_CORE_TYPES_H_INCLUDED

#include "ebnf__dec.h"
#include "noncopyable.h"
#include "util_mptr.h"
#include "util_string.h"

///////////////////////////////////////
//Declarations
///////////////////////////////////////

namespace synbin {

	namespace types {

		class Type;
		class PrimitiveType;
		class UserPrimitiveType;
		class SystemPrimitiveType;
		class ClassType;
		class NonterminalClassType;
		class NameClassType;
		class VoidType;
		class ArrayType;

	}

	template<class T>
	class TypeVisitor;

}

///////////////////////////////////////
//Definitions
///////////////////////////////////////

//
//Type
//

class synbin::types::Type {
	NONCOPYABLE(Type);

public:
	Type(){}
	virtual ~Type(){}

	virtual bool is_void() const { return false; }
	virtual const PrimitiveType* as_primitive() const { return nullptr; }
	virtual const ClassType* as_class() const { return nullptr; }

	template<class T>
	T visit(TypeVisitor<T>* visitor) const;

	void visit(TypeVisitor<void>* visitor) const;

	virtual void print(std::ostream& out) const = 0;

	virtual bool equals(const types::Type* type) const;
	virtual bool equals_concrete(const ArrayType* type) const;

private:
	virtual void visit0(TypeVisitor<void>* visitor) const = 0;
};

//
//PrimitiveType
//

class synbin::types::PrimitiveType : public synbin::types::Type {
	NONCOPYABLE(PrimitiveType);

	virtual void abstract_class() = 0;

	const util::String m_name;

public:
	PrimitiveType(const util::String& name);

	const util::String& get_name() const;

	const PrimitiveType* as_primitive() const override { return this; }
	virtual bool is_system() const = 0;

private:
	void visit0(TypeVisitor<void>* visitor) const = 0;
};

//
//UserPrimitiveType
//

class synbin::types::UserPrimitiveType : public synbin::types::PrimitiveType {
	NONCOPYABLE(UserPrimitiveType);

	void abstract_class() override{}

public:
	UserPrimitiveType(const util::String& name);

	bool is_system() const override { return false; }
	void print(std::ostream& out) const override;

private:
	void visit0(TypeVisitor<void>* visitor) const override;
};

//
//SystemPrimitiveType
//

class synbin::types::SystemPrimitiveType : public synbin::types::PrimitiveType {
	NONCOPYABLE(SystemPrimitiveType);

	void abstract_class() override{}

public:
	SystemPrimitiveType(const util::String& name);

	bool is_system() const override { return true; }
	void print(std::ostream& out) const override;

private:
	void visit0(TypeVisitor<void>* visitor) const override;
};

//
//ClassType
//

class synbin::types::ClassType : public synbin::types::Type {
	NONCOPYABLE(ClassType);

public:
	ClassType(){}

	virtual const util::String& get_class_name() const = 0;
	const ClassType* as_class() const override { return this; }

private:
	void visit0(TypeVisitor<void>* visitor) const override;
};

//
//NonterminalClassType
//

class synbin::types::NonterminalClassType : public synbin::types::ClassType {
	NONCOPYABLE(NonterminalClassType);

	ebnf::NonterminalDeclaration* const m_nt;

public:
	NonterminalClassType(ebnf::NonterminalDeclaration* nt);

	const util::String& get_class_name() const;
	ebnf::NonterminalDeclaration* get_nt() const { return m_nt; }
	void print(std::ostream& out) const override;

private:
	void visit0(TypeVisitor<void>* visitor) const override;
};

//
//NameClassType
//

class synbin::types::NameClassType : public synbin::types::ClassType {
	NONCOPYABLE(NameClassType);

	const util::String m_name;

public:
	NameClassType(const util::String& name);

	const util::String& get_class_name() const;
	void print(std::ostream& out) const override;

private:
	void visit0(TypeVisitor<void>* visitor) const override;
};

//
//VoidType
//

class synbin::types::VoidType : public synbin::types::Type {
	NONCOPYABLE(VoidType);

public:
	VoidType(){}

	bool is_void() const override { return true; }
	void print(std::ostream& out) const override;

private:
	void visit0(TypeVisitor<void>* visitor) const override;
};

//
//ArrayType
//

class synbin::types::ArrayType : public synbin::types::Type {
	NONCOPYABLE(ArrayType);

	const util::MPtr<const Type> m_element_type;

public:
	ArrayType(util::MPtr<const Type> element_type);

	util::MPtr<const Type> get_element_type() const { return m_element_type; }
	void print(std::ostream& out) const override;

	bool equals(const types::Type* type) const override;
	bool equals_concrete(const ArrayType* type) const override;

private:
	void visit0(TypeVisitor<void>* visitor) const override;
};

//
//TypeVisitor
//

template<class T>
class synbin::TypeVisitor {
	NONCOPYABLE(TypeVisitor);

public:
	TypeVisitor(){}
	virtual ~TypeVisitor(){}

	virtual T visit_Type(const types::Type* type);
	virtual T visit_PrimitiveType(const types::PrimitiveType* type);
	virtual T visit_UserPrimitiveType(const types::UserPrimitiveType* type);
	virtual T visit_SystemPrimitiveType(const types::SystemPrimitiveType* type);
	virtual T visit_ClassType(const types::ClassType* type);
	virtual T visit_NonterminalClassType(const types::NonterminalClassType* type);
	virtual T visit_NameClassType(const types::NameClassType* type);
	virtual T visit_VoidType(const types::VoidType* type);
	virtual T visit_ArrayType(const types::ArrayType* type);
};

///////////////////////////////////////
//Implementations
///////////////////////////////////////

//
//Type
//

template<class T>
T synbin::types::Type::visit(synbin::TypeVisitor<T>* visitor) const {
	struct AdaptingVisitor : public TypeVisitor<void> {
		TypeVisitor<T>* m_effective_visitor;
		T m_value;
	
		void visit_Type(const types::Type* type) override {
			m_value = m_effective_visitor->visit_Type(type);
		}
		void visit_PrimitiveType(const types::PrimitiveType* type) override {
			m_value = m_effective_visitor->visit_PrimitiveType(type);
		}
		void visit_UserPrimitiveType(const types::UserPrimitiveType* type) override {
			m_value = m_effective_visitor->visit_UserPrimitiveType(type);
		}
		void visit_SystemPrimitiveType(const types::SystemPrimitiveType* type) override {
			m_value = m_effective_visitor->visit_SystemPrimitiveType(type);
		}
		void visit_ClassType(const types::ClassType* type) override {
			m_value = m_effective_visitor->visit_ClassType(type);
		}
		void visit_NonterminalClassType(const types::NonterminalClassType* type) override {
			m_value = m_effective_visitor->visit_NonterminalClassType(type);
		}
		void visit_NameClassType(const types::NameClassType* type) override {
			m_value = m_effective_visitor->visit_NameClassType(type);
		}
		void visit_VoidType(const types::VoidType* type) override {
			m_value = m_effective_visitor->visit_VoidType(type);
		}
		void visit_ArrayType(const types::ArrayType* type) override {
			m_value = m_effective_visitor->visit_ArrayType(type);
		}
	};
	
	AdaptingVisitor adapting_visitor;
	adapting_visitor.m_effective_visitor = visitor;
	visit0(&adapting_visitor);
	
	return adapting_visitor.m_value;
}

//
//TypeVisitor
//

template<class T>
T synbin::TypeVisitor<T>::visit_Type(const types::Type* type) {
	return T();
}

template<class T>
T synbin::TypeVisitor<T>::visit_PrimitiveType(const types::PrimitiveType* type) {
	return visit_Type(type);
}

template<class T>
T synbin::TypeVisitor<T>::visit_UserPrimitiveType(const types::UserPrimitiveType* type) {
	return visit_PrimitiveType(type);
}

template<class T>
T synbin::TypeVisitor<T>::visit_SystemPrimitiveType(const types::SystemPrimitiveType* type) {
	return visit_PrimitiveType(type);
}

template<class T>
T synbin::TypeVisitor<T>::visit_ClassType(const types::ClassType* type) {
	return visit_Type(type);
}

template<class T>
T synbin::TypeVisitor<T>::visit_NonterminalClassType(const types::NonterminalClassType* type) {
	return visit_ClassType(type);
}

template<class T>
T synbin::TypeVisitor<T>::visit_NameClassType(const types::NameClassType* type) {
	return visit_ClassType(type);
}

template<class T>
T synbin::TypeVisitor<T>::visit_VoidType(const types::VoidType* type) {
	return visit_Type(type);
}

template<class T>
T synbin::TypeVisitor<T>::visit_ArrayType(const types::ArrayType* type) {
	return visit_Type(type);
}

#endif//SYN_CORE_TYPES_H_INCLUDED
