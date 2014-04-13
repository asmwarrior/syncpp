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

//AST: types of tokens.

#include "ast.h"
#include "ast_type.h"
#include "gc.h"
#include "name.h"
#include "stringex.h"

namespace ss = syn_script;
namespace ast = ss::ast;
namespace gc = ss::gc;

//
//AstValue
//

void ast::AstValue::gc_enumerate_refs() {
	gc::Object::gc_enumerate_refs();
	gc_ref(m_pos);
}

void ast::AstValue::initialize(const gc::Local<ss::TextPos>& pos) {
	m_pos = pos;
}

const gc::Ref<ss::TextPos>& ast::AstValue::pos() const {
	return m_pos;
}

//
//AstInteger
//

void ast::AstInteger::initialize(const gc::Local<ss::TextPos>& pos, ss::ScriptIntegerType value) {
	AstValue::initialize(pos);
	m_value = value;
}

ss::ScriptIntegerType ast::AstInteger::value() const {
	return m_value;
}

//
//AstFloat
//

void ast::AstFloat::initialize(const gc::Local<ss::TextPos>& pos, ss::ScriptFloatType value) {
	AstValue::initialize(pos);
	m_value = value;
}

ss::ScriptFloatType ast::AstFloat::value() const {
	return m_value;
}

//
//AstName
//

void ast::AstName::gc_enumerate_refs() {
	AstValue::gc_enumerate_refs();
	gc_ref(m_info);
}

void ast::AstName::initialize(const gc::Local<ss::TextPos>& pos, const gc::Local<const ss::NameInfo>& info) {
	AstValue::initialize(pos);
	m_info = info;
}

const gc::Ref<const ss::NameInfo>& ast::AstName::get_info() const {
	return m_info;
}

const ss::NameID& ast::AstName::get_id() const {
	return m_info->get_id();
}

ss::StringLoc ast::AstName::get_str() const {
	return m_info->get_str();
}

//
//AstString
//

void ast::AstString::gc_enumerate_refs() {
	AstValue::gc_enumerate_refs();
	gc_ref(m_value);
}

void ast::AstString::initialize(const gc::Local<ss::TextPos>& pos, const ss::StringLoc& value) {
	AstValue::initialize(pos);
	m_value = value;
}

ss::StringLoc ast::AstString::value() const {
	return m_value;
}
