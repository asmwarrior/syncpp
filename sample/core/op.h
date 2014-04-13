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

//Definition of operator classes.

#ifndef SYNSAMPLE_CORE_OP_H_INCLUDED
#define SYNSAMPLE_CORE_OP_H_INCLUDED

#include "ast_type.h"
#include "gc.h"
#include "value__dec.h"
#include "scope.h"

namespace syn_script {
	namespace rt {

		//
		//Operator
		//

		class Operator {
			NONCOPYABLE(Operator);

		protected:
			Operator(){}

			RuntimeError type_missmatch_error() const;
		};

		//
		//UnaryOperator
		//

		class UnaryOperator : public Operator {
			NONCOPYABLE(UnaryOperator);

		protected:
			UnaryOperator(){}

		public:
			gc::Local<Value> evaluate(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a) const;

		protected:
			virtual gc::Local<Value> evaluate_0(
				const gc::Local<ExecContext>& context,
				OperandType type,
				const gc::Local<Value>& a) const = 0;
		};

		//
		//PlusUnaryOperator
		//

		class PlusUnaryOperator : public UnaryOperator {
			NONCOPYABLE(PlusUnaryOperator);

			PlusUnaryOperator(){}

		public:
			static const PlusUnaryOperator Instance;

		protected:
			gc::Local<Value> evaluate_0(
				const gc::Local<ExecContext>& context,
				OperandType type,
				const gc::Local<Value>& a) const override;
		};

		//
		//MinusUnaryOperator
		//

		class MinusUnaryOperator : public UnaryOperator {
			NONCOPYABLE(MinusUnaryOperator);

			MinusUnaryOperator(){}

		public:
			static const MinusUnaryOperator Instance;

		protected:
			gc::Local<Value> evaluate_0(
				const gc::Local<ExecContext>& context,
				OperandType type,
				const gc::Local<Value>& a) const override;
		};

		//
		//LogicalNotUnaryOperator
		//

		class LogicalNotUnaryOperator : public UnaryOperator {
			NONCOPYABLE(LogicalNotUnaryOperator);

			LogicalNotUnaryOperator(){}

		public:
			static const LogicalNotUnaryOperator Instance;

		protected:
			gc::Local<Value> evaluate_0(
				const gc::Local<ExecContext>& context,
				OperandType type,
				const gc::Local<Value>& a) const override;
		};

		//
		//BinaryOperator
		//

		class BinaryOperator : public Operator {
			NONCOPYABLE(BinaryOperator);

		protected:
			BinaryOperator(){}

		public:
			virtual gc::Local<Value> evaluate_short(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a) const;

			gc::Local<Value> evaluate(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b) const;

		protected:
			virtual gc::Local<Value> evaluate_by_type(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				OperandType type_a,
				OperandType type_b) const;

			virtual gc::Local<Value> evaluate_integer(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				ScriptIntegerType value_a,
				ScriptIntegerType value_b) const;

			virtual gc::Local<Value> evaluate_float(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				ScriptFloatType value_a,
				ScriptFloatType value_b) const;

			virtual gc::Local<Value> evaluate_boolean(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				bool value_a,
				bool value_b) const;

			virtual gc::Local<Value> evaluate_string(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				const StringLoc& value_a,
				const StringLoc& value_b) const;

		private:
			ScriptFloatType float_promotion(const gc::Local<Value>& value, OperandType type) const;
		};

		//
		//LogicalBinaryOperator
		//

		class LogicalBinaryOperator : public BinaryOperator {
			NONCOPYABLE(LogicalBinaryOperator);

		protected:
			LogicalBinaryOperator(){}

		public:
			gc::Local<Value> evaluate_short(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a) const override final;

		protected:
			gc::Local<Value> evaluate_boolean(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				bool value_a,
				bool value_b) const override;

			virtual bool evaluate_short_boolean(bool value, bool* result) const = 0;
			virtual bool evaluate_boolean_boolean(bool a, bool b) const = 0;
		};

		//
		//LogicalOrBinaryOperator
		//

		class LogicalOrBinaryOperator : public LogicalBinaryOperator {
			NONCOPYABLE(LogicalOrBinaryOperator);

			LogicalOrBinaryOperator(){}

		public:
			static const LogicalOrBinaryOperator Instance;

		protected:
			bool evaluate_short_boolean(bool a, bool* result) const override;
			bool evaluate_boolean_boolean(bool a, bool b) const override;
		};

		//
		//LogicalAndBinaryOperator
		//

		class LogicalAndBinaryOperator : public LogicalBinaryOperator {
			NONCOPYABLE(LogicalAndBinaryOperator);

			LogicalAndBinaryOperator(){}

		public:
			static const LogicalAndBinaryOperator Instance;

		protected:
			bool evaluate_short_boolean(bool a, bool* result) const override;
			bool evaluate_boolean_boolean(bool a, bool b) const override;
		};

		//
		//EqNeBinaryOperator
		//

		class EqNeBinaryOperator : public BinaryOperator {
			NONCOPYABLE(EqNeBinaryOperator);

		protected:
			EqNeBinaryOperator(){}

			gc::Local<Value> evaluate_by_type(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				OperandType type_a,
				OperandType type_b) const override final;

			gc::Local<Value> evaluate_integer(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				ScriptIntegerType value_a,
				ScriptIntegerType value_b) const override final;

			gc::Local<Value> evaluate_float(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				ScriptFloatType value_a,
				ScriptFloatType value_b) const override final;

			gc::Local<Value> evaluate_boolean(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				bool value_a,
				bool value_b) const override final;

			gc::Local<Value> evaluate_string(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				const StringLoc& value_a,
				const StringLoc& value_b) const override final;

			virtual bool get_result(bool eq) const = 0;

		private:
			gc::Local<Value> get_result_value(const gc::Local<ExecContext>& context, bool eq) const;
		};

		//
		//EqBinaryOperator
		//

		class EqBinaryOperator : public EqNeBinaryOperator {
			NONCOPYABLE(EqBinaryOperator);

			EqBinaryOperator(){}

		public:
			static const EqBinaryOperator Instance;

		protected:
			bool get_result(bool eq) const override;
		};

		//
		//NeBinaryOperator
		//

		class NeBinaryOperator : public EqNeBinaryOperator {
			NONCOPYABLE(NeBinaryOperator);

			NeBinaryOperator(){}

		public:
			static const NeBinaryOperator Instance;

		protected:
			bool get_result(bool eq) const override;
		};

		//
		//RelBinaryOperator
		//

		class RelBinaryOperator : public BinaryOperator {
			NONCOPYABLE(RelBinaryOperator);

		protected:
			RelBinaryOperator(){}

			gc::Local<Value> evaluate_integer(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				ScriptIntegerType value_a,
				ScriptIntegerType value_b) const override final;

			gc::Local<Value> evaluate_float(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				ScriptFloatType value_a,
				ScriptFloatType value_b) const override final;

			gc::Local<Value> evaluate_string(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				const StringLoc& value_a,
				const StringLoc& value_b) const override final;

			virtual bool get_result(int rel) const = 0;
		};

		//
		//LtBinaryOperator
		//

		class LtBinaryOperator : public RelBinaryOperator {
			NONCOPYABLE(LtBinaryOperator);

			LtBinaryOperator(){}

		public:
			static const LtBinaryOperator Instance;

		protected:
			bool get_result(int rel) const override;
		};

		//
		//GtBinaryOperator
		//

		class GtBinaryOperator : public RelBinaryOperator {
			NONCOPYABLE(GtBinaryOperator);

			GtBinaryOperator(){}

		public:
			static const GtBinaryOperator Instance;

		protected:
			bool get_result(int rel) const override;
		};

		//
		//LeBinaryOperator
		//

		class LeBinaryOperator : public RelBinaryOperator {
			NONCOPYABLE(LeBinaryOperator);

			LeBinaryOperator(){}

		public:
			static const LeBinaryOperator Instance;

		protected:
			bool get_result(int rel) const override;
		};

		//
		//GeBinaryOperator
		//

		class GeBinaryOperator : public RelBinaryOperator {
			NONCOPYABLE(GeBinaryOperator);

			GeBinaryOperator(){}

		public:
			static const GeBinaryOperator Instance;

		protected:
			bool get_result(int rel) const override;
		};

		//
		//ArithmeticalBinaryOperator
		//

		class ArithmeticalBinaryOperator : public BinaryOperator {
			NONCOPYABLE(ArithmeticalBinaryOperator);

		protected:
			ArithmeticalBinaryOperator(){}

			gc::Local<Value> evaluate_integer(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				ScriptIntegerType value_a,
				ScriptIntegerType value_b) const override final;

			gc::Local<Value> evaluate_float(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				ScriptFloatType value_a,
				ScriptFloatType value_b) const override final;

			virtual ScriptIntegerType evaluate_integer_integer(
				ScriptIntegerType a,
				ScriptIntegerType b) const = 0;

			virtual ScriptFloatType evaluate_float_float(
				ScriptFloatType a,
				ScriptFloatType b) const = 0;
		};

		//
		//AddBinaryOperator
		//

		class AddBinaryOperator : public ArithmeticalBinaryOperator {
			NONCOPYABLE(AddBinaryOperator);

			AddBinaryOperator(){}

		public:
			static const AddBinaryOperator Instance;

		protected:
			gc::Local<Value> evaluate_by_type(
				const gc::Local<ExecContext>& context,
				const gc::Local<Value>& a,
				const gc::Local<Value>& b,
				OperandType type_a,
				OperandType type_b) const override;

			ScriptIntegerType evaluate_integer_integer(
				ScriptIntegerType a,
				ScriptIntegerType b) const override;

			ScriptFloatType evaluate_float_float(
				ScriptFloatType a,
				ScriptFloatType b) const override;
		};

		//
		//SubBinaryOperator
		//

		class SubBinaryOperator : public ArithmeticalBinaryOperator {
			NONCOPYABLE(SubBinaryOperator);

			SubBinaryOperator(){}

		public:
			static const SubBinaryOperator Instance;

		protected:
			ScriptIntegerType evaluate_integer_integer(
				ScriptIntegerType a,
				ScriptIntegerType b) const override;

			ScriptFloatType evaluate_float_float(
				ScriptFloatType a,
				ScriptFloatType b) const override;
		};

		//
		//MulBinaryOperator
		//

		class MulBinaryOperator : public ArithmeticalBinaryOperator {
			NONCOPYABLE(MulBinaryOperator);

			MulBinaryOperator(){}

		public:
			static const MulBinaryOperator Instance;

		protected:
			ScriptIntegerType evaluate_integer_integer(
				ScriptIntegerType a,
				ScriptIntegerType b) const override;

			ScriptFloatType evaluate_float_float(
				ScriptFloatType a,
				ScriptFloatType b) const override;
		};

		//
		//DivBinaryOperator
		//

		class DivBinaryOperator : public ArithmeticalBinaryOperator {
			NONCOPYABLE(DivBinaryOperator);

			DivBinaryOperator(){}

		public:
			static const DivBinaryOperator Instance;

		protected:
			ScriptIntegerType evaluate_integer_integer(
				ScriptIntegerType a,
				ScriptIntegerType b) const override;

			ScriptFloatType evaluate_float_float(
				ScriptFloatType a,
				ScriptFloatType b) const override;
		};

		//
		//ModBinaryOperator
		//

		class ModBinaryOperator : public ArithmeticalBinaryOperator {
			NONCOPYABLE(ModBinaryOperator);

			ModBinaryOperator(){}

		public:
			static const ModBinaryOperator Instance;

		protected:
			ScriptIntegerType evaluate_integer_integer(
				ScriptIntegerType a,
				ScriptIntegerType b) const override;

			ScriptFloatType evaluate_float_float(
				ScriptFloatType a,
				ScriptFloatType b) const override;
		};

	}
}

#endif//SYNSAMPLE_CORE_OP_H_INCLUDED
