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

//Literal tokens scanner graph classes definition.

#ifndef SYN_CORE_CONCRETESCAN_H_INCLUDED
#define SYN_CORE_CONCRETESCAN_H_INCLUDED

#include <list>
#include <memory>
#include <ostream>

#include "descriptor.h"
#include "util_mptr.h"

namespace synbin {

	class ConcreteScanGenerator;
	class ConcreteScanEdge;

	//
	//ConcreteScanNode
	//

	//Literal tokens scanner state graph node.
	class ConcreteScanNode {
		friend class ConcreteScanGenerator;

	public:
		typedef std::list<ConcreteScanEdge> EdgeList;

	private:
		//Pointer to the token accepted in this state, or nullptr, if the state does not accept any tokens.
		const StrTrDescriptor* m_token;

		//List of edges.
		EdgeList m_edges;

	public:
		ConcreteScanNode() : m_token(){}

		const StrTrDescriptor* get_token() const;
		const EdgeList& get_edges() const;

		void print(std::ostream& out, std::size_t indent) const;

	private:
		ConcreteScanNode* add_edge(char ch);
		void set_token(const StrTrDescriptor* token);
	};

	//
	//ConcreteScanEdge
	//

	//Literal tokens scanner state graph edge.
	class ConcreteScanEdge {
		friend class ConcreteScanNode;

		//The character associated with the edge.
		const char m_ch;

		//The destination state node.
		ConcreteScanNode m_node;

	public:
		ConcreteScanEdge(char ch);

		char get_ch() const;
		const ConcreteScanNode* get_node() const;

		void print(std::ostream& out, std::size_t indent) const;

	private:
		ConcreteScanNode* get_node_mutable();
	};

	//
	//build_concrete_scan_tree()
	//

	std::unique_ptr<ConcreteScanNode> build_concrete_scan_tree(const std::vector<const StrTrDescriptor*>& tokens);

}

#endif//SYN_CORE_CONCRETESCAN_H_INCLUDED
