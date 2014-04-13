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

//EBNF extensions template functions implementation.

#ifndef SYN_CORE_EBNF_EXTENSION_IMP_H_INCLUDED
#define SYN_CORE_EBNF_EXTENSION_IMP_H_INCLUDED

#include "conversion.h"
#include "ebnf_extension__def.h"
#include "ebnf_visitor__def.h"

//
//AndExpressionMeaning
//

template<class T>
T synbin::AndExpressionMeaning::visit(AndExpressionMeaningVisitor<T>* visitor) {
	struct AdaptingVisitor : public AndExpressionMeaningVisitor<void> {
		AndExpressionMeaningVisitor<T>* m_effective_visitor;
		T m_value;
	
		void visit_AndExpressionMeaning(AndExpressionMeaning *meaning) override {
			m_value = m_effective_visitor->visit_AndExpressionMeaning(meaning);
		}
		void visit_VoidAndExpressionMeaning(VoidAndExpressionMeaning *meaning) override {
			m_value = m_effective_visitor->visit_VoidAndExpressionMeaning(meaning);
		}
		void visit_ThisAndExpressionMeaning(ThisAndExpressionMeaning *meaning) override {
			m_value = m_effective_visitor->visit_ThisAndExpressionMeaning(meaning);
		}
		void visit_ClassAndExpressionMeaning(ClassAndExpressionMeaning *meaning) override {
			m_value = m_effective_visitor->visit_ClassAndExpressionMeaning(meaning);
		}
	};
	
	AdaptingVisitor adapting_visitor;
	adapting_visitor.m_effective_visitor = visitor;
	visit0(&adapting_visitor);
	
	return adapting_visitor.m_value;
}

#endif//SYN_CORE_EBNF_EXTENSION_IMP_H_INCLUDED
