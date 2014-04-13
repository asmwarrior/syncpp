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

//Classes for Script Language execution stack trace tracking.

#ifndef SYNSAMPLE_CORE_STACKTRACE_H_INCLUDED
#define SYNSAMPLE_CORE_STACKTRACE_H_INCLUDED

#include <ostream>

#include "ast_type.h"
#include "common.h"
#include "gc.h"
#include "stringex.h"

namespace syn_script {
	namespace rt {

		//
		//StackTraceElement
		//

		class StackTraceElement : public gc::Object {
			NONCOPYABLE(StackTraceElement);

			gc::Ref<TextPos> m_text_pos;

		public:
			StackTraceElement(){}
			void gc_enumerate_refs() override;
			void initialize(const gc::Local<TextPos>& text_pos);

			gc::Local<TextPos> get_text_pos() const;
		};

		std::ostream& operator<<(std::ostream& out, const gc::Local<StackTraceElement>& element);

		//
		//StackTraceMark
		//

		class StackTraceMark {
			NONCOPYABLE(StackTraceMark);

			typedef gc::Array<StackTraceElement> ElementArray;

			StackTraceMark* const m_next;
			const gc::Local<TextPos> m_text_pos;

		public:
			explicit StackTraceMark(const gc::Local<TextPos>& text_pos);
			~StackTraceMark();

			static gc::Local<ElementArray> get_stack_trace(const gc::Local<TextPos>& cur_text_pos);
		};

	}
}

#endif//SYNSAMPLE_CORE_STACKTRACE_H_INCLUDED
