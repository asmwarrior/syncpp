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

//Definition of descriptors associated with BNF grammar symbols.

#ifndef SYN_CORE_DESCRIPTOR_H_INCLUDED
#define SYN_CORE_DESCRIPTOR_H_INCLUDED

#include <ostream>

#include "action__dec.h"
#include "descriptor_type.h"
#include "noncopyable.h"
#include "util_string.h"

namespace synbin {

	class AbstractDescriptor;
	class SymDescriptor;
	class TrDescriptor;
	class NameTrDescriptor;
	class StrTrDescriptor;
	class NtDescriptor;
	class UserNtDescriptor;
	class AutoNtDescriptor;
	class PrDescriptor;

}

//
//AbstractDescriptor
//

//Base class for BNF object descriptors.
class synbin::AbstractDescriptor {
	NONCOPYABLE(AbstractDescriptor);

protected:
	AbstractDescriptor(){}

public:
	virtual ~AbstractDescriptor(){}

	//Returns the type produced by the correspondnig symbol.
	virtual util::MPtr<const TypeDescriptor> get_type() const = 0;
};

//
//SymDescriptor
//

//BNF symbol descriptor - terminal or nonterminal.
class synbin::SymDescriptor : public AbstractDescriptor {
	NONCOPYABLE(SymDescriptor);

	const util::MPtr<const TypeDescriptor> m_type;

protected:
	SymDescriptor(util::MPtr<const TypeDescriptor> type);

public:
	util::MPtr<const TypeDescriptor> get_type() const override;

	virtual void print(std::ostream& out) const = 0;
};

//
//TrDescriptor
//

//Terminal BNF symbol descriptor.
class synbin::TrDescriptor : public SymDescriptor {
	NONCOPYABLE(TrDescriptor);

protected:
	TrDescriptor(util::MPtr<const TypeDescriptor> type);

public:
	virtual void generate_constant_name(std::ostream& out) const = 0;
	virtual void generate_constant_comment(std::ostream& out) const = 0;
	virtual void generate_token_str(std::ostream& out) const = 0;
};

//
//NameTrDescriptor
//

//Descriptor of a BNF terminal symbol defined by name.
class synbin::NameTrDescriptor : public TrDescriptor {
	NONCOPYABLE(NameTrDescriptor);

	const util::String m_name;

public:
	NameTrDescriptor(util::MPtr<const TypeDescriptor> type, const util::String& name);

	const util::String& get_name() const;

	void generate_constant_name(std::ostream& out) const override;
	void generate_constant_comment(std::ostream& out) const override;
	void generate_token_str(std::ostream& out) const override;

	void print(std::ostream& out) const override;
};

//
//StrTrDescriptor
//

//Descriptor of a BNF terminal symbol defined by a literal.
class synbin::StrTrDescriptor : public TrDescriptor {
	NONCOPYABLE(StrTrDescriptor);

	const util::String m_str;
	const std::size_t m_id;
	const bool m_is_name;

public:
	StrTrDescriptor(util::MPtr<const TypeDescriptor> type, const util::String& str, std::size_t id, bool is_name);

	const util::String& get_str() const;
	bool is_name() const;

	void generate_constant_name(std::ostream& out) const override;
	void generate_constant_comment(std::ostream& out) const override;
	void generate_token_str(std::ostream& out) const override;

	void print(std::ostream& out) const override;
};

//
//NtDescriptor
//

//Nonterminal symbol descriptor.
class synbin::NtDescriptor : public SymDescriptor {
	NONCOPYABLE(NtDescriptor);

	const util::String m_bnf_name;

protected:
	NtDescriptor(util::MPtr<const TypeDescriptor> type, const util::String& bnf_name);

public:
	const util::String& get_bnf_name() const;

	virtual const UserNtDescriptor* as_user_nt() const = 0;
};

//
//UserNtDescriptor
//

//Descriptor of a BNF nonterminal which corresponds to a EBNF nonterminal.
class synbin::UserNtDescriptor : public NtDescriptor {
	NONCOPYABLE(UserNtDescriptor);

	const util::String m_name;

public:
	UserNtDescriptor(util::MPtr<const TypeDescriptor> type, const util::String& bnf_name, const util::String& name);

	const util::String& get_name() const;
	const UserNtDescriptor* as_user_nt() const override;

	void print(std::ostream& out) const override;
};

//
//AutoNtDescriptor
//

//Descriptor of an auto-generated BNF nonterminal symbol, which does not correspond to an EBNF nonterminal.
class synbin::AutoNtDescriptor : public NtDescriptor {
	NONCOPYABLE(AutoNtDescriptor);

public:
	AutoNtDescriptor(util::MPtr<const TypeDescriptor> type, const util::String& bnf_name);

	const UserNtDescriptor* as_user_nt() const override;

	void print(std::ostream& out) const override;
};

//
//PrDescriptor
//

//Descriptor of a BNF production.
class synbin::PrDescriptor : public AbstractDescriptor {
	NONCOPYABLE(PrDescriptor);

	const util::MPtr<const Action> m_action;

public:
	PrDescriptor(util::MPtr<const Action> action);

	util::MPtr<const TypeDescriptor> get_type() const override;
	util::MPtr<const Action> get_action() const;
};

#endif//SYN_CORE_DESCRIPTOR_H_INCLUDED
