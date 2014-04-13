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

//Implementation of action factory classes.

#include <cassert>
#include <memory>
#include <utility>

#include "action.h"
#include "action_factory.h"
#include "commons.h"
#include "descriptor_type.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace util = ns::util;

using std::unique_ptr;
using util::MPtr;
using util::MContainer;

//
//VoidActionFactory
//

MPtr<const ns::Action> ns::VoidActionFactory::create_action(Container* container, const TypeProduction* production) {
	MPtr<const VoidTypeDescriptor> void_type = container->get_void_type();
	MPtr<const Action> action = container->manage_action(make_unique1<VoidAction>(void_type));
	return action;
}

//
//CopyActionFactory
//

MPtr<const ns::Action> ns::CopyActionFactory::create_action(Container* container, const TypeProduction* production) {
	assert(1 == production->size());
	MPtr<const TypeDescriptor> type = production->get(0);
	assert(!type->is_void());
	MPtr<const Action> action = container->manage_action(make_unique1<CopyAction>(type));
	return action;
}

//
//CastActionFactory
//

ns::CastActionFactory::CastActionFactory(MPtr<const ClassTypeDescriptor> cast_type)
	: m_cast_type(cast_type)
{}

MPtr<const ns::Action> ns::CastActionFactory::create_action(Container* type_container, const TypeProduction* production) {
	assert(1 == production->size());
	MPtr<const TypeDescriptor> actual_type = production->get(0);
	assert(!actual_type->is_void());
	
	const ClassTypeDescriptor* actual_class_type0 = actual_type->as_class_type();
	assert(actual_class_type0);
	MPtr<const ClassTypeDescriptor> actual_class_type = MPtr<const ClassTypeDescriptor>::unsafe_cast(actual_class_type0);
	
	MPtr<const Action> action = type_container->manage_action(make_unique1<CastAction>(m_cast_type, actual_class_type));
	return action;
}

//
//AbstractClassActionFactory
//

ns::AbstractClassActionFactory::AbstractClassActionFactory(
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes)
	: m_attributes(std::move(attributes)),
	m_part_classes(std::move(part_classes)),
	m_classes(std::move(classes)),
	m_action_created(false)
{}

MPtr<const ns::Action> ns::AbstractClassActionFactory::create_action(
	Container* type_container,
	const TypeProduction* production)
{
	//For efficiency reasons, this factory can create only one action object.
	//The factory gives owned vectors to the created action, and thus becomes invalid then.
	assert(!m_action_created);

	MPtr<const Action> action = create_action0(
		type_container, production, std::move(m_attributes), std::move(m_part_classes), std::move(m_classes));

	m_action_created = true;
	return action;
}

//
//ClassActionFactory
//

ns::ClassActionFactory::ClassActionFactory(
	MPtr<const ClassTypeDescriptor> class_type,
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes)
	: AbstractClassActionFactory(std::move(attributes), std::move(part_classes), std::move(classes)),
	m_class_type(class_type)
{}

MPtr<const ns::Action> ns::ClassActionFactory::create_action0(
	Container* type_container,
	const TypeProduction* production,
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes) const
{
	return type_container->manage_action(make_unique1<ClassAction>(m_class_type, std::move(attributes), std::move(part_classes), std::move(classes)));
}

//
//PartClassActionFactory
//

ns::PartClassActionFactory::PartClassActionFactory(
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes,
	MPtr<const PartClassTypeDescriptor> part_class_type)
	: AbstractClassActionFactory(std::move(attributes), std::move(part_classes), std::move(classes)),
	m_part_class_type(part_class_type)
{}

MPtr<const ns::Action> ns::PartClassActionFactory::create_action0(
	Container* type_container,
	const TypeProduction* production,
	unique_ptr<const AttributeVector> attributes,
	unique_ptr<const PartClassVector> part_classes,
	unique_ptr<const ClassVector> classes) const
{
	return type_container->manage_action(make_unique1<PartClassAction>(
		m_part_class_type,
		std::move(attributes),
		std::move(part_classes),
		std::move(classes)));
}

//
//ResultAndActionFactory
//

ns::ResultAndActionFactory::ResultAndActionFactory(std::size_t index)
	: m_index(index)
{}

MPtr<const ns::Action> ns::ResultAndActionFactory::create_action(Container* type_container, const TypeProduction* production) {
	assert(m_index < production->size());
	MPtr<const TypeDescriptor> type = production->get(m_index);
	assert(!type->is_void());
	MPtr<const Action> action = type_container->manage_action(make_unique1<ResultAndAction>(type, m_index));
	return action;
}

//
//ListActionFactory
//

ns::ListActionFactory::ListActionFactory(MPtr<const ListTypeDescriptor> type)
	: m_type(type)
{}

//
//FirstListActionFactory
//

ns::FirstListActionFactory::FirstListActionFactory(MPtr<const ListTypeDescriptor> type)
	: ListActionFactory(type)
{}

MPtr<const ns::Action> ns::FirstListActionFactory::create_action(Container* type_container, const TypeProduction* production) {
	assert(1 == production->size());
	assert(production->get(0)->equals(m_type->get_element_type().get()));
	return type_container->manage_action(make_unique1<FirstListAction>(m_type));
}

//
//NextListActionFactory
//

ns::NextListActionFactory::NextListActionFactory(MPtr<const ListTypeDescriptor> type, bool separator)
	: ListActionFactory(type),
	m_separator(separator)
{}

MPtr<const ns::Action> ns::NextListActionFactory::create_action(Container* type_container, const TypeProduction* production) {
	assert(production->get(0)->equals(m_type.get()));
	assert((m_separator ? 3 : 2) == production->size());
	assert(production->get(production->size() - 1)->equals(m_type->get_element_type().get()));
	return type_container->manage_action(make_unique1<NextListAction>(m_type, m_separator));
}

//
//ConstActionFactory
//

ns::ConstActionFactory::ConstActionFactory(
	MPtr<const PrimitiveTypeDescriptor> type,
	const ebnf::ConstExpression* const_expr)
	: m_type(type),
	m_const_expr(const_expr)
{
	assert(!m_type->is_void());
	assert(m_const_expr);
}

MPtr<const ns::Action> ns::ConstActionFactory::create_action(Container* type_container, const TypeProduction* production) {
	assert(0 == production->size());
	return type_container->manage_action(make_unique1<ConstAction>(m_type, m_const_expr));
}
