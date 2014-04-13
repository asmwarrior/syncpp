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

//Definition of GC String class.

#ifndef SYNSAMPLE_CORE_STRINGEX_H_INCLUDED
#define SYNSAMPLE_CORE_STRINGEX_H_INCLUDED

#include <atomic>
#include <cassert>
#include <memory>
#include <ostream>
#include <string>

#include "common.h"
#include "gc.h"
#include "stringex__dec.h"

namespace syn_script {

	StringLoc operator+(const StringLoc& a, const StringLoc& b);
	std::ostream& operator<<(std::ostream& out, const StringLoc& str);

	//
	//String
	//

	class String : public ObjectEx {
		NONCOPYABLE(String);

		friend StringLoc operator+(const StringLoc& a, const StringLoc& b);
		friend std::ostream& operator<<(std::ostream& out, const StringLoc& str);

		gc::Ref<gc::PrimitiveArray<char>> m_value;
		mutable std::atomic<std::size_t> m_hash;

	public:
		String();
		void gc_enumerate_refs() override;

		void initialize(const char* str);
		void initialize(const char* str, std::size_t len);
		void initialize(const std::string& str);
		void initialize(const std::string& str, std::size_t start_pos, std::size_t end_pos);
		void initialize(const StringLoc& str, std::size_t start_pos, std::size_t end_pos);
		void initialize(const StringLoc& a, const StringLoc& b);

	public:
		bool is_empty() const;
		std::size_t length() const;
		char char_at(std::size_t index) const;
		StringLoc substring(std::size_t start) const;
		StringLoc substring(std::size_t start, std::size_t end) const;

		std::unique_ptr<char[]> get_c_string() const;
		std::string get_std_string() const;
		void get_std_string(std::size_t start_ofs, std::size_t end_ofs, std::string& str) const;
		gc::Local<ByteArray> get_bytes() const;
		const char* get_raw_data() const;

		int compare_to(const StringLoc& str) const;

		bool equals(const gc::Local<const ObjectEx>& obj) const override;
		std::size_t hash_code() const override;

		static int compare(const gc::Local<const String>& str1, const gc::Local<const String>& str2);
	};

	//
	//StringKey
	//

	class StdStringKey;
	class GCStringKey;
	class GCPtrStringKey;

	class StringKey {
	protected:
		StringKey(){}

	public:
		virtual int compare_to(const StringKey& key) const = 0;
		virtual int compare_to_std(const StdStringKey& key) const = 0;
		virtual int compare_to_gc(const GCStringKey& key) const = 0;
		virtual int compare_to_gc_ptr(const GCPtrStringKey& key) const = 0;
		virtual const StringLoc& get_gc_string() const;
	};

	//
	//StdStringKey
	//

	class StdStringKey : public StringKey {
		const std::string* m_str;
		std::size_t m_start_ofs;
		std::size_t m_end_ofs;

	public:
		explicit StdStringKey(const std::string& str);
		StdStringKey(const std::string& str, std::size_t start_ofs, std::size_t end_ofs);

		int compare_to(const StringKey& key) const override;
		int compare_to_std(const StdStringKey& key) const override;
		int compare_to_gc(const GCStringKey& key) const override;
		int compare_to_gc_ptr(const GCPtrStringKey& key) const override;
		const StringLoc& get_gc_string() const override;

		std::size_t length() const {
			return m_end_ofs - m_start_ofs;
		}

		char char_at(std::size_t index) const {
			return (*m_str)[m_start_ofs + index];
		}
	};

	//
	//GCStringKey
	//

	class GCStringKey : public StringKey {
		StringLoc m_str;
		std::size_t m_start_ofs;
		std::size_t m_end_ofs;

	public:
		explicit GCStringKey(const StringLoc& str);
		GCStringKey(const StringLoc& str, std::size_t start_ofs, std::size_t end_ofs);

		int compare_to(const StringKey& key) const override;
		int compare_to_std(const StdStringKey& key) const override;
		int compare_to_gc(const GCStringKey& key) const override;
		int compare_to_gc_ptr(const GCPtrStringKey& key) const override;
		const StringLoc& get_gc_string() const override;

		std::size_t length() const {
			return m_end_ofs - m_start_ofs;
		}

		char char_at(std::size_t index) const {
			return m_str->char_at(m_start_ofs + index);
		}
	};

	//
	//GCPtrStringKey
	//

	class GCPtrStringKey : public StringKey {
		const String* m_str;
		std::size_t m_start_ofs;
		std::size_t m_end_ofs;

	public:
		explicit GCPtrStringKey(const String* str);
		GCPtrStringKey(const String* str, std::size_t start_ofs, std::size_t end_ofs);

		int compare_to(const StringKey& key) const override;
		int compare_to_std(const StdStringKey& key) const override;
		int compare_to_gc(const GCStringKey& key) const override;
		int compare_to_gc_ptr(const GCPtrStringKey& key) const override;

		std::size_t length() const {
			return m_end_ofs - m_start_ofs;
		}

		char char_at(std::size_t index) const {
			return m_str->char_at(m_start_ofs + index);
		}
	};

	//
	//StringKeyValue
	//

	class StringKeyValue {
		const StringKey* m_key;

	public:
		explicit StringKeyValue(const StringKey& key);
		const StringKey& get_key() const;
	};

	bool operator==(const StringKey& a, const StringKey& b);
	bool operator<(const StringKey& a, const StringKey& b);
	bool operator==(const StringKeyValue& a, const StringKeyValue& b);
	bool operator<(const StringKeyValue& a, const StringKeyValue& b);

	class StringIterator;

	bool operator==(const StringIterator& a, const StringIterator& b);
	bool operator!=(const StringIterator& a, const StringIterator& b);

	//
	//StringIterator
	//

	class StringIterator {
		friend bool operator==(const StringIterator& a, const StringIterator& b);
		friend bool operator!=(const StringIterator& a, const StringIterator& b);

		static const std::string EMPTY;

		const String* m_str;
		std::size_t m_pos;

	public:
		StringIterator() : m_str(nullptr), m_pos(0){}
		StringIterator(const String* str, std::size_t pos) : m_str(str), m_pos(pos){}

		static StringIterator begin(const String* str) {
			assert(str);
			return StringIterator(str, 0);
		}

		static StringIterator end(const String* str) {
			assert(str);
			return StringIterator(str, str->length());
		}

		std::size_t pos() const { return m_pos; }

		char operator*() const {
			assert(m_str);
			return m_str->char_at(m_pos);
		}

		StringIterator operator+(std::size_t dist) const {
			return StringIterator(m_str, m_pos + dist);
		}

		StringIterator& operator++() {
			++m_pos;
			return *this;
		}

		char operator[](std::size_t index) const {
			assert(m_str);
			assert(index <= m_str->length() - m_pos);
			return m_str->char_at(m_pos + index);
		}

		std::size_t operator-(const StringIterator& iter) const {
			assert(m_str == iter.m_str);
			assert(m_pos >= iter.m_pos);
			return m_pos - iter.m_pos;
		}

		StringLoc get_string(const StringIterator& end) const {
			assert(m_str);
			assert(m_str == end.m_str);
			assert(m_pos <= end.m_pos);
			return m_str->substring(m_pos, end.m_pos);
		}

		void get_std_string(const StringIterator& end, std::string& str) const {
			assert(m_str);
			assert(m_str == end.m_str);
			assert(m_pos <= end.m_pos);
			return m_str->get_std_string(m_pos, end.m_pos, str);
		}

		GCPtrStringKey get_string_key(const StringIterator& end) const {
			assert(m_str);
			assert(m_str == end.m_str);
			assert(m_pos <= end.m_pos);
			return GCPtrStringKey(m_str, m_pos, end.m_pos);
		}
	};

	inline bool operator==(const StringIterator& a, const StringIterator& b) {
		assert(a.m_str == b.m_str);
		return a.m_pos == b.m_pos;
	}

	inline bool operator!=(const StringIterator& a, const StringIterator& b) {
		assert(a.m_str == b.m_str);
		return a.m_pos != b.m_pos;
	}

}

#endif//SYNSAMPLE_CORE_STRINGEX_H_INCLUDED
