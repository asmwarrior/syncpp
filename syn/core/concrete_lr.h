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

//Definition of concrete LR tables representation types (based on the LRTables template).

#ifndef SYN_CORE_CONCRETE_LR_H_INCLUDED
#define SYN_CORE_CONCRETE_LR_H_INCLUDED

#include "concrete_bnf.h"
#include "lrtables.h"

namespace synbin {

	typedef LRTables<ConcreteBNFTraits> ConcreteLRTables;
	
	typedef ConcreteLRTables::Sym ConcreteLRSym;
	typedef ConcreteLRTables::Tr ConcreteLRTr;
	typedef ConcreteLRTables::Nt ConcreteLRNt;
	typedef ConcreteLRTables::Pr ConcreteLRPr;
	typedef ConcreteLRTables::State ConcreteLRState;
	typedef ConcreteLRTables::Shift ConcreteLRShift;
	typedef ConcreteLRTables::Goto ConcreteLRGoto;

}

#endif//SYN_CORE_CONCRETE_LR_H_INCLUDED
