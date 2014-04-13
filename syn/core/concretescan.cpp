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

//Literal tokens scanner graph: nodes and generator implementation.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

#include "commons.h"
#include "concretescan.h"
#include "util_mptr.h"

namespace ns = synbin;
namespace util = ns::util;

using std::unique_ptr;

using util::MPtr;

//
//ConcreteScanNode
//

const ns::StrTrDescriptor* ns::ConcreteScanNode::get_token() const {
	return m_token;
}

const ns::ConcreteScanNode::EdgeList& ns::ConcreteScanNode::get_edges() const {
	return m_edges;
}

ns::ConcreteScanNode* ns::ConcreteScanNode::add_edge(char ch) {
	m_edges.push_back(ConcreteScanEdge(ch));
	return m_edges.back().get_node_mutable();
}

void ns::ConcreteScanNode::set_token(const ns::StrTrDescriptor* token) {
	assert(token);
	assert(!m_token);
	m_token = token;
}

void ns::ConcreteScanNode::print(std::ostream& out, std::size_t indent) const {
	for (const ConcreteScanEdge& edge : m_edges) edge.print(out, indent);
}

//
//ConcreteScanEdge
//

ns::ConcreteScanEdge::ConcreteScanEdge(char ch)
	: m_ch(ch)
{}

char ns::ConcreteScanEdge::get_ch() const {
	return m_ch;
}

const ns::ConcreteScanNode* ns::ConcreteScanEdge::get_node() const {
	return &m_node;
}

ns::ConcreteScanNode* ns::ConcreteScanEdge::get_node_mutable() {
	return &m_node;
}

void ns::ConcreteScanEdge::print(std::ostream& out, std::size_t indent) const {
	for (std::size_t i = 0; i < indent; ++i) out << "\t";
	out << "'" << m_ch << "'";
	if (m_node.get_token()) out << " : \"" << m_node.get_token()->get_str() << "\"";
	out << '\n';
	m_node.print(out, indent + 1);
}

//
//ConcreteScanGenerator : definition.
//

class ns::ConcreteScanGenerator {
public:
	static unique_ptr<ConcreteScanNode> build_concrete_scan_tree(const std::vector<const StrTrDescriptor*>& tokens);

private:
	static void create_sub_nodes(
		ConcreteScanNode* node,
		const std::vector<const StrTrDescriptor*>& tokens,
		std::size_t start,
		std::size_t end,
		std::size_t str_ofs);
};

//
//ConcreteScanGenerator : implementation
//

namespace {
	bool compare_tokens(const ns::StrTrDescriptor* a, const ns::StrTrDescriptor* b) {
		return a->get_str() < b->get_str();
	}
}//namespace

unique_ptr<ns::ConcreteScanNode> ns::ConcreteScanGenerator::build_concrete_scan_tree(
	const std::vector<const StrTrDescriptor*>& tokens)
{
	//Find out non-name tokens.
	std::vector<const StrTrDescriptor*> s_tokens;
	for (const StrTrDescriptor* token : tokens) {
		if (!token->is_name()) s_tokens.push_back(token);
	}

	//Sort the tokens, so tokens which start with the same characters are together.
	std::sort(s_tokens.begin(), s_tokens.end(), &compare_tokens);

	//Create the root node.
	unique_ptr<ConcreteScanNode> root_node = make_unique1<ConcreteScanNode>();

	//Create sub-nodes.
	create_sub_nodes(root_node.get(), s_tokens, 0, s_tokens.size(), 0);

	return root_node;
}

void ns::ConcreteScanGenerator::create_sub_nodes(
	ConcreteScanNode* node,
	const std::vector<const StrTrDescriptor*>& tokens,
	std::size_t start,
	std::size_t end,
	std::size_t str_ofs)
{
	std::size_t pos = start;
	if (pos < end) {
		const StrTrDescriptor* token = tokens[pos];
		const std::string& s = token->get_str().str();
		if (s.length() == str_ofs) {
			node->set_token(token);
			++pos;
		}
	}

	while (pos < end) {
		std::size_t sub_start = pos;

		const std::string& s = tokens[pos]->get_str().str();
		assert(str_ofs < s.length());
		char c = s[str_ofs];
		++pos;
		while (pos < end) {
			const std::string& p = tokens[pos]->get_str().str();
			if (str_ofs >= p.length() || c != p[str_ofs]) {
				break;
			}
			++pos;
		}

		ConcreteScanNode* sub_node = node->add_edge(c);
		create_sub_nodes(sub_node, tokens, sub_start, pos, str_ofs + 1);
	}
}

//
//build_concrete_scan_tree()
//

unique_ptr<ns::ConcreteScanNode> ns::build_concrete_scan_tree(const std::vector<const StrTrDescriptor*>& tokens) {
	return ConcreteScanGenerator::build_concrete_scan_tree(tokens);
}
