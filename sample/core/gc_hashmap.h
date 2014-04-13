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

//GC Hashmap.

#ifndef SYNSAMPLE_CORE_GC_HASHMAP_H_INCLUDED
#define SYNSAMPLE_CORE_GC_HASHMAP_H_INCLUDED

#include <cassert>

#include "common.h"
#include "gc.h"

namespace syn_script {
	namespace gc {

		template<class K> using HashCodeFn = std::size_t(const gc::Local<K>&);
		template<class K> using EqualsFn = bool(const gc::Local<K>&, const gc::Local<K>&);

		class BasicHashMap;

		template<class K, class V, HashCodeFn<K> HashFn, EqualsFn<K> EqFn>
		class BasicGenericHashMap;

	}
}

//
//BasicHashMap
//

class syn_script::gc::BasicHashMap : public gc::Object {
	NONCOPYABLE(BasicHashMap);

	static const float LOAD_FACTOR;

	class Entry : public gc::Object {
		NONCOPYABLE(Entry);

		gc::Ref<Entry> m_next;
		gc::Ref<Object> m_key;
		gc::Ref<Object> m_value;

	public:
		Entry(){}
		void gc_enumerate_refs() override;

		void initialize(
			const gc::Local<Entry>& next,
			const gc::Local<Object>& key,
			const gc::Local<Object>& value);

		const gc::Ref<Entry>& get_next() const;
		const gc::Ref<Object>& get_key() const;
		const gc::Ref<Object>& get_value() const;

		void set_next(const gc::Local<Entry>& next);
		void set_value(const gc::Local<Object>& value);
	};

	std::size_t m_size;
	std::size_t m_threshold;
	gc::Ref<gc::Array<Entry>> m_table;

protected:
	BasicHashMap(){}
	void gc_enumerate_refs() override;

public:
	void initialize();

	bool is_empty() const;
	std::size_t size() const;
	void clear();

protected:
	class BasicIterator {
		friend class BasicHashMap;

		const gc::Local<const BasicHashMap> m_map;
		std::size_t m_index;
		gc::Local<Entry> m_entry;

	private:
		BasicIterator(const gc::Local<const BasicHashMap>& map);

	public:
		bool end() const;
		void next();
		gc::Local<Object> key() const;
		gc::Local<Object> value() const;

	private:
		void next_index();
	};

	BasicIterator basic_iterator() const;

	bool contains_0(const gc::Local<Object>& key) const;
	gc::Local<Object> get_0(const gc::Local<Object>& key) const;
	gc::Local<Object> put_0(const gc::Local<Object>& key, const gc::Local<Object>& value);
	gc::Local<Object> remove_0(const gc::Local<Object>& key);

	virtual std::size_t key_hash_code(const gc::Local<Object>& key) const = 0;
	virtual bool key_equals(const gc::Local<Object>& key1, const gc::Local<Object>& key2) const = 0;

private:
	void expand();
	gc::Local<Entry> find_entry(const gc::Local<Object>& key, gc::Local<Entry>& prev, std::size_t& index) const;
};

//
//BasicGenericHashMap
//

template<class K, class V, syn_script::gc::HashCodeFn<K> HashFn, syn_script::gc::EqualsFn<K> EqFn>
class syn_script::gc::BasicGenericHashMap : public BasicHashMap {
	NONCOPYABLE(BasicGenericHashMap);

public:
	class Iterator {
		friend class BasicGenericHashMap<K, V, HashFn, EqFn>;

		BasicIterator m_iter;

	private:
		Iterator(const BasicIterator& iter) : m_iter(iter){}

	public:
		bool end() const {
			return m_iter.end();
		}

		void next() {
			m_iter.next();
		}

		gc::Local<K> key() const {
			return m_iter.key().stat_cast<K>();
		}

		gc::Local<V> value() const {
			return m_iter.value().stat_cast<V>();
		}
	};

public:
	BasicGenericHashMap(){}

	bool contains(const gc::Local<K>& key) const {
		return contains_0(key);
	}

	gc::Local<V> get(const gc::Local<K>& key) const {
		gc::Local<Object> value0 = get_0(key);
		return value0.stat_cast<V>();
	}

	gc::Local<V> put(const gc::Local<K>& key, const gc::Local<V>& value) {
		gc::Local<Object> old_value0 = put_0(key, value);
		return old_value0.stat_cast<V>();
	}

	gc::Local<V> remove(const gc::Local<K>& key) {
		gc::Local<Object> value0 = remove_0(key);
		return value0.stat_cast<V>();
	}

	Iterator iterator() const {
		return Iterator(basic_iterator());
	}

private:
	template<class T, gc::Local<Object> (BasicIterator::*Fn)() const>
	gc::Local<gc::Array<T>> elements() const {
		std::size_t cnt = size();
		gc::Local<gc::Array<T>> array = gc::Array<T>::create(cnt);

		BasicIterator iter = basic_iterator();
		while (cnt && !iter.end()) {
			--cnt;
			(*array)[cnt] = (iter.*Fn)().stat_cast<T>();
			iter.next();
		}

		assert(!cnt);
		assert(iter.end());

		return array;
	}

public:
	gc::Local<gc::Array<K>> keys() const {
		return elements<K, &BasicIterator::key>();
	}

	gc::Local<gc::Array<V>> values() const {
		return elements<V, &BasicIterator::value>();
	}

protected:
	std::size_t key_hash_code(const gc::Local<Object>& key) const override {
		return HashFn(key.stat_cast<K>());
	}

	bool key_equals(const gc::Local<Object>& key1, const gc::Local<Object>& key2) const override {
		return EqFn(key1.stat_cast<K>(), key2.stat_cast<K>());
	}
};

#endif//SYNSAMPLE_CORE_GC_HASHMAP_H_INCLUDED
