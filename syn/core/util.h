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

//Useful utilities.

#ifndef SYN_CORE_UTIL_H_INCLUDED
#define SYN_CORE_UTIL_H_INCLUDED

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "noncopyable.h"

namespace synbin {
	namespace util {

		//
		//AssignOnce
		//

		//Wrapper for an arbitrary variable, which allows it to be changed only once at run-time.
		template<class T>
		class AssignOnce {
			NONCOPYABLE(AssignOnce);

			bool m_set;
			T m_value;

		public:
			AssignOnce() : m_set(false){}

			void set(T value) {
				assert(!m_set);
				m_value = value;
				m_set = true;
			}

			T get() const {
				assert(m_set);
				return m_value;
			}

			bool is_defined() const { return m_set; }
		};

		//
		//AssignOncePtr
		//

		//Same as AssignOnce, but for a pointer type. The value cannot be null.
		template<class T>
		class AssignOncePtr {
			NONCOPYABLE(AssignOncePtr);

			T* m_value;

		public:
			AssignOncePtr() : m_value(nullptr){}

			void set(T* value) {
				assert(value);
				assert(!m_value);
				m_value = value;
			}

			T* get() const {
				assert(m_value);
				return m_value;
			}

			T* get_opt() const { return m_value; }
		};

		template<class T>
		std::unique_ptr<const std::vector<T>> const_vector_ptr(std::unique_ptr<std::vector<T>> ptr) {
			return std::unique_ptr<const std::vector<T>>(std::move(ptr));
		}

		template<class Map, class Fn>
		void for_each(const Map& map, Fn fn) {
			for (const typename Map::value_type& value : map) fn(value.second);
		}

		//
		//IndexedMap
		//

		template<class K, class V, class Idx>
		class IndexedMap {
			NONCOPYABLE(IndexedMap);

		public:
			class const_iterator;

		private:
			friend class const_iterator;

			const Idx m_idx_fn;
			const std::size_t m_max_size;
			std::vector<std::size_t> m_index_to_offset;
			std::vector<std::pair<K, V>> m_entries;
			std::size_t m_size;

			inline std::size_t key_to_index(const K& key) const {
				std::size_t index = m_idx_fn(key);
				assert(index < m_max_size);
				return index;
			}

		public:
			class const_iterator {
				friend class IndexedMap<K, V, Idx>;

				const IndexedMap<K, V, Idx>* m_map;
				std::size_t m_offset;

				const_iterator(const IndexedMap<K, V, Idx>* map, std::size_t offset)
					: m_map(map),
					m_offset(offset)
				{}

				inline std::size_t fix_offset() const {
					assert(m_map);
					return m_offset < m_map->m_size ? m_offset : m_map->m_max_size;
				}

			public:
				const_iterator()
					: m_map(nullptr),
					m_offset(0)
				{}

				bool operator==(const const_iterator& iter) const {
					assert(m_map == iter.m_map);
					return fix_offset() == iter.fix_offset();
				}

				bool operator!=(const const_iterator& iter) const {
					return !(*this == iter);
				}

				bool operator<(const const_iterator& iter) const {
					assert(m_map == iter.m_map);
					return fix_offset() < iter.fix_offset();
				}

				bool operator<=(const const_iterator& iter) const {
					assert(m_map == iter.m_map);
					return fix_offset() <= iter.fix_offset();
				}
				
				bool operator>(const const_iterator& iter) const {
					return !(*this <= iter);
				}

				bool operator>=(const const_iterator& iter) const {
					return !(*this < iter);
				}

				const_iterator& operator++() {
					assert(m_map);
					++m_offset;
					return *this;
				}

				const_iterator operator++(int) {
					assert(m_map);
					const_iterator result = *this;
					++m_offset;
					return result;
				}

				const K& operator*() const {
					assert(m_map);
					assert(m_offset < m_map->m_size);
					return m_map->m_entries[m_offset].first;
				}

				const V& value() const {
					assert(m_map);
					assert(m_offset < m_map->m_size);
					return m_map->m_entries[m_offset].second;
				}
			};

			IndexedMap(std::size_t max_size, Idx idx_fn = Idx())
				: m_idx_fn(idx_fn),
				m_max_size(max_size),
				m_index_to_offset(max_size, max_size),
				m_entries(max_size),
				m_size(0)
			{}

			std::size_t size() const {
				return m_size;
			}

			std::size_t max_size() const {
				return m_max_size;
			}

			bool empty() const {
				return !m_size;
			}

			bool put(const K& key, const V& value) {
				std::size_t index = key_to_index(key);
				std::size_t offset = m_index_to_offset[index];
				if (offset != m_max_size) {
					m_entries[offset].second = value;
					return false;
				} else {
					m_index_to_offset[index] = m_size;
					std::pair<K, V>& entry = m_entries[m_size];
					entry.first = key;
					entry.second = value;
					++m_size;
					return true;
				}
			}

			bool remove(const K& key) {
				std::size_t index = key_to_index(key);
				std::size_t offset = m_index_to_offset[index];
				if (offset != m_max_size) {
					std::pair<K, V>& last_entry = m_entries[m_size - 1];
					if (offset < m_size - 1) {
						std::size_t last_entry_index = key_to_index(last_entry.first);
						m_index_to_offset[last_entry_index] = offset;
						m_entries[offset] = last_entry;
					}
					last_entry.first = K();
					last_entry.second = V();
					m_index_to_offset[index] = m_max_size;
					--m_size;
					return true;
				} else {
					return false;
				}
			}

			bool contains(const K& key) const {
				std::size_t index = key_to_index(key);
				return m_max_size != index;
			}

			const_iterator begin() const {
				return const_iterator(this, 0);
			}

			const_iterator end() const {
				return const_iterator(this, m_size);
			}

			const_iterator max_end() const {
				return const_iterator(this, m_max_size);
			}

			const_iterator find(const K& key) const {
				const_iterator result;
				if (find(key, &result)) {
					return result;
				}
				return max_end();
			}

			bool find(const K& key, const_iterator* result) const {
				std::size_t index = key_to_index(key);
				std::size_t offset = m_index_to_offset[index];
				if (m_max_size == offset) {
					return false;
				}
				*result = const_iterator(this, offset);
				return true;
			}

			V get(const K& key) const {
				const_iterator iter;
				if (find(key, &iter)) {
					return m_entries[iter.m_offset].second;
				}
				return V();
			}

			bool get(const K& key, V* value) const {
				const_iterator iter;
				if (find(key, &iter)) {
					*value = m_entries[iter.m_offset].second;
					return true;
				}
				return false;
			}

			const V& operator[](std::size_t index) const {
				assert(index < m_size);
				return m_entries[index].second;
			}

			V& operator[](std::size_t index) {
				assert(index < m_size);
				return m_entries[index].second;
			}

			const K& key_at(std::size_t index) const {
				assert(index < m_size);
				return m_entries[index].first;
			}

			K& key_at(std::size_t index) {
				assert(index < m_size);
				return m_entries[index].first;
			}
		};

		//
		//IndexedSet
		//

		template<class T, class Idx>
		class IndexedSet {
			NONCOPYABLE(IndexedSet);

			IndexedMap<T, bool, Idx> m_map;

		public:
			typedef typename IndexedMap<T, bool, Idx>::const_iterator const_iterator;

			IndexedSet(std::size_t max_size, Idx idx_fn = Idx())
				: m_map(max_size, idx_fn)
			{}

			std::size_t size() const {
				return m_map.size();
			}

			std::size_t max_size() const {
				return m_map.max_size();
			}

			bool empty() const {
				return m_map.empty();
			}

			bool add(const T& value) {
				return m_map.put(value, true);
			}

			bool remove(const T& value) {
				return m_map.remove(value);
			}

			bool contains(const T& value) const {
				return m_map.contains(value);
			}

			const_iterator begin() const {
				return m_map.begin();
			}

			const_iterator end() const {
				return m_map.end();
			}

			const_iterator max_end() const {
				return m_map.max_end();
			}

			const_iterator find(const T& value) const {
				return m_map.find(value);
			}

			bool find(const T& value, const_iterator* result) const {
				return m_map.find(value, result);
			}

			const T& operator[](std::size_t index) const {
				return m_map.key_at(index);
			}

			T& operator[](std::size_t index) {
				return m_map.key_at(index);
			}
		};
	}
}

#endif//SYN_CORE_UTIL_H_INCLUDED
