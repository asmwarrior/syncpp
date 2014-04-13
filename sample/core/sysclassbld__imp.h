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

//Implementation of SysClass builder templates.

#ifndef SYNSAMPLE_CORE_SYSCLASSBLD_IMP_H_INCLUDED
#define SYNSAMPLE_CORE_SYSCLASSBLD_IMP_H_INCLUDED

#include "common.h"
#include "gc.h"
#include "name__dec.h"
#include "sysclass__dec.h"
#include "sysclassbld.h"
#include "value.h"

//
//MethodAdapter
//

class syn_script::rt::MethodAdapter : public gc::Object {
	NONCOPYABLE(MethodAdapter);

protected:
	MethodAdapter(){}

public:
	virtual bool accepts_arguments(const gc::Local<gc::Array<Value>>& arguments) const = 0;
};

//
//StaticFieldAdapter
//

class syn_script::rt::StaticFieldAdapter : public gc::Object {
	NONCOPYABLE(StaticFieldAdapter);

protected:
	StaticFieldAdapter(){}

public:
	virtual gc::Local<Value> get(const gc::Local<ExecContext>& context) const = 0;
};

//
//StaticMethodAdapter
//

class syn_script::rt::StaticMethodAdapter : public MethodAdapter {
	NONCOPYABLE(StaticMethodAdapter);

protected:
	StaticMethodAdapter(){}

public:
	virtual gc::Local<Value> invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments) const = 0;
};

//
//DynamicFieldAdapter
//

class syn_script::rt::DynamicFieldAdapter : public gc::Object {
	NONCOPYABLE(DynamicFieldAdapter);

protected:
	DynamicFieldAdapter(){}

public:
	virtual gc::Local<Value> get(
		const gc::Local<ExecContext>& context,
		const gc::Local<Value>& object) const = 0;
};

//
//DynamicMethodAdapter
//

class syn_script::rt::DynamicMethodAdapter : public MethodAdapter {
	NONCOPYABLE(DynamicMethodAdapter);

protected:
	DynamicMethodAdapter(){}

public:
	virtual gc::Local<Value> invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<Value>& object,
		const gc::Local<gc::Array<Value>>& arguments) const = 0;
};

namespace syn_script {
	namespace rt {
		class ClassStaticFieldAdapter;
		class ValueStaticFieldAdapter;
		template<class R> class GenericStaticFieldAdapter;
		template<class R, class... Args> class GenericStaticMethodAdapter;
		template<class T, class R> class GenericDynamicFieldAdapter;
		template<class T, class R, class... Args> class GenericDynamicMethodAdapter;

		gc::Local<Value> adapter__result(const gc::Local<ExecContext>& context, bool v);
		gc::Local<Value> adapter__result(const gc::Local<ExecContext>& context, ScriptIntegerType v);
		gc::Local<Value> adapter__result(const gc::Local<ExecContext>& context, const StringLoc& v);

		gc::Local<Value> adapter__result(
			const gc::Local<ExecContext>& context,
			const gc::Local<gc::Array<Value>>& v);

		template<class T>
		gc::Local<Value> adapter__result(const gc::Local<ExecContext>& context,	const gc::Local<T>& v) {
			if (!!v) return v;
			return context->get_value_factory()->get_null_value();
		}

		template<class R>
		struct AdapterGenericResult {
			template<class Fn>
			static gc::Local<Value> adapt(const gc::Local<ExecContext>& context, Fn fn) {
				return adapter__result(context, fn());
			}
		};

		template<>
		struct AdapterGenericResult<void> {
			template<class Fn>
			static gc::Local<Value> adapt(const gc::Local<ExecContext>& context, Fn fn) {
				fn();
				return context->get_value_factory()->get_void_value();
			}
		};

		template<>
		struct AdapterGenericResult<gc::Local<Value>> {
			template<class Fn>
			static gc::Local<Value> adapt(const gc::Local<ExecContext>& context, Fn fn) {
				return fn();
			}
		};

		template<class R, class ADAPTER>
		gc::Local<Value> adapter__get_field(
			const ADAPTER* adapter,
			const gc::Local<ExecContext>& context,
			const gc::Local<Value>& object)
		{
			return AdapterGenericResult<R>::adapt(context, [=]() -> R {
				return adapter->get_0(context, object);
			});
		}

		template<class... Args>
		bool adapter__check_arguments(const gc::Local<gc::Array<Value>>& arguments) {
			return sizeof...(Args) == arguments->length();
		}

		template<class T> class AdapterTag {};

		template<class... T> class AdapterTags {};

		bool adapter__convert_argument(
			const gc::Local<Value>& v,
			AdapterTag<bool>);

		ScriptIntegerType adapter__convert_argument(
			const gc::Local<Value>& v,
			AdapterTag<ScriptIntegerType>);

		StringLoc adapter__convert_argument(
			const gc::Local<Value>& v,
			AdapterTag<const StringLoc&>);

		gc::Local<ByteArray> adapter__convert_argument(
			const gc::Local<Value>& v,
			AdapterTag<const gc::Local<ByteArray>&>);

		const gc::Local<Value>& adapter__convert_argument(
			const gc::Local<Value>& v,
			AdapterTag<const gc::Local<Value>&>);

		gc::Local<gc::Array<Value>> adapter__convert_argument(
			const gc::Local<Value>& v,
			AdapterTag<const gc::Local<gc::Array<Value>>&>);

		template<class T>
		gc::Local<T> adapter__convert_argument_gclocal(const gc::Local<Value>& v, AdapterTag<T>, const Value*) {
			if (!v) return nullptr;
			gc::Local<T> result = v.cast_opt<T>();
			if (!result) throw RuntimeError("Wrong argument type");
			return result;
		}

		template<class T>
		gc::Local<T> adapter__convert_argument(const gc::Local<Value>& v, AdapterTag<const gc::Local<T>&>) {
			return adapter__convert_argument_gclocal(v, AdapterTag<T>(), static_cast<const T*>(nullptr));
		}

		template<class R, class ADAPTER, class... LArgs>
		R adapter__convert_arguments_recursively(
			const ADAPTER* adapter,
			const gc::Local<ExecContext>& context,
			const gc::Local<Value>& object,
			const gc::Local<gc::Array<Value>>& arguments,
			AdapterTags<>,
			LArgs... values)
		{
			return adapter->invoke_0(context, object, values...);
		}

		template<class R, class ADAPTER, class... LArgs, class MArg, class... RArgs>
		R adapter__convert_arguments_recursively(
			const ADAPTER* adapter,
			const gc::Local<ExecContext>& context,
			const gc::Local<Value>& object,
			const gc::Local<gc::Array<Value>>& arguments,
			AdapterTags<MArg, RArgs...>,
			LArgs... values)
		{
			std::size_t idx = sizeof...(LArgs);
			gc::Local<Value> argument = arguments->get(idx);
			assert(!!argument);

			return adapter__convert_arguments_recursively<R>(
				adapter,
				context,
				object,
				arguments,
				AdapterTags<RArgs...>(),
				values...,
				adapter__convert_argument(argument, AdapterTag<MArg>()));
		}

		template<class R, class... Args, class ADAPTER>
		gc::Local<Value> adapter__invoke_method(
			const ADAPTER* adapter,
			const gc::Local<ExecContext>& context,
			const gc::Local<Value>& object,
			const gc::Local<gc::Array<Value>>& arguments)
		{
			return AdapterGenericResult<R>::adapt(context, [=]() -> R {
				return adapter__convert_arguments_recursively<R>(
					adapter,
					context,
					object,
					arguments,
					AdapterTags<Args...>());
			});
		}
	}
}

//
//ClassStaticFieldAdapter
//

class syn_script::rt::ClassStaticFieldAdapter : public StaticFieldAdapter {
	NONCOPYABLE(ClassStaticFieldAdapter);

	const BasicSysClassInitializer* m_class_initializer;

public:
	ClassStaticFieldAdapter(){}

	void initialize(const BasicSysClassInitializer* class_initializer) {
		m_class_initializer = class_initializer;
	}

	gc::Local<Value> get(const gc::Local<ExecContext>& context) const override {
		std::size_t class_id = m_class_initializer->get_class_id();
		gc::Local<SysClass> cls = context->get_value_factory()->get_sys_class(class_id);
		return cls->get_class_value();
	}
};

//
//ValueStaticFieldAdapter
//

class syn_script::rt::ValueStaticFieldAdapter : public StaticFieldAdapter {
	NONCOPYABLE(ValueStaticFieldAdapter);

	gc::Ref<Value> m_value;

public:
	ValueStaticFieldAdapter(){}

	void gc_enumerate_refs() override {
		gc_ref(m_value);
	}

	void initialize(const gc::Local<Value>& value) {
		m_value = value;
	}

	gc::Local<Value> get(const gc::Local<ExecContext>& context) const override {
		return m_value;
	}
};

//
//GenericStaticFieldAdapter
//

template<class R>
class syn_script::rt::GenericStaticFieldAdapter : public StaticFieldAdapter {
	NONCOPYABLE(GenericStaticFieldAdapter);

	typedef R (*FieldFn)(const gc::Local<ExecContext>&);

	FieldFn m_fn;

public:
	GenericStaticFieldAdapter(){}

	void initialize(FieldFn fn) {
		m_fn = fn;
	}

	inline R get_0(
		const gc::Local<ExecContext>& context,
		const gc::Local<Value>& object) const
	{
		return m_fn(context);
	}

	gc::Local<Value> get(const gc::Local<ExecContext>& context) const override {
		return adapter__get_field<R>(this, context, nullptr);
	}
};

//
//GenericStaticMethodAdapter
//

template<class R, class... Args>
class syn_script::rt::GenericStaticMethodAdapter : public StaticMethodAdapter {
	NONCOPYABLE(GenericStaticMethodAdapter);

	typedef R (*MethodFn)(const gc::Local<ExecContext>&, Args...);

	MethodFn m_fn;

public:
	GenericStaticMethodAdapter(){}

	void initialize(MethodFn fn) {
		m_fn = fn;
	}

	inline R invoke_0(const gc::Local<ExecContext>& context, const gc::Local<Value>& object, Args... args) const {
		return m_fn(context, args...);
	}

	bool accepts_arguments(const gc::Local<gc::Array<Value>>& arguments) const override {
		return adapter__check_arguments<Args...>(arguments);
	}

	gc::Local<Value> invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<gc::Array<Value>>& arguments) const override
	{
		return adapter__invoke_method<R, Args...>(this, context, nullptr, arguments);
	}
};

//
//GenericDynamicFieldAdapter
//

template<class T, class R>
class syn_script::rt::GenericDynamicFieldAdapter : public DynamicFieldAdapter {
	NONCOPYABLE(GenericDynamicFieldAdapter);

	typedef R (T::*FieldFn)(const gc::Local<ExecContext>&);

	FieldFn m_fn;

public:
	GenericDynamicFieldAdapter(){}

	void initialize(FieldFn fn) {
		m_fn = fn;
	}

	inline R get_0(
		const gc::Local<ExecContext>& context,
		const gc::Local<Value>& object) const
	{
		T* concrete_object = object.cast<T>().get();
		return (concrete_object->*m_fn)(context);
	}

	gc::Local<Value> get(
		const gc::Local<ExecContext>& context,
		const gc::Local<Value>& object) const override
	{
		return adapter__get_field<R>(this, context, object);
	}
};

//
//GenericDynamicMethodAdapter
//

template<class T, class R, class... Args>
class syn_script::rt::GenericDynamicMethodAdapter : public DynamicMethodAdapter {
	NONCOPYABLE(GenericDynamicMethodAdapter);

	typedef R (T::*MethodFn)(const gc::Local<ExecContext>&, Args...);

	MethodFn m_fn;

public:
	GenericDynamicMethodAdapter(){}

	void initialize(MethodFn fn) {
		m_fn = fn;
	}

	inline R invoke_0(const gc::Local<ExecContext>& context, const gc::Local<Value>& object, Args... args) const {
		T* concrete_object = object.cast<T>().get();
		return (concrete_object->*m_fn)(context, args...);
	}

	bool accepts_arguments(const gc::Local<gc::Array<Value>>& arguments) const override {
		return adapter__check_arguments<Args...>(arguments);
	}

	gc::Local<Value> invoke(
		const gc::Local<ExecContext>& context,
		const gc::Local<Value>& object,
		const gc::Local<gc::Array<Value>>& arguments) const override
	{
		return adapter__invoke_method<R, Args...>(this, context, object, arguments);
	}
};

//
//SysClassBuilder
//

template<class T>
template<class C>
void syn_script::rt::SysClassBuilder<T>::add_class(const char* name) {
	add_static_field_0(name, gc::create<ClassStaticFieldAdapter>(C::API::get_initializer()));
}

template<class T>
template<class... Args>
void syn_script::rt::SysClassBuilder<T>::add_constructor(
	gc::Local<T> (*fn)(const gc::Local<ExecContext>&, Args...))
{
	add_constructor_0(gc::create<GenericStaticMethodAdapter<gc::Local<T>, Args...>>(fn));
}

template<class T>
void syn_script::rt::SysClassBuilder<T>::add_static_field(const char* name, const gc::Local<Value>& value) {
	add_static_field_0(name, gc::create<ValueStaticFieldAdapter>(value));
}

template<class T>
template<class R>
void syn_script::rt::SysClassBuilder<T>::add_static_field(
	const char* name,
	R (*fn)(const gc::Local<ExecContext>& context))
{
	add_static_field_0(name, gc::create<GenericStaticFieldAdapter<R>>(fn));
}

template<class T>
template<class R, class... Args>
void syn_script::rt::SysClassBuilder<T>::add_static_method(
	const char* name,
	R (*fn)(const gc::Local<ExecContext>&, Args...))
{
	add_static_method_0(name, gc::create<GenericStaticMethodAdapter<R, Args...>>(fn));
}

template<class T>
template<class R>
void syn_script::rt::SysClassBuilder<T>::add_field(
	const char* name,
	R (T::*fn)(const gc::Local<ExecContext>& context))
{
	add_field_0(name, gc::create<GenericDynamicFieldAdapter<T, R>>(fn));
}

template<class T>
template<class R, class... Args>
void syn_script::rt::SysClassBuilder<T>::add_method(
	const char* name,
	R (T::*fn)(const gc::Local<ExecContext>&, Args...))
{
	add_method_0(name, gc::create<GenericDynamicMethodAdapter<T, R, Args...>>(fn));
}

//
//SysAPI
//

template<class T>
class syn_script::rt::SysAPI : public SysClassInitializer::Adapter {
	NONCOPYABLE(SysAPI);

	static rt::SysClassInitializer s_init;

protected:
	rt::SysClassBuilder<T>* bld;

protected:
	SysAPI(){}

	virtual void init() = 0;

public:
	gc::Local<SysClass> create_sys_class(NameRegistry& name_registry) override {
		SysClassBuilder<T> local_bld(name_registry);
		bld = &local_bld;
		init();
		bld = nullptr;
		return local_bld.create_sys_class();
	}

	static std::size_t get_class_id() {
		return s_init.get_class_id();
	}

	static const rt::SysClassInitializer* get_initializer() {
		return &s_init;
	}
};

template<class T>
syn_script::rt::SysClassInitializer syn_script::rt::SysAPI<T>::s_init(new typename T::API());

#endif//SYNSAMPLE_CORE_SYSCLASSBLD_IMP_H_INCLUDED
