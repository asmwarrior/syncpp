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

#ifndef SYNSAMPLE_CORE_AST_TYPE_H_INCLUDED
#define SYNSAMPLE_CORE_AST_TYPE_H_INCLUDED

#include "basetype.h"
#include "common.h"
#include "gc.h"
#include "name__dec.h"
#include "stringex.h"

namespace syn_script {
	namespace ast {

		typedef gc::Local<TextPos> SynPos;

		//
		//AstValue
		//
		
		class AstValue : public gc::Object {
			NONCOPYABLE(AstValue);

			gc::Ref<TextPos> m_pos;

		protected:
			AstValue(){}
			void gc_enumerate_refs() override;

		public:
			void initialize(const gc::Local<TextPos>& pos);

			const gc::Ref<TextPos>& pos() const;
		};

		//
		//AstInteger
		//

		class AstInteger : public AstValue {
			NONCOPYABLE(AstInteger);

			ScriptIntegerType m_value;

		public:
			AstInteger(){}
			void initialize(const gc::Local<TextPos>& pos, ScriptIntegerType value);

			ScriptIntegerType value() const;
		};

		//
		//AstFloat
		//

		class AstFloat : public AstValue {
			NONCOPYABLE(AstFloat);

			ScriptFloatType m_value;

		public:
			AstFloat(){}
			void initialize(const gc::Local<TextPos>& pos, ScriptFloatType value);

			ScriptFloatType value() const;
		};

		//
		//AstName
		//

		class AstName : public AstValue {
			NONCOPYABLE(AstName);

			gc::Ref<const NameInfo> m_info;

		public:
			AstName(){}
			void gc_enumerate_refs() override;
			void initialize(const gc::Local<TextPos>& pos, const gc::Local<const NameInfo>& info);

			const gc::Ref<const NameInfo>& get_info() const;
			const NameID& get_id() const;
			StringLoc get_str() const;
		};

		//
		//AstString
		//

		class AstString : public AstValue {
			NONCOPYABLE(AstString);

			StringRef m_value;

		public:
			AstString(){}
			void gc_enumerate_refs() override;
			void initialize(const gc::Local<TextPos>& pos, const StringLoc& value);

			StringLoc value() const;
		};

		//
		//AstBinOp
		//

		enum class AstBinOp {
			NONE,
			ADD,
			SUB,
			MUL,
			DIV,
			MOD,
			LAND,
			LOR,
			EQ,
			NE,
			LT,
			GT,
			LE,
			GE
		};

		//
		//AstUnOp
		//

		enum class AstUnOp {
			PLUS,
			MINUS,
			LNOT
		};

		//
		//ModifierType
		//

		enum class ModifierType {
			NONE,
			PRIVATE,
			PUBLIC
		};

		typedef gc::Local<AstInteger> SynInteger;
		typedef gc::Local<AstFloat> SynFloat;
		typedef gc::Local<AstName> SynName;
		typedef gc::Local<AstString> SynString;

	}
}

#endif//SYNSAMPLE_CORE_AST_TYPE_H_INCLUDED
