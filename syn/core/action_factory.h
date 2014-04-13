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

//Definitions of classes used to construct action classes (action factory classes).

#ifndef SYN_CORE_ACTION_FACTORY_H_INCLUDED
#define SYN_CORE_ACTION_FACTORY_H_INCLUDED

#include <cstddef>
#include <memory>
#include <utility>

#include "action__dec.h"
#include "descriptor_type.h"
#include "noncopyable.h"
#include "util_mptr.h"

namespace synbin {

	class ActionFactory;
	class VoidActionFactory;
	class CopyActionFactory;
	class CastActionFactory;
	class AbstractClassActionFactory;
	class ClassActionFactory;
	class PartClassActionFactory;
	class ResultAndActionFactory;
	class ListActionFactory;
	class FirstListActionFactory;
	class NextListActionFactory;
	class ConstActionFactory;

}

//
//ActionFactory
//

//Abstract base class for action factories. See comments for corresponding action types.
class synbin::ActionFactory {
	NONCOPYABLE(ActionFactory);

protected:
	ActionFactory(){}

public:
	virtual ~ActionFactory(){}

	//An interface of a container object, used for memory management.
	class Container {
		NONCOPYABLE(Container);

	protected:
		Container(){}

	public:
		//Returns the pointer to the void type descriptor singleton.
		virtual util::MPtr<const VoidTypeDescriptor> get_void_type() = 0;

		//Adds the passed action into a container and returns its MPtr.
		virtual util::MPtr<const Action> manage_action(std::unique_ptr<Action> action) = 0;
	};

	//An interface used to get types of production elements. 
	class TypeProduction {
		NONCOPYABLE(TypeProduction);

	protected:
		TypeProduction(){}

	public:
		virtual std::size_t size() const = 0;
		virtual util::MPtr<const TypeDescriptor> get(std::size_t index) const = 0;
	};

	//Creates an instance of an action.
	virtual util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) = 0;
};

//
//VoidActionFactory
//

class synbin::VoidActionFactory : public ActionFactory {
	NONCOPYABLE(VoidActionFactory);

public:
	VoidActionFactory(){}

	util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) override;
};

//
//CopyActionFactory
//

class synbin::CopyActionFactory : public ActionFactory {
	NONCOPYABLE(CopyActionFactory);

public:
	CopyActionFactory(){}

	util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) override;
};

//
//CastActionFactory
//

class synbin::CastActionFactory : public ActionFactory {
	NONCOPYABLE(CastActionFactory);

	const util::MPtr<const ClassTypeDescriptor> m_cast_type;

public:
	CastActionFactory(util::MPtr<const ClassTypeDescriptor> cast_type);

	util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) override;
};

//
//AbstractClassActionFactory
//

class synbin::AbstractClassActionFactory : public ActionFactory {
	NONCOPYABLE(AbstractClassActionFactory);

public:
	typedef ActionDefs::AttributeVector AttributeVector;
	typedef ActionDefs::PartClassVector PartClassVector;
	typedef ActionDefs::ClassVector ClassVector;

private:
	std::unique_ptr<const AttributeVector> m_attributes;
	std::unique_ptr<const PartClassVector> m_part_classes;
	std::unique_ptr<const ClassVector> m_classes;
	bool m_action_created;

protected:
	AbstractClassActionFactory(
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes);

	util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) override;

private:
	virtual util::MPtr<const Action> create_action0(
		Container* type_container,
		const TypeProduction* production,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes) const = 0;
};

//
//ClassActionFactory
//

class synbin::ClassActionFactory : public AbstractClassActionFactory {
	NONCOPYABLE(ClassActionFactory);

	const util::MPtr<const ClassTypeDescriptor> m_class_type;

public:
	ClassActionFactory(
		util::MPtr<const ClassTypeDescriptor> class_type,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes);

private:
	util::MPtr<const Action> create_action0(
		Container* type_container,
		const TypeProduction* production,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes) const override;
};

//
//PartClassActionFactory
//

class synbin::PartClassActionFactory : public AbstractClassActionFactory {
	NONCOPYABLE(PartClassActionFactory);

	const util::MPtr<const PartClassTypeDescriptor> m_part_class_type;

public:
	PartClassActionFactory(
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes,
		util::MPtr<const PartClassTypeDescriptor> part_class_type);

private:
	util::MPtr<const Action> create_action0(
		Container* type_container,
		const TypeProduction* production,
		std::unique_ptr<const AttributeVector> attributes,
		std::unique_ptr<const PartClassVector> part_classes,
		std::unique_ptr<const ClassVector> classes) const override;
};

//
//ResultAndActionFactory
//

class synbin::ResultAndActionFactory : public ActionFactory {
	NONCOPYABLE(ResultAndActionFactory);

	const std::size_t m_index;

public:
	ResultAndActionFactory(std::size_t index);

	util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) override;
};

//
//ListActionFactory
//

class synbin::ListActionFactory : public ActionFactory {
	NONCOPYABLE(ListActionFactory);

protected:
	const util::MPtr<const ListTypeDescriptor> m_type;

protected:
	ListActionFactory(util::MPtr<const ListTypeDescriptor> type);
};

//
//FirstListActionFactory
//

class synbin::FirstListActionFactory : public ListActionFactory {
	NONCOPYABLE(FirstListActionFactory);

public:
	FirstListActionFactory(util::MPtr<const ListTypeDescriptor> type);

	util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) override;
};

//
//NextListActionFactory
//

class synbin::NextListActionFactory : public ListActionFactory {
	NONCOPYABLE(NextListActionFactory);

	const bool m_separator;

public:
	NextListActionFactory(util::MPtr<const ListTypeDescriptor> type, bool separator);

	util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) override;
};

//
//ConstActionFactory
//

class synbin::ConstActionFactory : public ActionFactory {
	NONCOPYABLE(ConstActionFactory);

	const util::MPtr<const PrimitiveTypeDescriptor> m_type;
	const ebnf::ConstExpression* const m_const_expr;

public:
	ConstActionFactory(util::MPtr<const PrimitiveTypeDescriptor> type, const ebnf::ConstExpression* const_expr);

	util::MPtr<const Action> create_action(Container* type_container, const TypeProduction* production) override;
};

#endif//SYN_CORE_ACTION_FACTORY_H_INCLUDED
