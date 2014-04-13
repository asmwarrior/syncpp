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

//File API.

#include <algorithm>
#include <memory>

#include "api.h"
#include "api_basic.h"
#include "api_io.h"
#include "common.h"
#include "gc.h"
#include "platform_file.h"
#include "scope.h"
#include "stringex.h"
#include "sysclass.h"
#include "sysclassbld.h"
#include "sysclassbld__imp.h"
#include "value.h"
#include "value_core.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace rt = ss::rt;
namespace pf = ss::platform;

namespace syn_script {
	namespace rt {
		class FileValue;
	}
}

//
//FileValue (definition)
//

class rt::FileValue : public SysObjectValue {
	NONCOPYABLE(FileValue);

	StringRef m_path;

public:
	FileValue(){}
	void gc_enumerate_refs() override;
	void initialize(const StringLoc& path);

	StringLoc to_string(const gc::Local<ExecContext>& context) const override;

protected:
	std::size_t get_sys_class_id() const override;

private:
	static gc::Local<FileValue> create(const StringLoc& path);
	static gc::Local<FileValue> create(const gc::Local<FileValue>& parent, const StringLoc& path);

	static gc::Local<FileValue> api_create_1(
		const gc::Local<ExecContext>& context,
		const StringLoc& path);

	static gc::Local<FileValue> api_create_2(
		const gc::Local<ExecContext>& context,
		const gc::Local<FileValue>& parent,
		const StringLoc& path);

	StringLoc api_get_name(const gc::Local<ExecContext>& context);
	StringLoc api_get_path(const gc::Local<ExecContext>& context);
	StringLoc api_get_absolute_path(const gc::Local<ExecContext>& context);
	StringLoc api_get_native_path(const gc::Local<ExecContext>& context);
	gc::Local<FileValue> api_get_absolute_file(const gc::Local<ExecContext>& context);
	StringLoc api_get_parent_path(const gc::Local<ExecContext>& context);
	gc::Local<FileValue> api_get_parent_file(const gc::Local<ExecContext>& context);
	bool api_exists(const gc::Local<ExecContext>& context);
	bool api_is_file(const gc::Local<ExecContext>& context);
	bool api_is_directory(const gc::Local<ExecContext>& context);
	ScriptIntegerType api_get_size(const gc::Local<ExecContext>& context);
	gc::Local<gc::Array<Value>> api_list_files(const gc::Local<ExecContext>& context);
	gc::Local<ByteArrayValue> api_read_bytes(const gc::Local<ExecContext>& context);
	StringLoc api_read_text(const gc::Local<ExecContext>& context);
	void api_write_text_1(const gc::Local<ExecContext>& context, const StringLoc& text);
	void api_write_text_2(const gc::Local<ExecContext>& context, const StringLoc& text, bool append);
	void api_rename_to(const gc::Local<ExecContext>& context, const gc::Local<FileValue>& file);
	void api_delete(const gc::Local<ExecContext>& context);
	void api_mkdir(const gc::Local<ExecContext>& context);
	gc::Local<TextOutputValue> api_text_out_0(const gc::Local<ExecContext>& context);
	gc::Local<TextOutputValue> api_text_out_1(const gc::Local<ExecContext>& context, bool append);
	gc::Local<BinaryInputValue> api_binary_in(const gc::Local<ExecContext>& context);
	gc::Local<BinaryOutputValue> api_binary_out_0(const gc::Local<ExecContext>& context);
	gc::Local<BinaryOutputValue> api_binary_out_1(const gc::Local<ExecContext>& context, bool append);

public:
	class API;
};

//
//FileValue::API
//

class rt::FileValue::API : public SysAPI<FileValue> {
	void init() override {
		bld->add_constructor(api_create_1);
		bld->add_constructor(api_create_2);
		bld->add_method("get_name", &FileValue::api_get_name);
		bld->add_method("get_path", &FileValue::api_get_path);
		bld->add_method("get_absolute_path", &FileValue::api_get_absolute_path);
		bld->add_method("get_native_path", &FileValue::api_get_native_path);
		bld->add_method("get_absolute_file", &FileValue::api_get_absolute_file);
		bld->add_method("get_parent_path", &FileValue::api_get_parent_path);
		bld->add_method("get_parent_file", &FileValue::api_get_parent_file);
		bld->add_method("exists", &FileValue::api_exists);
		bld->add_method("is_file", &FileValue::api_is_file);
		bld->add_method("is_directory", &FileValue::api_is_directory);
		bld->add_method("get_size", &FileValue::api_get_size);
		bld->add_method("list_files", &FileValue::api_list_files);
		bld->add_method("read_bytes", &FileValue::api_read_bytes);
		bld->add_method("read_text", &FileValue::api_read_text);
		bld->add_method("write_text", &FileValue::api_write_text_1);
		bld->add_method("write_text", &FileValue::api_write_text_2);
		bld->add_method("rename_to", &FileValue::api_rename_to);
		bld->add_method("delete", &FileValue::api_delete);
		bld->add_method("mkdir", &FileValue::api_mkdir);
		bld->add_method("text_out", &FileValue::api_text_out_0);
		bld->add_method("text_out", &FileValue::api_text_out_1);
		bld->add_method("binary_in", &FileValue::api_binary_in);
		bld->add_method("binary_out", &FileValue::api_binary_out_0);
		bld->add_method("binary_out", &FileValue::api_binary_out_1);
	}
};

//
//FileValue (implementation)
//

void rt::FileValue::gc_enumerate_refs() {
	gc_ref(m_path);
}

void rt::FileValue::initialize(const ss::StringLoc& path) {
	m_path = pf::replace_characters<'\\','/'>(path);
}

ss::StringLoc rt::FileValue::to_string(const gc::Local<ExecContext>& context) const {
	return m_path;
}

std::size_t rt::FileValue::get_sys_class_id() const {
	return API::get_class_id();
}

gc::Local<rt::FileValue> rt::FileValue::create(const ss::StringLoc& path) {
	if (!path) return gc::Local<FileValue>();
	return gc::create<FileValue>(path);
}

gc::Local<rt::FileValue> rt::FileValue::create(
	const gc::Local<FileValue>& parent,
	const ss::StringLoc& path)
{
	const StringLoc child_path =
		pf::get_file_child_path(parent->m_path, pf::replace_characters<'\\','/'>(path));
	return create(child_path);
}

gc::Local<rt::FileValue> rt::FileValue::api_create_1(
	const gc::Local<ExecContext>& context,
	const ss::StringLoc& path)
{
	if (!path) throw RuntimeError("Path is null");
	return create(path);
}

gc::Local<rt::FileValue> rt::FileValue::api_create_2(
	const gc::Local<ExecContext>& context,
	const gc::Local<FileValue>& parent,
	const ss::StringLoc& path)
{
	if (!parent) throw RuntimeError("Parent is null");
	if (!path) throw RuntimeError("Path is null");
	return create(parent, path);
}

ss::StringLoc rt::FileValue::api_get_name(const gc::Local<ExecContext>& context) {
	return pf::get_file_name(m_path);
}

ss::StringLoc rt::FileValue::api_get_path(const gc::Local<ExecContext>& context) {
	return m_path;
}

ss::StringLoc rt::FileValue::api_get_absolute_path(const gc::Local<ExecContext>& context) {
	return pf::get_file_absolute_path(m_path);
}

ss::StringLoc rt::FileValue::api_get_native_path(const gc::Local<ExecContext>& context) {
	return pf::get_file_native_path(m_path);
}

gc::Local<rt::FileValue> rt::FileValue::api_get_absolute_file(const gc::Local<ExecContext>& context) {
	return create(api_get_absolute_path(context));
}

ss::StringLoc rt::FileValue::api_get_parent_path(const gc::Local<ExecContext>& context) {
	return pf::get_file_parent_path(m_path);
}

gc::Local<rt::FileValue> rt::FileValue::api_get_parent_file(const gc::Local<ExecContext>& context) {
	return create(api_get_parent_path(context));
}

bool rt::FileValue::api_exists(const gc::Local<ExecContext>& context) {
	pf::FileInfo info = pf::get_file_info(m_path);
	return pf::FileType::NONEXISTENT != info.m_type;
}

bool rt::FileValue::api_is_file(const gc::Local<ExecContext>& context) {
	pf::FileInfo info = pf::get_file_info(m_path);
	return pf::FileType::FILE == info.m_type;
}

bool rt::FileValue::api_is_directory(const gc::Local<ExecContext>& context) {
	pf::FileInfo info = pf::get_file_info(m_path);
	return pf::FileType::DIRECTORY == info.m_type;
}

ss::ScriptIntegerType rt::FileValue::api_get_size(const gc::Local<ExecContext>& context) {
	pf::FileInfo info = pf::get_file_info(m_path);
	if (!info.m_size_valid) return 0;
	return ulonglong_to_scriptint_opt(info.m_size);
}

gc::Local<gc::Array<rt::Value>> rt::FileValue::api_list_files(const gc::Local<ExecContext>& context) {
	gc::Local<StringArray> files = pf::list_files(m_path);
	std::size_t cnt = files->length();
	
	gc::Local<gc::Array<Value>> values = gc::Array<Value>::create(cnt);
	for (std::size_t i = 0; i < cnt; ++i) {
		StringLoc path = files->get(i);
		values->get(i) = gc::create<FileValue>(path);
	}

	return values;
}

gc::Local<rt::ByteArrayValue> rt::FileValue::api_read_bytes(const gc::Local<ExecContext>& context) {
	gc::Local<ByteArray> array = pf::read_file_bytes(m_path);
	return gc::create<ByteArrayValue>(array);
}

ss::StringLoc rt::FileValue::api_read_text(const gc::Local<ExecContext>& context) {
	return pf::read_file_text(m_path);
}

void rt::FileValue::api_write_text_1(const gc::Local<ExecContext>& context, const ss::StringLoc& text) {
	pf::write_file_text(m_path, text, false);
}

void rt::FileValue::api_write_text_2(
	const gc::Local<ExecContext>& context,
	const ss::StringLoc& text,
	bool append)
{
	pf::write_file_text(m_path, text, append);
}

void rt::FileValue::api_rename_to(const gc::Local<ExecContext>& context, const gc::Local<FileValue>& file) {
	pf::rename_file(m_path, file->m_path);
}

void rt::FileValue::api_delete(const gc::Local<ExecContext>& context) {
	pf::delete_file(m_path);
}

void rt::FileValue::api_mkdir(const gc::Local<ExecContext>& context) {
	pf::create_directory(m_path);
}

gc::Local<rt::TextOutputValue> rt::FileValue::api_text_out_0(const gc::Local<ExecContext>& context) {
	return api_text_out_1(context, false);
}

gc::Local<rt::TextOutputValue> rt::FileValue::api_text_out_1(
	const gc::Local<ExecContext>& context,
	bool append)
{
	const StringLoc path = m_path;
	return gc::create<FileTextOutputValue>(path, append);
}

gc::Local<rt::BinaryInputValue> rt::FileValue::api_binary_in(const gc::Local<ExecContext>& context) {
	const StringLoc path = m_path;
	return gc::create<FileBinaryInputValue>(path);
}

gc::Local<rt::BinaryOutputValue> rt::FileValue::api_binary_out_0(const gc::Local<ExecContext>& context) {
	return api_binary_out_1(context, false);
}

gc::Local<rt::BinaryOutputValue> rt::FileValue::api_binary_out_1(
	const gc::Local<ExecContext>& context,
	bool append)
{
	const StringLoc path = m_path;
	return gc::create<FileBinaryOutputValue>(path, append);
}

//
//(Functions)
//

namespace {
	rt::SysNamespaceInitializer s_sys_namespace_initializer([](rt::SysClassBuilder<rt::SysNamespaceValue>& bld){
		bld.add_class<rt::FileValue>("File");
	});
}

void link__api_file(){}
