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

//Platform-independent file functions.

#include <fstream>
#include <string>

#include "platform.h"
#include "platform_file.h"
#include "platform_file_common.h"
#include "stringex.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace pf = ss::platform;

std::size_t pf::get_path_root_end(const ss::StringLoc& path) {
	std::size_t pos = pf::pf_get_path_drive_letter_end(path);
	const std::size_t len = path->length();
	if (pos < len && '/' == path->char_at(pos)) ++pos;
	return pos;
}

std::string pf::get_path_with_slash(const ss::StringLoc& path) {
	std::string path_str = path->get_std_string();
	std::size_t path_len = path_str.length();
	if (0 != path_len && '/' != path_str[path_len - 1]) path_str += '/';
	return path_str;
}

namespace {
	std::size_t get_path_end(const ss::StringLoc& path, std::size_t root_end) {
		std::size_t len = path->length();
		return (root_end < len && '/' == path->char_at(len - 1)) ? len - 1 : len;
	}

	std::size_t get_file_name_start(const ss::StringLoc& path, std::size_t root_end, std::size_t path_end) {
		std::size_t name_start = path_end;
		while (name_start > root_end && '/' != path->char_at(name_start - 1)) --name_start;
		return name_start;
	}
}

ss::StringLoc pf::get_file_name(const ss::StringLoc& path) {
	std::size_t root_end = get_path_root_end(path);
	std::size_t path_end = get_path_end(path, root_end);
	std::size_t name_start = get_file_name_start(path, root_end, path_end);
	return path->substring(name_start, path_end);
}

ss::StringLoc pf::get_file_parent_path(const ss::StringLoc& path) {
	std::size_t root_end = get_path_root_end(path);
	std::size_t path_end = get_path_end(path, root_end);
	if (root_end == path_end) return nullptr;

	std::size_t name_start = get_file_name_start(path, root_end, path_end);
	if (name_start > root_end && '/' == path->char_at(name_start - 1)) --name_start;
	if (0 == name_start) return nullptr;

	return path->substring(0, name_start);
}

ss::StringLoc pf::get_file_child_path(
	const ss::StringLoc& parent_path,
	const ss::StringLoc& name)
{
	std::string path_str = get_path_with_slash(parent_path);
	return gc::create<String>(path_str + name->get_std_string());
}

ss::StringLoc pf::get_file_absolute_path(const ss::StringLoc& path) {
	std::size_t root_end = get_path_root_end(path);

	//For simplicity, syntax like "C:path\file.ext" is treated as an absolute path, and returned as is.
	if (root_end > 0) return path;

	std::unique_ptr<char[]> buffer = pf_get_current_directory();
	std::string abs_path = buffer.get();
	std::size_t len = abs_path.length();
	if (len > 0 && '/' != abs_path[len - 1]) abs_path += '/';
	abs_path += path->get_std_string();

	return gc::create<String>(abs_path);
}

gc::Local<ss::StringArray> pf::list_files(const ss::StringLoc& path) {
	std::string path_str = get_path_with_slash(path);
	std::vector<StringLoc> result_vector;
	pf_list_files(path_str, result_vector);

	std::size_t count = result_vector.size();
	gc::Local<StringArray> result_array = StringArray::create(count);
	for (std::size_t i = 0; i < count; ++i) result_array->get(i) = result_vector[i];
	
	return result_array;
}

namespace {
	void open_file(std::ifstream& in, const ss::StringLoc& path, std::ios_base::openmode mode) {
		std::string std_path = path->get_std_string();

		in.exceptions(std::ios_base::badbit | std::ios_base::failbit);
		try {
			in.open(std_path, std::ios_base::in | mode);
		} catch (std::ios_base::failure&) {
			throw ss::RuntimeError(std::string("File not found: ") + std_path);
		}
		in.exceptions(std::ios_base::badbit);
	}
}

gc::Local<ss::ByteArray> pf::read_file_bytes(const ss::StringLoc& path) {
	try {
		std::ifstream in;
		open_file(in, path, std::ios_base::binary | std::ios_base::ate);
		std::streamoff pos = in.tellg();

		if (pos > std::numeric_limits<std::size_t>::max()) {
			throw RuntimeError("File is too large");
		}

		std::size_t size = static_cast<std::size_t>(pos);

		gc::Local<ByteArray> array = ByteArray::create(size);
		in.seekg(0, std::ios::beg);
		in.read(array->raw_array(), size);

		return array;
	} catch (std::ios_base::failure& e) {
		throw ss::RuntimeError(std::string("File read error: ") + e.what());
	}
}

ss::StringLoc pf::read_file_text(const ss::StringLoc& path) {
	try {
		std::ifstream in;
		open_file(in, path, std::ios_base::openmode());

		typedef std::istreambuf_iterator<char> iter_t;
		std::string text((iter_t(in)), iter_t());
		return gc::create<String>(text);
	} catch (std::ios_base::failure& e) {
		throw ss::RuntimeError(std::string("File read error: ") + e.what());
	}
}

void pf::write_file_text(const ss::StringLoc& path, const ss::StringLoc& text, bool append) {
	std::string std_path = path->get_std_string();

	try {
		std::ofstream out;
		out.exceptions(std::ios_base::badbit | std::ios_base::failbit);

		std::ios_base::openmode mode = append ? (std::ios_base::app | std::ios_base::ate) : std::ios_base::trunc;
		out.open(std_path, std::ios_base::out | mode);
		out << text;
	} catch (std::ios_base::failure& e) {
		throw RuntimeError(std::string("File read error: ") + e.what());
	}
}

void pf::rename_file(const ss::StringLoc& src_path, const ss::StringLoc& dst_path) {
	std::unique_ptr<char[]> src_cpath = src_path->get_c_string();
	std::unique_ptr<char[]> dst_cpath = dst_path->get_c_string();
	if (rename(src_cpath.get(), dst_cpath.get())) throw RuntimeError("Renaming failed");
}
