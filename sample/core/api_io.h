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

//Input/output API values.

#ifndef SYNSAMPLE_CORE_API_IO_H_INCLUDED
#define SYNSAMPLE_CORE_API_IO_H_INCLUDED

#include <fstream>
#include <ostream>

#include "api.h"
#include "gc.h"
#include "scope__dec.h"
#include "sysclassbld.h"
#include "sysvalue.h"
#include "value__dec.h"

namespace syn_script {
	namespace rt {

		//
		//TextOutputValue
		//

		class TextOutputValue : public SysObjectValue {
			NONCOPYABLE(TextOutputValue);

			friend class SysAPI<TextOutputValue>;
			class API;

		protected:
			TextOutputValue(){}

		public:
			virtual std::ostream& get_out() = 0;

		protected:
			std::size_t get_sys_class_id() const override;

			virtual void close();

		private:
			void api_print(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
			void api_println_0(const gc::Local<ExecContext>& context);
			void api_println_1(const gc::Local<ExecContext>& context, const gc::Local<Value>& value);
			void api_close(const gc::Local<ExecContext>& context);
		};

		//
		//FileTextOutputValue
		//

		class FileTextOutputValue : public TextOutputValue {
			NONCOPYABLE(FileTextOutputValue);

			std::ofstream m_out;

		public:
			FileTextOutputValue(){}
			void initialize(const StringLoc& path, bool append);

			std::ostream& get_out() override;

		protected:
			void close() override;
		};

		//
		//BinaryInputValue
		//

		class BinaryInputValue : public SysObjectValue {
			NONCOPYABLE(BinaryInputValue);

			friend class SysAPI<BinaryInputValue>;
			class API;

		protected:
			BinaryInputValue(){}

		protected:
			std::size_t get_sys_class_id() const override;

			virtual int read() = 0;
			virtual int read(const gc::Local<ByteArray>& buffer, std::size_t ofs, std::size_t len);
			virtual int read(const gc::Local<ByteArray>& buffer);
			virtual void close();

		private:
			ScriptIntegerType api_read_byte(const gc::Local<ExecContext>& context);
			
			ScriptIntegerType api_read_3(
				const gc::Local<ExecContext>& context,
				const gc::Local<ByteArray>& buffer,
				ScriptIntegerType ofs,
				ScriptIntegerType len);

			ScriptIntegerType api_read_1(const gc::Local<ExecContext>& context, const gc::Local<ByteArray>& buffer);

			virtual void api_close(const gc::Local<ExecContext>& context);
		};

		//
		//BinaryOutputValue
		//

		class BinaryOutputValue : public SysObjectValue {
			NONCOPYABLE(BinaryOutputValue);

			friend class SysAPI<BinaryOutputValue>;
			class API;

		protected:
			BinaryOutputValue(){}

		protected:
			std::size_t get_sys_class_id() const override;

			virtual void write(int value) = 0;
			virtual void write(const gc::Local<ByteArray>& buffer, std::size_t ofs, std::size_t len);
			virtual void write(const gc::Local<ByteArray>& buffer);
			virtual void close();

		private:
			void api_write_byte(const gc::Local<ExecContext>& context, ScriptIntegerType value);
			
			void api_write_3(
				const gc::Local<ExecContext>& context,
				const gc::Local<ByteArray>& buffer,
				ScriptIntegerType ofs,
				ScriptIntegerType len);

			void api_write_1(const gc::Local<ExecContext>& context, const gc::Local<ByteArray>& buffer);
			void api_close(const gc::Local<ExecContext>& context);
		};

		//
		//FileBinaryInputValue
		//

		class FileBinaryInputValue : public BinaryInputValue {
			NONCOPYABLE(FileBinaryInputValue);

			std::ifstream m_in;

		public:
			FileBinaryInputValue(){}
			void initialize(const StringLoc& path);

		protected:
			int read() override;
			virtual int read(const gc::Local<ByteArray>& buffer, std::size_t ofs, std::size_t len) override;
			void close() override;
		};

		//
		//FileBinaryOutputValue
		//

		class FileBinaryOutputValue : public BinaryOutputValue {
			NONCOPYABLE(FileBinaryOutputValue);

			std::ofstream m_out;

		public:
			FileBinaryOutputValue(){}
			void initialize(const StringLoc& path, bool append);

		protected:
			void write(int value) override;
			void write(const gc::Local<ByteArray>& buffer, std::size_t ofs, std::size_t len) override;
			void close() override;
		};

	}
}

#endif//SYNSAMPLE_CORE_API_IO_H_INCLUDED
