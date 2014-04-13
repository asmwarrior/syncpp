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

//Names handling.

#include <list>
#include <map>
#include <mutex>
#include <string>

#include "common.h"
#include "gc.h"
#include "gc_vector.h"
#include "name.h"
#include "stringex.h"

namespace ss = syn_script;
namespace gc = ss::gc;

//
//NameID
//

ss::NameID::NameID(std::size_t name_id)
: m_name_id(name_id)
{
	assert(0 != BAD_ID);
	assert(BAD_ID != name_id);
}

bool ss::NameID::operator!() const {
	return BAD_ID == m_name_id;
}

bool ss::NameID::operator==(const NameID& b) const {
	return m_name_id == b.m_name_id;
}

bool ss::NameID::operator<(const NameID& b) const {
	return m_name_id < b.m_name_id;
}

//
//NameInfo
//

void ss::NameInfo::gc_enumerate_refs() {
	Object::gc_enumerate_refs();
	gc_ref(m_str);
}

void ss::NameInfo::initialize(const NameID& id, const StringLoc& str) {
	m_id = id;
	m_str = str;
}

const ss::NameID& ss::NameInfo::get_id() const {
	return m_id;
}

const ss::StringRef& ss::NameInfo::get_str() const {
	return m_str;
}

//
//NameTable::Internal
//

class ss::NameTable::Internal {
	NONCOPYABLE(Internal);

	std::mutex m_mutex;
	gc::Local<gc::Vector<const NameInfo>> m_id_to_info;
	std::map<StringKeyValue, std::size_t> m_name_to_id;
	std::list<GCPtrStringKey> m_owned_keys;
	gc::Local<gc::Vector<const String>> m_owned_names;

public:
	Internal()
		: m_id_to_info(gc::create<gc::Vector<const NameInfo>>()),
		m_owned_names(gc::create<gc::Vector<const String>>())
	{}

private:
	gc::Local<const NameInfo> lookup_name(const StringKey& key) const {
		auto iter = m_name_to_id.find(StringKeyValue(key));
		if (iter != m_name_to_id.end()) {
			std::size_t id = iter->second;
			return m_id_to_info->get(id);
		}
		return nullptr;
	}

	gc::Local<const NameInfo> do_register_name(const StringLoc& name) {
		std::size_t name_id = m_id_to_info->size();
		gc::Local<const NameInfo> info = gc::create<NameInfo>(NameID(name_id), name);

		m_owned_names->add(name);
		m_owned_keys.push_back(GCPtrStringKey(name.get()));
		m_name_to_id[StringKeyValue(m_owned_keys.back())] = name_id;
		m_id_to_info->add(info);

		return info;
	}

public:
	gc::Local<const NameInfo> register_name(
		const StringIterator& start_pos,
		const StringIterator& end_pos)
	{
		GCPtrStringKey key = start_pos.get_string_key(end_pos);
		gc::Local<const NameInfo> info = lookup_name(key);
		if (!!info) return info;

		StringLoc name = start_pos.get_string(end_pos);
		return do_register_name(name);
	}

	gc::Local<const NameInfo> register_name(const StringLoc& name) {
		GCStringKey key(name);
		gc::Local<const NameInfo> info = lookup_name(key);
		if (!!info) return info;
		return do_register_name(name);
	}

	gc::Local<const NameInfo> register_name(const std::string& str) {
		StdStringKey key(str);
		gc::Local<const NameInfo> info = lookup_name(key);
		if (!!info) return info;

		StringLoc name = gc::create<String>(str);
		return do_register_name(name);
	}

	std::mutex& get_mutex() {
		return m_mutex;
	}
};

//
//NameTable
//

ss::NameTable::NameTable()
: m_internal(new Internal())
{}

ss::NameTable::~NameTable() {
	delete m_internal;
}

//
//NameRegistry::Internal
//

class ss::NameRegistry::Internal {
	NONCOPYABLE(Internal);

	NameTable& m_name_table;
	std::lock_guard<std::mutex> m_name_table_lock;

public:
	Internal(NameTable& name_table)
		: m_name_table(name_table),
		m_name_table_lock(name_table.m_internal->get_mutex())
	{}

	gc::Local<const NameInfo> register_name(const StringLoc& name) {
		return m_name_table.m_internal->register_name(name);
	}

	gc::Local<const NameInfo> register_name(const std::string& str) {
		return m_name_table.m_internal->register_name(str);
	}

	gc::Local<const NameInfo> register_name(
		const StringIterator& start_pos,
		const StringIterator& end_pos)
	{
		return m_name_table.m_internal->register_name(start_pos, end_pos);
	}
};

//
//NameRegistry
//

ss::NameRegistry::NameRegistry(NameTable& name_table)
: m_internal(new Internal(name_table))
{}

ss::NameRegistry::~NameRegistry() {
	delete m_internal;
}

gc::Local<const ss::NameInfo> ss::NameRegistry::register_name(const StringLoc& name) {
	return m_internal->register_name(name);
}

gc::Local<const ss::NameInfo> ss::NameRegistry::register_name(const std::string& str) {
	return m_internal->register_name(str);
}

gc::Local<const ss::NameInfo> ss::NameRegistry::register_name(
	const StringIterator& start_pos,
	const StringIterator& end_pos)
{
	return m_internal->register_name(start_pos, end_pos);
}
