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

//Stack trace classes implementation.

#include <cassert>

#include "common.h"
#include "platform.h"
#include "stacktrace.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace rt = ss::rt;

namespace {
	PLATFORM__THREAD_LOCAL rt::StackTraceMark* gt_stack_top = nullptr;
}

//
//StackTraceElement
//

void rt::StackTraceElement::gc_enumerate_refs() {
	gc_ref(m_text_pos);
}

void rt::StackTraceElement::initialize(const gc::Local<ss::TextPos>& text_pos) {
	m_text_pos = text_pos;
}

gc::Local<ss::TextPos> rt::StackTraceElement::get_text_pos() const {
	return m_text_pos;
}

std::ostream& rt::operator<<(std::ostream& out, const gc::Local<rt::StackTraceElement>& element) {
	ss::operator<<(out, element->get_text_pos());
	return out;
}

//
//StackTraceMark
//

rt::StackTraceMark::StackTraceMark(const gc::Local<ss::TextPos>& text_pos)
: m_next(gt_stack_top),
m_text_pos(text_pos)
{
	gt_stack_top = this;
}

rt::StackTraceMark::~StackTraceMark() {
	assert(this == gt_stack_top);
	gt_stack_top = m_next;
}

gc::Local<rt::StackTraceMark::ElementArray> rt::StackTraceMark::get_stack_trace(
	const gc::Local<ss::TextPos>& cur_text_pos)
{
	std::size_t cnt = 0;
	StackTraceMark* mark = gt_stack_top;
	while (mark) {
		++cnt;
		mark = mark->m_next;
	}

	gc::Local<ElementArray> array = ElementArray::create(cnt + 1);
	std::size_t ofs = 0;
	array->get(ofs++) = gc::create<StackTraceElement>(cur_text_pos);

	mark = gt_stack_top;
	while (mark) {
		array->get(ofs++) = gc::create<StackTraceElement>(mark->m_text_pos);
		mark = mark->m_next;
	}

	return array;
}
