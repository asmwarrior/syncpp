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

//Conversions : declarations.

#ifndef SYN_CORE_CONVERSION_DEC_H_INCLUDED
#define SYN_CORE_CONVERSION_DEC_H_INCLUDED

namespace synbin {
	namespace conv {

		//Context of a simple conversion. Describes in which context a particular syntax expression is used.
		enum SimpleConversionType {
			//Top-level, independent expression.
			SIMPLE_TOP,

			//Dead expression, the produced value is not used.
			SIMPLE_DEAD
		};

		//Hierarchy of ComplexConversionType. Describes the context of a ComplexConversion, i. e. the outer syntax expression
		//of the expression being converted. An expression can be, for instance, a top-level expression (N: E), an attribute
		//expression (N: a=E), etc. The type defines which kind of value is produced by the expression, and how to produce it.
		class ComplexConversionType;
		class TopComplexConversionType;
		class DeadComplexConversionType;
		class AndComplexConversionType;
		class ThisAndComplexConversionType;
		class AttrAndComplexConversionType;
		class PartClassAndComplexConversionType;
		class ClassAndComplexConversionType;

		//Hierarchy of Conversion. Any SyntaxExpression has an associated Conversion. The conversion describes how to convert
		//that SyntaxExpression into a BNF symbol.
		class Conversion;

		class EmptyConversion;
		class ConstConversion;
		class CastConversion;
		class ThisConversion;

		class SimpleConversion;
		class NameConversion;
		class StringConversion;
		class LoopConversion;
		class ZeroManyConversion;
		class OneManyConversion;

		class ComplexConversion;
		class OrConversion;
		class ZeroOneConversion;

		class AttributeConversion;
		class TopAttributeConversion;
		class AttrAndAttributeConversion;
		class PartClassAndAttributeConversion;
		class ClassAndAttributeConversion;

		class AndConversion;
		class VoidAndConversion;
		class ThisAndConversion;
		class AttributeAndConversion;
		class AbstractClassAndConversion;
		class PartClassAndConversion;
		class ClassAndConversion;

	}
}

#endif//SYN_CORE_CONVERSION_DEC_H_INCLUDED
