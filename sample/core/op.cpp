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

//Arithmetical operator classes implementation.

#include "common.h"
#include "gc.h"
#include "op.h"
#include "value.h"

namespace ss = syn_script;
namespace rt = ss::rt;
namespace gc = ss::gc;

//
//Operator
//

ss::RuntimeError rt::Operator::type_missmatch_error() const {
	return RuntimeError("Type missmatch");
}

//
//UnaryOperator
//

gc::Local<rt::Value> rt::UnaryOperator::evaluate(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a) const
{
	OperandType type = a->get_operand_type();
	return evaluate_0(context, type, a);
}

//
//PlusUnaryOperator
//

const rt::PlusUnaryOperator rt::PlusUnaryOperator::Instance;

gc::Local<rt::Value> rt::PlusUnaryOperator::evaluate_0(
	const gc::Local<ExecContext>& context,
	OperandType type,
	const gc::Local<Value>& a) const
{
	if (OperandType::INTEGER != type && OperandType::FLOAT != type) throw type_missmatch_error();
	return a;
}

//
//MinusUnaryOperator
//

const rt::MinusUnaryOperator rt::MinusUnaryOperator::Instance;

gc::Local<rt::Value> rt::MinusUnaryOperator::evaluate_0(
	const gc::Local<ExecContext>& context,
	OperandType type,
	const gc::Local<Value>& a) const
{
	if (OperandType::INTEGER == type) {
		ScriptIntegerType v = a->get_integer();
		const ScriptIntegerType z = 0;
		return context->get_value_factory()->get_integer_value(z - v);
	} else if (OperandType::FLOAT == type) {
		ScriptFloatType v = a->get_float();
		return context->get_value_factory()->get_float_value(-v);
	} else {
		throw type_missmatch_error();
	}
}

//
//LogicalNotUnaryOperator
//

const rt::LogicalNotUnaryOperator rt::LogicalNotUnaryOperator::Instance;

gc::Local<rt::Value> rt::LogicalNotUnaryOperator::evaluate_0(
	const gc::Local<ExecContext>& context,
	OperandType type,
	const gc::Local<Value>& a) const
{
	if (OperandType::BOOLEAN != type) throw type_missmatch_error();
	bool v = a->get_boolean();
	return context->get_value_factory()->get_boolean_value(!v);
}

//
//BinaryOperator
//

gc::Local<rt::Value> rt::BinaryOperator::evaluate_short(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a) const
{
	return nullptr;
}

gc::Local<rt::Value> rt::BinaryOperator::evaluate(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b) const
{
	OperandType type_a = a->get_operand_type();
	OperandType type_b = b->get_operand_type();
	return evaluate_by_type(context, a, b, type_a, type_b);
}

gc::Local<rt::Value> rt::BinaryOperator::evaluate_by_type(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	OperandType type_a,
	OperandType type_b) const
{
	if (OperandType::FLOAT == type_a || OperandType::FLOAT == type_b) {
		ScriptFloatType value_a = float_promotion(a, type_a);
		ScriptFloatType value_b = float_promotion(b, type_b);
		return evaluate_float(context, a, b, value_a, value_b);
	} else if (type_a != type_b) {
		throw type_missmatch_error();
	}

	if (OperandType::BOOLEAN == type_a) {
		bool value_a = a->get_boolean();
		bool value_b = b->get_boolean();
		return evaluate_boolean(context, a, b, value_a, value_b);
	} else if (OperandType::INTEGER == type_a) {
		ScriptIntegerType value_a = a->get_integer();
		ScriptIntegerType value_b = b->get_integer();
		return evaluate_integer(context, a, b, value_a, value_b);
	} else if (OperandType::STRING == type_a) {
		StringLoc value_a = a->get_string();
		StringLoc value_b = b->get_string();
		return evaluate_string(context, a, b, value_a, value_b);
	} else {
		throw type_missmatch_error();
	}
}

gc::Local<rt::Value> rt::BinaryOperator::evaluate_integer(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	ss::ScriptIntegerType value_a,
	ss::ScriptIntegerType value_b) const
{
	throw type_missmatch_error();
}

gc::Local<rt::Value> rt::BinaryOperator::evaluate_float(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	ss::ScriptFloatType value_a,
	ss::ScriptFloatType value_b) const
{
	throw type_missmatch_error();
}

gc::Local<rt::Value> rt::BinaryOperator::evaluate_boolean(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	bool value_a,
	bool value_b) const
{
	throw type_missmatch_error();
}

gc::Local<rt::Value> rt::BinaryOperator::evaluate_string(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	const ss::StringLoc& value_a,
	const ss::StringLoc& value_b) const
{
	throw type_missmatch_error();
}

ss::ScriptFloatType rt::BinaryOperator::float_promotion(
	const gc::Local<Value>& value,
	OperandType type) const
{
	if (OperandType::INTEGER == type) {
		return static_cast<ScriptFloatType>(value->get_integer());
	} else if (OperandType::FLOAT == type) {
		return value->get_float();
	} else {
		throw type_missmatch_error();
	}
}

//
//LogicalBinaryOperator
//

gc::Local<rt::Value> rt::LogicalBinaryOperator::evaluate_short(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a) const
{
	if (OperandType::BOOLEAN == a->get_operand_type()) {
		bool value = a->get_boolean();
		bool result;
		if (evaluate_short_boolean(value, &result)) {
			return context->get_value_factory()->get_boolean_value(result);
		} else {
			return nullptr;
		}
	} else {
		throw type_missmatch_error();
	}
}

gc::Local<rt::Value> rt::LogicalBinaryOperator::evaluate_boolean(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	bool value_a,
	bool value_b) const
{
	bool result = evaluate_boolean_boolean(value_a, value_b);
	return context->get_value_factory()->get_boolean_value(result);
}

//
//LogicalOrBinaryOperator
//

const rt::LogicalOrBinaryOperator rt::LogicalOrBinaryOperator::Instance;

bool rt::LogicalOrBinaryOperator::evaluate_short_boolean(bool a, bool* result) const {
	if (a) {
		*result = true;
		return true;
	}
	return false;
}

bool rt::LogicalOrBinaryOperator::evaluate_boolean_boolean(bool a, bool b) const {
	return a || b;
}

//
//LogicalAndBinaryOperator
//

const rt::LogicalAndBinaryOperator rt::LogicalAndBinaryOperator::Instance;

bool rt::LogicalAndBinaryOperator::evaluate_short_boolean(bool a, bool* result) const {
	if (!a) {
		*result = false;
		return true;
	}
	return false;
}

bool rt::LogicalAndBinaryOperator::evaluate_boolean_boolean(bool a, bool b) const {
	return a && b;
}

//
//EqNeBinaryOperator
//

namespace {
	bool is_reference(rt::OperandType type) {
		return rt::OperandType::REFERENCE == type;
	}

	bool is_reference_or_string(rt::OperandType type) {
		return rt::OperandType::REFERENCE == type || rt::OperandType::STRING == type;
	}
}

gc::Local<rt::Value> rt::EqNeBinaryOperator::evaluate_by_type(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	OperandType type_a,
	OperandType type_b) const
{
	bool null_a = a->is_null();
	bool null_b = b->is_null();

	if (null_a || null_b) {
		return get_result_value(context, null_a && null_b);
	} else if (is_reference(type_a) && is_reference(type_b)) {
		bool eq = a.get() == b.get();
		return get_result_value(context, eq);
	}

	return BinaryOperator::evaluate_by_type(context, a, b, type_a, type_b);
}

gc::Local<rt::Value> rt::EqNeBinaryOperator::evaluate_integer(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	ss::ScriptIntegerType value_a,
	ss::ScriptIntegerType value_b) const
{
	return get_result_value(context, value_a == value_b);
}

gc::Local<rt::Value> rt::EqNeBinaryOperator::evaluate_float(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	ss::ScriptFloatType value_a,
	ss::ScriptFloatType value_b) const
{
	return get_result_value(context, value_a == value_b);
}

gc::Local<rt::Value> rt::EqNeBinaryOperator::evaluate_boolean(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	bool value_a,
	bool value_b) const
{
	return get_result_value(context, value_a == value_b);
}

gc::Local<rt::Value> rt::EqNeBinaryOperator::evaluate_string(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	const ss::StringLoc& value_a,
	const ss::StringLoc& value_b) const
{
	return get_result_value(context, value_a->equals(value_b));
}

gc::Local<rt::Value> rt::EqNeBinaryOperator::get_result_value(
	const gc::Local<ExecContext>& context,
	bool eq) const
{
	bool result = get_result(eq);
	return context->get_value_factory()->get_boolean_value(result);
}

//
//EqBinaryOperator
//

const rt::EqBinaryOperator rt::EqBinaryOperator::Instance;

bool rt::EqBinaryOperator::get_result(bool eq) const {
	return eq;
}

//
//NeBinaryOperator
//

const rt::NeBinaryOperator rt::NeBinaryOperator::Instance;

bool rt::NeBinaryOperator::get_result(bool eq) const {
	return !eq;
}

//
//RelBinaryOperator
//

gc::Local<rt::Value> rt::RelBinaryOperator::evaluate_integer(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	ss::ScriptIntegerType value_a,
	ss::ScriptIntegerType value_b) const
{
	ScriptIntegerType diff = value_a - value_b;

	int rel;
	if (diff == 0) {
		rel = 0;
	} else {
		int sign = scriptint_sign(diff);
		rel = sign > 0 ? 1 : -1;
	}

	bool result = get_result(rel);
	return context->get_value_factory()->get_boolean_value(result);
}

gc::Local<rt::Value> rt::RelBinaryOperator::evaluate_float(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	ss::ScriptFloatType value_a,
	ss::ScriptFloatType value_b) const
{
	int rel = (value_a < value_b ? -1 : (value_a > value_b ? 1 : 0));
	bool result = get_result(rel);
	return context->get_value_factory()->get_boolean_value(result);
}

gc::Local<rt::Value> rt::RelBinaryOperator::evaluate_string(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	const ss::StringLoc& value_a,
	const ss::StringLoc& value_b) const
{
	int rel = value_a->compare_to(value_b);
	bool result = get_result(rel);
	return context->get_value_factory()->get_boolean_value(result);
}

//
//LtBinaryOperator
//

const rt::LtBinaryOperator rt::LtBinaryOperator::Instance;

bool rt::LtBinaryOperator::get_result(int rel) const {
	return rel < 0;
}

//
//GtBinaryOperator
//

const rt::GtBinaryOperator rt::GtBinaryOperator::Instance;

bool rt::GtBinaryOperator::get_result(int rel) const {
	return rel > 0;
}

//
//LeBinaryOperator
//

const rt::LeBinaryOperator rt::LeBinaryOperator::Instance;

bool rt::LeBinaryOperator::get_result(int rel) const {
	return rel <= 0;
}

//
//GeBinaryOperator
//

const rt::GeBinaryOperator rt::GeBinaryOperator::Instance;

bool rt::GeBinaryOperator::get_result(int rel) const {
	return rel >= 0;
}

//
//ArithmeticalBinaryOperator
//

gc::Local<rt::Value> rt::ArithmeticalBinaryOperator::evaluate_integer(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	ss::ScriptIntegerType value_a,
	ss::ScriptIntegerType value_b) const
{
	ScriptIntegerType result = evaluate_integer_integer(value_a, value_b);
	return context->get_value_factory()->get_integer_value(result);
}

gc::Local<rt::Value> rt::ArithmeticalBinaryOperator::evaluate_float(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	ss::ScriptFloatType value_a,
	ss::ScriptFloatType value_b) const
{
	ScriptFloatType result = evaluate_float_float(value_a, value_b);
	return context->get_value_factory()->get_float_value(result);
}

//
//AddBinaryOperator
//

const rt::AddBinaryOperator rt::AddBinaryOperator::Instance;

gc::Local<rt::Value> rt::AddBinaryOperator::evaluate_by_type(
	const gc::Local<ExecContext>& context,
	const gc::Local<Value>& a,
	const gc::Local<Value>& b,
	OperandType type_a,
	OperandType type_b) const
{
	if (OperandType::STRING == type_a || OperandType::STRING == type_b) {
		StringLoc str_a = OperandType::STRING == type_a ? a->get_string() : a->to_string(context);
		StringLoc str_b = OperandType::STRING == type_b ? b->get_string() : b->to_string(context);
		return context->get_value_factory()->get_string_value(str_a + str_b);
	} else {
		return BinaryOperator::evaluate_by_type(context, a, b, type_a, type_b);
	}
}

ss::ScriptIntegerType rt::AddBinaryOperator::evaluate_integer_integer(
	ss::ScriptIntegerType a,
	ss::ScriptIntegerType b) const
{
	return a + b;
}

ss::ScriptFloatType rt::AddBinaryOperator::evaluate_float_float(
	ss::ScriptFloatType a,
	ss::ScriptFloatType b) const
{
	//TODO Handle floating-point overflow.
	return a + b;
}

//
//SubBinaryOperator
//

const rt::SubBinaryOperator rt::SubBinaryOperator::Instance;

ss::ScriptIntegerType rt::SubBinaryOperator::evaluate_integer_integer(
	ss::ScriptIntegerType a,
	ss::ScriptIntegerType b) const
{
	return a - b;
}

ss::ScriptFloatType rt::SubBinaryOperator::evaluate_float_float(
	ss::ScriptFloatType a,
	ss::ScriptFloatType b) const
{
	//TODO Handle floating-point overflow.
	return a - b;
}

//
//MulBinaryOperator
//

const rt::MulBinaryOperator rt::MulBinaryOperator::Instance;

ss::ScriptIntegerType rt::MulBinaryOperator::evaluate_integer_integer(
	ss::ScriptIntegerType a,
	ss::ScriptIntegerType b) const
{
	return a * b;
}

ss::ScriptFloatType rt::MulBinaryOperator::evaluate_float_float(
	ss::ScriptFloatType a,
	ss::ScriptFloatType b) const
{
	//TODO Handle floating-point overflow.
	return a * b;
}

//
//DivBinaryOperator
//

const rt::DivBinaryOperator rt::DivBinaryOperator::Instance;

ss::ScriptIntegerType rt::DivBinaryOperator::evaluate_integer_integer(
	ss::ScriptIntegerType a,
	ss::ScriptIntegerType b) const
{
	if (!b) throw RuntimeError("Division by zero");
	return a / b;
}

ss::ScriptFloatType rt::DivBinaryOperator::evaluate_float_float(
	ss::ScriptFloatType a,
	ss::ScriptFloatType b) const
{
	//TODO Handle floating-point overflow/underflow.
	return a / b;
}

//
//ModBinaryOperator
//

const rt::ModBinaryOperator rt::ModBinaryOperator::Instance;

ss::ScriptIntegerType rt::ModBinaryOperator::evaluate_integer_integer(
	ss::ScriptIntegerType a,
	ss::ScriptIntegerType b) const
{
	if (!b) throw RuntimeError("Division by zero");
	return a % b;
}

ss::ScriptFloatType rt::ModBinaryOperator::evaluate_float_float(
	ss::ScriptFloatType a,
	ss::ScriptFloatType b) const
{
	throw RuntimeError("Floating-point remainder operator is not supported");
}
