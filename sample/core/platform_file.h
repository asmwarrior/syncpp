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

//Platform-dependent file handling functions.

#ifndef SYNSAMPLE_CORE_PLATFORM_FILE_H_INCLUDED
#define SYNSAMPLE_CORE_PLATFORM_FILE_H_INCLUDED

#include "gc.h"
#include "stringex.h"

namespace syn_script {
	namespace platform {

		enum class FileType {
			NONEXISTENT,
			FILE,
			DIRECTORY,
			OTHER
		};

		struct FileInfo {
			FileType m_type;
			unsigned long long m_size;
			bool m_size_valid;
		};

		StringLoc get_file_name(const StringLoc& path);
		StringLoc get_file_parent_path(const StringLoc& path);
		StringLoc get_file_absolute_path(const StringLoc& path);
		StringLoc get_file_native_path(const StringLoc& path);
		StringLoc get_file_child_path(const StringLoc& parent_path, const StringLoc& name);
		FileInfo get_file_info(const StringLoc& path);
		gc::Local<StringArray> list_files(const StringLoc& path);
		gc::Local<ByteArray> read_file_bytes(const StringLoc& path);
		StringLoc read_file_text(const StringLoc& path);
		void write_file_text(const StringLoc& path, const StringLoc& text, bool append);
		void rename_file(const StringLoc& src_path, const StringLoc& dst_path);
		void delete_file(const StringLoc& path);
		void create_directory(const StringLoc& path);

		template<char oldc, char newc>
		StringLoc replace_characters(const StringLoc& path) {
			const std::size_t len = path->length();
			for (std::size_t i = 0; i < len; ++i) {
				char c = path->char_at(i);
				if (c == oldc) {
					std::unique_ptr<char[]> cpath = path->get_c_string();
					cpath[i] = newc;
					for (std::size_t j = i + 1; j < len; ++j) {
						char& cref = cpath[j];
						if (cref == oldc) cref = newc;
					}
					StringLoc new_path = gc::create<String>(cpath.get());
					return new_path;
				}
			}
			return path;
		}

	}
}

#endif//SYNSAMPLE_CORE_PLATFORM_FILE_H_INCLUDED
