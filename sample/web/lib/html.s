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

//HTML Generation Library.

//HTML Document. Allows to generate HTML conveniently.
class HTML_Document {
	var m_out;
	const m_tags = new sys.ArrayList();
	const m_buffer = new sys.StringBuffer();

	new(resp) {
		m_out = resp.get_out();
	}
	
	private function flush_buffer() {
		m_out.write(m_buffer.to_string());
		m_buffer.clear();
	}
	
	private function check_buffer_overflow() {
		if (m_buffer.length() >= 4096) flush_buffer();
	}
	
	private function extract_tag(str) {
		var pos = str.index_of(' ');
		return pos == -1 ? str : str.substring(0, pos);
	}
	
	private function tag0(str, closing_bracket) {
		m_buffer.append("<");
		m_buffer.append(str);
		m_buffer.append(closing_bracket);
		check_buffer_overflow();
	}
	
	//Writes a start tag of an element to the response. Adds the tag to the list of tags, so calling
	//end() will write the corresponding end tag.
	function tag(str) {
		tag0(str, ">");
		m_tags.add(extract_tag(str));
	}
	
	//Write an empty element (ending with '/>').
	function tag_z(str) {
		tag0(str, "/>");
	}
	
	//Writes the end tag (which corresponds to the last non-empty element) to the response.
	function end() {
		var size = m_tags.size();
		if (size > 0) {
			var name = m_tags.get(size - 1);
			m_tags.remove(size - 1);
			
			m_buffer.append("</");
			m_buffer.append(name);
			m_buffer.append(">");
			check_buffer_overflow();
		}
	}
	
	//Writes a plain text to the response.
	function text(str) {
		m_buffer.append(str);
		check_buffer_overflow();
	}
	
	//Writes data to the response. The argument may be:
	//- Array, denoting an element. The first element of the array is the tag (and attributes, optionally),
	//  rest of the elements are processed by this function recursively, and written to the response as
	//  nested elements of this element.
	//- String. Plain text.
	//- Function. The function is invoked. Must take no arguments.
	function data(d) {
		const type = typeof(d);
		if ("array" == type) {
			const len = d.length;
			if (len == 1) {
				tag_z(d[0]);
			} else if (len > 1) {
				tag(d[0]);
				for (var i = 1; i < len; ++i) data(d[i]);
				end();
			}
		} else if ("string" == type) {
			text(d);
		} else if ("function" == type) {
			d();
		} else {
			throw "Invalid data type: " + type;
		}
	}
	
	//Closes this document, writting all necessary end tags to the response.
	function close() {
		while (!m_tags.is_empty()) end();
		if (!m_buffer.is_empty()) flush_buffer();
	}
}

//Encodes a string, replacing HTML control characters by escape sequences.
function html_encode(str) {
	var buf = new sys.StringBuffer();
	for (var i = 0, n = str.length(); i < n; ++i) {
		var c = str[i];
		if (c == '&') {
			buf.append("&amp;");
		} else if (c == '\'') {
			buf.append("&apos;");
		} else if (c == '"') {
			buf.append("&quot;");
		} else if (c == '<') {
			buf.append("&lt;");
		} else if (c == '>') {
			buf.append("&gt;");
		} else if (c < 0x20 || c >= 0x80) { 
			buf.append("?"); //TODO Correct character code.
		} else {
			buf.append_char(c);
		}
	}
	return buf.to_string();
}
