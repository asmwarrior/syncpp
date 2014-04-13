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

//Windows platform-dependent file functions.

#include <cstdio>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include <Windows.h>

#include "platform.h"
#include "platform_file.h"
#include "platform_file_common.h"
#include "stringex.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace pf = ss::platform;

namespace {
	bool is_drive_letter(char c) {
		//TODO Use a more reliable approach.
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	}

	void path_slash_external(std::string& str) {
		for (std::size_t i = 0, len = str.length(); i < len; ++i) {
			char c = str[i];
			if (c == '/') str[i] = '\\';
		}
	}

	void path_slash_external(char* path) {
		for (;;) {
			char c = *path;
			if (c == 0) break;
			if (c == '/') *path = '\\';
			++path;
		}
	}

	void path_slash_internal(char* path) {
		for (;;) {
			char c = *path;
			if (c == 0) break;
			if (c == '\\') *path = '/';
			++path;
		}
	}
}

std::size_t pf::pf_get_path_drive_letter_end(const ss::StringLoc& path) {
	std::size_t pos = 0;
	std::size_t len = path->length();
	if (len >= 2 && ':' == path->char_at(1) && is_drive_letter(path->char_at(0))) pos += 2;
	return pos;
}

std::unique_ptr<char[]> pf::pf_get_current_directory() {
	std::unique_ptr<char[]> buffer(new char[MAX_PATH]);
	DWORD count = GetCurrentDirectoryA(MAX_PATH, buffer.get());
	if (!count) throw RuntimeError("Unable to get current directory");
	if (count > MAX_PATH) {
		buffer.reset(new char[count]);
		DWORD new_count = GetCurrentDirectoryA(count, buffer.get());
		if (!new_count || new_count > count) throw RuntimeError("Unable to get current directory");
	}
	path_slash_internal(buffer.get());
	return std::move(buffer);
}

ss::StringLoc pf::get_file_native_path(const ss::StringLoc& path) {
	return replace_characters<'/','\\'>(path);
}

pf::FileInfo pf::get_file_info(const ss::StringLoc& path) {
	FileInfo result;
	result.m_type = FileType::NONEXISTENT;
	result.m_size = 0;
	result.m_size_valid = false;

	WIN32_FILE_ATTRIBUTE_DATA data;

	std::unique_ptr<char[]> cpath = path->get_c_string();
	path_slash_external(cpath.get());
	if (!GetFileAttributesExA(cpath.get(), GetFileExInfoStandard, &data)) return result;

	if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		result.m_type = FileType::DIRECTORY;
	} else if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)) {
		result.m_type = FileType::FILE;

		static_assert(4 == sizeof(data.nFileSizeHigh), "Wrong type size");
		static_assert(4 == sizeof(data.nFileSizeLow), "Wrong type size");
		static_assert(8 == sizeof(result.m_size), "Wrong type size");

		//TODO Make this computation platform-independent.
		result.m_size = ((unsigned long long)data.nFileSizeHigh << 32) | data.nFileSizeLow;
		result.m_size_valid = true;
	} else {
		result.m_type = FileType::OTHER;
	}

	return result;
}

void pf::pf_list_files(const std::string& path_str, std::vector<ss::StringLoc>& list) {
	std::string search_str = path_str + '*';
	path_slash_external(search_str);

	WIN32_FIND_DATAA find_data;
	memset(&find_data, 0, sizeof(find_data));

	HANDLE handle = FindFirstFileA(search_str.c_str(), &find_data);
	if (INVALID_HANDLE_VALUE == handle) return;

	try {
		for (;;) {
			const char* file_name = find_data.cFileName;
			if (strcmp(".", file_name) && strcmp("..", file_name)) {
				std::string file_path = path_str + file_name;
				list.push_back(gc::create<String>(file_path));
			}
			if (!FindNextFileA(handle, &find_data)) break;
		}
	} catch (...) {
		FindClose(handle);
		throw;
	}

	FindClose(handle);
}

void pf::delete_file(const ss::StringLoc& path) {
	std::unique_ptr<char[]> cpath = path->get_c_string();
	path_slash_external(cpath.get());

	DWORD attrs = GetFileAttributesA(cpath.get());
	if (INVALID_FILE_ATTRIBUTES == attrs) throw RuntimeError("Deletion failed : file not found");

	BOOL b = attrs & FILE_ATTRIBUTE_DIRECTORY ? RemoveDirectoryA(cpath.get()) : DeleteFileA(cpath.get());
	if (!b) throw RuntimeError("Deletion failed");
}

void pf::create_directory(const ss::StringLoc& path) {
	std::unique_ptr<char[]> cpath = path->get_c_string();
	path_slash_external(cpath.get());
	if (!CreateDirectoryA(cpath.get(), NULL)) throw RuntimeError("Unable to create directory");
}
