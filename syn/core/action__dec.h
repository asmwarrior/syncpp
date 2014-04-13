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

//Declarations of action classes.

#ifndef SYN_CORE_ACTION_DEC_H_INCLUDED
#define SYN_CORE_ACTION_DEC_H_INCLUDED

#include <cstddef>
#include <vector>

#include "descriptor_type.h"
#include "util_mptr.h"
#include "util_string.h"

namespace synbin {

	class Action;
	template<class T> class AbstractAction;
	class VoidAction;
	class CopyAction;
	class CastAction;
	class AbstractClassAction;
	class ClassAction;
	class PartClassAction;
	class ResultAndAction;
	class ListAction;
	class FirstListAction;
	class NextListAction;
	class ConstAction;

	class ActionVisitor;

	struct ActionDefs {
		struct Field {
			std::size_t m_offset;

			Field(std::size_t offset) : m_offset(offset){}
		};
		
		struct AttributeField : public Field {
			const util::String m_name;

			AttributeField(std::size_t offset, const util::String& name) : Field(offset), m_name(name){}
		};
		
		struct PartClassField : public Field {
			const util::MPtr<const PartClassTypeDescriptor> m_part_class_type;

			PartClassField(std::size_t offset, util::MPtr<const PartClassTypeDescriptor> part_class_type)
				: Field(offset), m_part_class_type(part_class_type)
			{}
		};
		
		struct ClassField : public Field {
			ClassField(std::size_t offset) : Field(offset){}
		};
		
		typedef std::vector<AttributeField> AttributeVector;
		typedef std::vector<PartClassField> PartClassVector;
		typedef std::vector<ClassField> ClassVector;
	};

}

#endif//SYN_CORE_ACTION_DEC_H_INCLUDED
