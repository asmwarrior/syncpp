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

//Definitions of type descriptor classes.

#ifndef SYN_CORE_DESCRIPTOR_TYPE_H_INCLUDED
#define SYN_CORE_DESCRIPTOR_TYPE_H_INCLUDED

#include <cstddef>

#include "types.h"
#include "noncopyable.h"
#include "util_mptr.h"

namespace synbin {

	class TypeDescriptor;
	class VoidTypeDescriptor;
	class AbstractClassTypeDescriptor;
	class ClassTypeDescriptor;
	class PartClassTypeDescriptor;
	class ListTypeDescriptor;
	class PrimitiveTypeDescriptor;

	class TypeDescriptorVisitor;

}

//
//TypeDescriptor
//

//Data type descriptor. Describes a type produced by a syntax expression or a BNF symbol.
class synbin::TypeDescriptor {
	NONCOPYABLE(TypeDescriptor);

protected:
	TypeDescriptor(){}
	
public:
	virtual ~TypeDescriptor(){}

	virtual bool is_void() const;
	virtual const ClassTypeDescriptor* as_class_type() const;
	virtual const PrimitiveTypeDescriptor* as_primitive_type() const;

	virtual bool equals(const TypeDescriptor* type) const = 0;
	virtual bool accepts(const TypeDescriptor* type) const;

	virtual void visit(TypeDescriptorVisitor* visitor) const = 0;

	virtual void print(std::ostream& out) const = 0;

	virtual bool equals_class(const ClassTypeDescriptor* type) const;
	virtual bool equals_part_class(const PartClassTypeDescriptor* type) const;
	virtual bool equals_list(const ListTypeDescriptor* type) const;
	virtual bool equals_primitive(const PrimitiveTypeDescriptor* type) const;
};

//
//VoidTypeDescriptor
//

class synbin::VoidTypeDescriptor : public TypeDescriptor {
	NONCOPYABLE(VoidTypeDescriptor);

public:
	VoidTypeDescriptor();

	bool is_void() const override;
	bool equals(const TypeDescriptor* type) const override;

	void visit(TypeDescriptorVisitor* visitor) const override;
	void print(std::ostream& out) const override;
};

//
//AbstractClassTypeDescriptor
//

class synbin::AbstractClassTypeDescriptor : public TypeDescriptor {
	NONCOPYABLE(AbstractClassTypeDescriptor);

protected:
	AbstractClassTypeDescriptor();
};

//
//ClassTypeDescriptor
//

class synbin::ClassTypeDescriptor : public AbstractClassTypeDescriptor {
	NONCOPYABLE(ClassTypeDescriptor);

	const std::size_t m_index;
	const util::String m_class_name;

public:
	ClassTypeDescriptor(std::size_t index, const util::String& class_name);

	std::size_t get_index() const;
	const util::String& get_class_name() const;

	const ClassTypeDescriptor* as_class_type() const override;
	bool equals(const TypeDescriptor* type) const override;
	bool accepts(const TypeDescriptor* type) const override;

	void visit(TypeDescriptorVisitor* visitor) const override;
	void print(std::ostream& out) const override;

	bool equals_class(const ClassTypeDescriptor* type) const override;
};

//
//PartClassTypeDescriptor
//

class synbin::PartClassTypeDescriptor : public AbstractClassTypeDescriptor {
	NONCOPYABLE(PartClassTypeDescriptor);

	const util::MPtr<const ClassTypeDescriptor> m_class_type;
	const std::size_t m_tag_index;

public:
	PartClassTypeDescriptor(util::MPtr<const ClassTypeDescriptor> class_type, std::size_t tag_index);

	const util::MPtr<const ClassTypeDescriptor> get_class_type() const;

	bool equals(const TypeDescriptor* type) const override;
	
	void visit(TypeDescriptorVisitor* visitor) const override;
	void print(std::ostream& out) const override;

	bool equals_part_class(const PartClassTypeDescriptor* type) const override;
};

//
//ListTypeDescriptor
//

class synbin::ListTypeDescriptor : public TypeDescriptor {
	NONCOPYABLE(ListTypeDescriptor);

	const util::MPtr<const TypeDescriptor> m_element_type;

public:
	ListTypeDescriptor(util::MPtr<const TypeDescriptor> element_type);

	util::MPtr<const TypeDescriptor> get_element_type() const;

	bool equals(const TypeDescriptor* type) const override;
	
	void visit(TypeDescriptorVisitor* visitor) const override;
	void print(std::ostream& out) const override;

	bool equals_list(const ListTypeDescriptor* type) const override;
};

//
//PrimitiveTypeDescriptor
//

class synbin::PrimitiveTypeDescriptor : public TypeDescriptor {
	NONCOPYABLE(PrimitiveTypeDescriptor);

	const types::PrimitiveType* const m_primitive_type;

public:
	PrimitiveTypeDescriptor(const types::PrimitiveType* primitive_type);

	const PrimitiveTypeDescriptor* as_primitive_type() const override;
	const types::PrimitiveType* get_primitive_type() const;

	bool equals(const TypeDescriptor* type) const override;
	
	void visit(TypeDescriptorVisitor* visitor) const override;
	void print(std::ostream& out) const override;

	bool equals_primitive(const PrimitiveTypeDescriptor* type) const override;
};

//
//TypeDescriptorVisitor
//

class synbin::TypeDescriptorVisitor {
	NONCOPYABLE(TypeDescriptorVisitor);

protected:
	TypeDescriptorVisitor(){}

public:
	virtual void visit_VoidTypeDescriptor(const VoidTypeDescriptor* type) = 0;
	virtual void visit_ClassTypeDescriptor(const ClassTypeDescriptor* type) = 0;
	virtual void visit_PartClassTypeDescriptor(const PartClassTypeDescriptor* type) = 0;
	virtual void visit_ListTypeDescriptor(const ListTypeDescriptor* type) = 0;
	virtual void visit_PrimitiveTypeDescriptor(const PrimitiveTypeDescriptor* type) = 0;
};

#endif//SYN_CORE_DESCRIPTOR_TYPE_H_INCLUDED
