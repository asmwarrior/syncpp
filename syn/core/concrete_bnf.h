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

//Definition of a concrete BNF grammar representation type (based on the BnfGrammar template class).

#ifndef SYN_CORE_CONCRETE_BNF_H_INCLUDED
#define SYN_CORE_CONCRETE_BNF_H_INCLUDED

#include "bnf.h"
#include "descriptor.h"
#include "descriptor_type.h"
#include "util_mptr.h"

namespace synbin {

	class ConcreteBNFTraits : public BnfTraits<
		util::MPtr<const NtDescriptor>,
		util::MPtr<const TrDescriptor>,
		util::MPtr<const PrDescriptor>
	>
	{};

	typedef BnfGrammar<ConcreteBNFTraits> ConcreteBNF;

}

#endif//SYN_CORE_CONCRETE_BNF_H_INCLUDED
