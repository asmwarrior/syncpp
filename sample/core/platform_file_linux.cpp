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

//Linux platform-dependent file functions.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "platform.h"
#include "platform_file.h"
#include "platform_file_common.h"
#include "stringex.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace pf = ss::platform;

std::size_t pf::pf_get_path_drive_letter_end(const ss::StringLoc& path) {
	return 0;
}

std::unique_ptr<char[]> pf::pf_get_current_directory() {
	std::size_t size = std::max(1024, PATH_MAX);
	std::unique_ptr<char[]> buf(new char[size]);
	if (!getcwd(buf.get(), size)) throw ss::RuntimeError("getcwd() failed!");
	return std::move(buf);
}

ss::StringLoc pf::get_file_native_path(const ss::StringLoc& path) {
	return path;
}

pf::FileInfo pf::get_file_info(const ss::StringLoc& path) {
	FileInfo result;
	result.m_type = FileType::NONEXISTENT;
	result.m_size = 0;
	result.m_size_valid = false;

	struct stat buf;

	std::unique_ptr<char[]> cpath = path->get_c_string();
	if (stat(cpath.get(), &buf)) return result;

	if (S_ISDIR(buf.st_mode)) {
		result.m_type = FileType::DIRECTORY;
	} else if (S_ISREG(buf.st_mode)) {
		result.m_type = FileType::FILE;
		result.m_size = buf.st_size;
		result.m_size_valid = true;
	} else {
		result.m_type = FileType::OTHER;
	}

	return result;
}

void pf::pf_list_files(const std::string& path_str, std::vector<ss::StringLoc>& list) {
	struct dirent entry;
	struct dirent* pentry;

	DIR* dir = opendir(path_str.c_str());
	if (!dir) return;

	try {
		for (;;) {
			if (readdir_r(dir, &entry, &pentry) || !pentry) break;
			const char* file_name = entry.d_name;
			if (std::strcmp(".", file_name) && std::strcmp("..", file_name)) {
				std::string file_path = path_str + file_name;
				list.push_back(gc::create<String>(file_path));
			}
		}
	} catch (...) {
		closedir(dir);
		throw;
	}

	closedir(dir);
}

void pf::delete_file(const ss::StringLoc& path) {
	struct stat buf;
	std::unique_ptr<char[]> cpath = path->get_c_string();
	if (stat(cpath.get(), &buf)) throw RuntimeError("File not found");

	int res = 1;
	if (S_ISDIR(buf.st_mode)) {
		res = rmdir(cpath.get());
	} else if (S_ISREG(buf.st_mode)) {
		res = std::remove(cpath.get());
	} else {
		throw ss::RuntimeError("Invalid file type");
	}

	if (res) throw ss::RuntimeError("Deletion failed");
}

void pf::create_directory(const ss::StringLoc& path) {
	std::unique_ptr<char[]> cpath = path->get_c_string();
	if (mkdir(cpath.get(), S_IRWXU | S_IRWXG | S_IRWXO)) throw RuntimeError("Unable to create directory");
}
