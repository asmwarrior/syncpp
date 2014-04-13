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

//Names processing.

#ifndef SYNSAMPLE_CORE_NAME_H_INCLUDED
#define SYNSAMPLE_CORE_NAME_H_INCLUDED

#include <string>

#include "common.h"
#include "gc.h"
#include "name__dec.h"
#include "stringex.h"

namespace syn_script {

	//
	//NameID
	//

	//Name identifier. Instead of using strings, numeric IDs are used to identify names - for efficiency reasons.
	class NameID {
		static const std::size_t BAD_ID = SIZE_MAX;

		std::size_t m_name_id;

	public:
		NameID(){}
		explicit NameID(std::size_t name_id);

		bool operator!() const;
		bool operator==(const NameID& b) const;
		bool operator<(const NameID& b) const;
	};

	//
	//NameInfo
	//

	class NameInfo : public gc::Object {
		NONCOPYABLE(NameInfo);

		NameID m_id;
		StringRef m_str;

	public:
		NameInfo(){}
		void gc_enumerate_refs() override;
		void initialize(const NameID& id, const StringLoc& str);

		const NameID& get_id() const;
		const StringRef& get_str() const;
	};

	//
	//NameTable
	//

	class NameTable {
		NONCOPYABLE(NameTable);

		friend class NameRegistry;
		class Internal;

		Internal* const m_internal;

	public:
		NameTable();
		~NameTable();
	};

	//
	//NameRegistry
	//

	class NameRegistry {
		NONCOPYABLE(NameRegistry);

		class Internal;
		Internal* const m_internal;

	public:
		explicit NameRegistry(NameTable& name_table);
		~NameRegistry();

		gc::Local<const NameInfo> register_name(const StringLoc& name);

		gc::Local<const NameInfo> register_name(const std::string& str);

		gc::Local<const NameInfo> register_name(
			const StringIterator& start_pos,
			const StringIterator& end_pos);
	};

}

#endif//SYNSAMPLE_CORE_NAME_H_INCLUDED
