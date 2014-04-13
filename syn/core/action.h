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

//Definitions of classes representing semantic actions (actions associated with BNF productions).

#ifndef SYN_CORE_ACTION_H_INCLUDED
#define SYN_CORE_ACTION_H_INCLUDED

#include <cassert>
#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "action__dec.h"
#include "descriptor_type.h"
#include "ebnf__dec.h"
#include "noncopyable.h"
#include "types.h"
#include "util_mptr.h"
#include "util_string.h"

//
//Action
//

//Root class for semantic actions. Describes which code has to be generated for an action
//associated with a particular BNF production. The code is executed when the production is reduced.
class synbin::Action {
	NONCOPYABLE(Action);

protected:
	Action(){}

public:
	virtual ~Action(){}

	//Returns the type of the result value produced by the code of this action.
	virtual util::MPtr<const TypeDescriptor> get_result_type() const = 0;

	//If the action produces a part-class, this function returns the class of that part-class.
	//Otherwise, it returns NULL.
	virtual util::MPtr<const ClassTypeDescriptor> get_part_class_type_class() const;

	virtual void visit(ActionVisitor* visitor) const = 0;
	virtual void print(std::ostream& out) const = 0;
};

//
//AbstractAction
//

//A template for an action base class. Parameterized with the type of the type descriptor used to
//describe the result type of the action.
template<class T>
class synbin::AbstractAction : public Action {
	NONCOPYABLE(AbstractAction);

protected:
	const util::MPtr<const T> m_result_type;

protected:
	AbstractAction(util::MPtr<const T> result_type) : m_result_type(result_type) {
		assert(!!m_result_type);
	}

public:
	util::MPtr<const TypeDescriptor> get_result_type() const override final {
		return m_result_type;
	}

	//Returns the same type descriptor as get_result_type() does, but as an object of
	//the concrete TypeDescriptor subclass, instead of TypeDescriptor itself.
	util::MPtr<const T> get_concrete_result_type() const {
		return m_result_type;
	}
};

//
//VoidAction
//

//An action that returns nothing.
class synbin::VoidAction : public AbstractAction<VoidTypeDescriptor> {
	NONCOPYABLE(VoidAction);

public:
	VoidAction(util::MPtr<const VoidTypeDescriptor> void_type);

	void visit(ActionVisitor* visitor) const override;
	void print(std::ostream& out) const override;
};

//
//CopyAction
//

//An action that just copies and returns the value returned by another action.
class synbin::CopyAction : public AbstractAction<TypeDescriptor> {
	NONCOPYABLE(CopyAction);

public:
	CopyAction(util::MPtr<const TypeDescriptor> type);

	void visit(ActionVisitor* visitor) const override;
	void print(std::ostream& out) const override;
};

//
//CastAction
//

//Class type casting action.
class synbin::CastAction : public AbstractAction<ClassTypeDescriptor> {
	NONCOPYABLE(CastAction);

	const util::MPtr<const ClassTypeDescriptor> m_actual_type;

public:
	CastAction(
		util::MPtr<const ClassTypeDescriptor> cast_type,
		util::MPtr<const ClassTypeDescriptor> actual_type);

	util::MPtr<const ClassTypeDescriptor> get_actual_type() const;

	void visit(ActionVisitor* visitor) const override;
	void print(std::ostream& out) const override;
};

//
//AbstractClassAction
//

//A common base class for class-based actions.
class synbin::AbstractClassAction : public Action {
	NONCOPYABLE(AbstractClassAction);

	const util::MPtr<const ClassTypeDescriptor> m_class_type;
	const std::unique_ptr<const ActionDefs::AttributeVector> m_attributes;
	const std::unique_ptr<const ActionDefs::PartClassVector> m_part_classes;
	const std::unique_ptr<const ActionDefs::ClassVector> m_classes;

public:
	AbstractClassAction(
		util::MPtr<const ClassTypeDescriptor> class_type,
		std::unique_ptr<const ActionDefs::AttributeVector> attributes,
		std::unique_ptr<const ActionDefs::PartClassVector> part_classes,
		std::unique_ptr<const ActionDefs::ClassVector> classes);

	util::MPtr<const ClassTypeDescriptor> get_class_type() const;
	const ActionDefs::AttributeVector& get_attributes() const;
	const ActionDefs::PartClassVector& get_part_classes() const;
	const ActionDefs::ClassVector& get_classes() const;

	void print(std::ostream& out) const override;

private:
	virtual void print_header(std::ostream& out) const = 0;
};

//
//ClassAction
//

//An action which code creates and returns an object of a C++ class.
class synbin::ClassAction : public AbstractClassAction {
	NONCOPYABLE(ClassAction);

public:
	ClassAction(
		util::MPtr<const ClassTypeDescriptor> class_type,
		std::unique_ptr<const ActionDefs::AttributeVector> attributes,
		std::unique_ptr<const ActionDefs::PartClassVector> part_classes,
		std::unique_ptr<const ActionDefs::ClassVector> classes);

	util::MPtr<const TypeDescriptor> get_result_type() const override;

	void visit(ActionVisitor* visitor) const override;

private:
	void print_header(std::ostream& out) const override;
};

//
//PartClassAction
//

//An action that produces a part-object.
class synbin::PartClassAction : public AbstractClassAction {
	NONCOPYABLE(PartClassAction);

	const util::MPtr<const PartClassTypeDescriptor> m_part_class_type;

public:
	PartClassAction(
		util::MPtr<const PartClassTypeDescriptor> part_class_type,
		std::unique_ptr<const ActionDefs::AttributeVector> attributes,
		std::unique_ptr<const ActionDefs::PartClassVector> part_classes,
		std::unique_ptr<const ActionDefs::ClassVector> classes);

	util::MPtr<const TypeDescriptor> get_result_type() const override;
	util::MPtr<const ClassTypeDescriptor> get_part_class_type_class() const override;

	void visit(ActionVisitor* visitor) const override;

private:
	void print_header(std::ostream& out) const override;
};

//
//ResultAndAction
//

//Same as CopyAction, but returns the value of an arbitrary production element, not only 0-th.
class synbin::ResultAndAction : public AbstractAction<TypeDescriptor> {
	NONCOPYABLE(ResultAndAction);

	const std::size_t m_result_index;

public:
	ResultAndAction(util::MPtr<const TypeDescriptor> type, std::size_t result_index);

	std::size_t get_result_index() const;

	void visit(ActionVisitor* visitor) const override;
	void print(std::ostream& out) const override;
};

//
//ListAction
//

//Common base class for list actions.
class synbin::ListAction : public AbstractAction<ListTypeDescriptor> {
	NONCOPYABLE(ListAction);

protected:
	ListAction(util::MPtr<const ListTypeDescriptor> list_type);
};

//
//FirstListAction
//

//Action that handles a terminal list production (i. e. A : B).
class synbin::FirstListAction : public ListAction {
	NONCOPYABLE(FirstListAction);

public:
	FirstListAction(util::MPtr<const ListTypeDescriptor> list_type);

	void visit(ActionVisitor* visitor) const override;
	void print(std::ostream& out) const override;
};

//
//NextListAction
//

//Action that handles a recursive list production (i. e. A : A B).
class synbin::NextListAction : public ListAction {
	NONCOPYABLE(NextListAction);

	//true, if the list has a separator.
	const bool m_separator;

public:
	NextListAction(util::MPtr<const ListTypeDescriptor> list_type, bool separator);

	bool is_separator() const;

	void visit(ActionVisitor* visitor) const override;
	void print(std::ostream& out) const override;
};

//
//ConstAction
//

//Action returning a constant value.
class synbin::ConstAction : public AbstractAction<PrimitiveTypeDescriptor> {
	NONCOPYABLE(ConstAction);

	const ebnf::ConstExpression* const m_const_expr;

public:
	ConstAction(util::MPtr<const PrimitiveTypeDescriptor> type, const ebnf::ConstExpression* const_expr);

	const ebnf::ConstExpression* get_const_expr() const;

	void visit(ActionVisitor* visitor) const override;
	void print(std::ostream& out) const override;
};

//
//ActionVisitor
//

//Action visitor.
class synbin::ActionVisitor {
	NONCOPYABLE(ActionVisitor);

protected:
	ActionVisitor(){}

public:
	virtual void visit_VoidAction(const VoidAction* action) = 0;
	virtual void visit_CopyAction(const CopyAction* action) = 0;
	virtual void visit_CastAction(const CastAction* action) = 0;
	virtual void visit_ClassAction(const ClassAction* action) = 0;
	virtual void visit_PartClassAction(const PartClassAction* action) = 0;
	virtual void visit_ResultAndAction(const ResultAndAction* action) = 0;
	virtual void visit_FirstListAction(const FirstListAction* action) = 0;
	virtual void visit_NextListAction(const NextListAction* action) = 0;
	virtual void visit_ConstAction(const ConstAction* action) = 0;
};

#endif//SYN_CORE_ACTION_H_INCLUDED
