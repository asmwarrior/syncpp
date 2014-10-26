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

//Sample HTTP Server.
//-------------------------------------------------------------------------------------------------

//HTTP request size limit. Used to prevent out-of-memory errors in case of very long requests.
//Requests sent by Firefox, IE, Opera and Safari seem to be 300-400 bytes long.
const REQUEST_SIZE_LIMIT = 5000;

//Command line.
const COMMAND_LINE = parse_command_line();

//Paths.
const WEB_DIR = COMMAND_LINE.dir;
const ROOT_DIR = new sys.File(WEB_DIR, "root");
const LIB_DIR = new sys.File(WEB_DIR, "lib");
const LOG_FILE = new sys.File(WEB_DIR, "server.log");
const REQUEST_LOG_FILE = new sys.File(WEB_DIR, "request.log");

///////////////////////////////////////////////////////////////////////////////////////////////////
//High-level request processing functions.
///////////////////////////////////////////////////////////////////////////////////////////////////

//Writes the contents of the specified file to the response.
function handle_file_plain(req, resp, file, content_type) {
	resp.set_content_type(content_type);
	var data = file.read_bytes();
	resp.get_out().write_bytes(data);
}

//Creates an object which will be used as 'http' namespace in a dynamic page.
function create_http_namespace(req1, resp1) {
	return new (class {
		public const context = g_context;
		public const req = req1;
		public const resp = resp1;

		function session() { return req1.get_session(); }
		function session_opt() { return req1.get_session_opt(); }
	})();
}

//Creates a 'server' namespace object for dynamic pages.
function create_server_namespace() {
	return new (class {
		public const root_dir = ROOT_DIR;
		function stop() { g_server_stopping = true; }
	})();
}

//Handles a dynamic page file.
function handle_file_s(req, resp, file) {
	function add_source_file(sources, f) {
		var path = f.get_path();
		var text = f.read_text();
		sources.add([ path, text ]);
	}

	var scope = new sys.HashMap();
	scope.put("sys", sys);
	scope.put("http", create_http_namespace(req, resp));
	scope.put("server", create_server_namespace());

	var sources = new sys.ArrayList();
	if (LIB_DIR.is_directory()) {
		for (var lib_file : LIB_DIR.list_files()) {
			if (ends_with(lib_file.get_name(), ".s") && lib_file.is_file()) add_source_file(sources, lib_file);
		}
	}
	add_source_file(sources, file);

	sys.execute_ex(sources.to_array(), scope);
}

//Handles a file of an unsupported type.
function handle_file_unknown(req, resp, file) {
	throw "Resource not found (code 2): " + file.get_absolute_path();
}

//Handles an arbitrary server file.
function handle_file(req, resp, file) {
	const plain_files = [
		[".html", "text/html"],
		[".css", "text/css"],
		[".png", "image/png"]
	];

	var file_name = file.get_name();
	if (ends_with(file_name, ".s")) {
		handle_file_s(req, resp, file);
		return;
	}

	for (var file_type : plain_files) {
		var suffix = file_type[0];
		var content_type = file_type[1];
		if (ends_with(file_name, suffix)) {
			handle_file_plain(req, resp, file, content_type);
			return;
		}
	}

	handle_file_unknown(req, resp, file);
}

//Finds an existing file for the specified file. If the specified file does not exist, a similar file is
//searched, e. g. by adding '.html' extension.
function find_valid_file(file) {
	if (file.is_file()) return file;
	if (file.is_directory()) {
		for (var sub_name : [ "index.html", "index.s" ]) {
			var sub_file = new sys.File(file, sub_name);
			if (sub_file.is_file()) return sub_file;
		}
		return null;
	}

	var dir = file.get_parent_file();
	if (dir != null && dir.is_directory()) {
		var file_name = file.get_name();
		for (var suffix : [ ".html", ".s" ]) {
			var sub_file = new sys.File(dir, file_name + suffix);
			if (sub_file.is_file()) return sub_file;
		}
	}

	return null;
}

//Handles an HTTP request.
function handle_request(req, resp) {
	//Splits "/"-separated path into elements.
	function split_path(path) {
		var list = new sys.ArrayList();
		const len = path.length();
		var ofs = 0;

		while (ofs < len) {
			var prev_ofs = ofs;
			ofs = path.index_of('/', ofs);
			var next_ofs;
			if (ofs == -1) {
				ofs = len;
				next_ofs = len;
			} else {
				next_ofs = ofs + 1;
			}

			if (ofs > prev_ofs) list.add(path.substring(prev_ofs, ofs));
			ofs = next_ofs;
		}

		return list.to_array();
	}

	//Searches for the mapping defined for the specified name.
	function get_mapped_name(mapping_file, name) {
		if (!mapping_file.exists()) return null;

		var text = mapping_file.read_text();
		var lines = text.get_lines();
		for (var line : lines) {
			var pos = line.index_of(' ');
			if (pos != -1) {
				var left = line.substring(0, pos);
				if (left == name) return line.substring(pos + 1);
			}
		}
		return null;
	}

	//Returns the file which the specified name is mapped to, or the file with that name,
	//if no mapping is defined.
	function get_mapped_file(dir, name) {
		if (name == "." || name == "..") throw "Invalid URL path";
	
		var mapping_file = new sys.File(dir, "mapping.txt");
		if (mapping_file != null) {
			var mapped_name = get_mapped_name(mapping_file, name);
			if (mapped_name != null) name = mapped_name;
		}
		return new sys.File(dir, name);
	}

	//Searches a file by a URL path.
	function get_file_by_path(path) {
		var path_parts = split_path(path);
		var file = ROOT_DIR;
		for (var name : path_parts) file = get_mapped_file(file, name);
		return file;
	}

	;;;

	var file = get_file_by_path(req.get_path());
	file = find_valid_file(file);
	if (file == null || !file.is_file()) throw "Resource not found: " + req.get_path();
	
	handle_file(req, resp, file);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//Context, session, request and response classes.
///////////////////////////////////////////////////////////////////////////////////////////////////

const g_context = new HttpContext();
var g_session_id_sequence = 0;
const g_sessions_map = new sys.HashMap();

//Context.
class HttpContext {
	const m_attributes = new sys.HashMap();

	function get_attribute(name) { return m_attributes.get(name); }
	function set_attribute(name, value) { m_attributes.put(name, value); }
}

//Global session object.
class HttpSessionGlobal {
	var m_id;
	const m_attributes = new sys.HashMap();

	new(id) { m_id = id; }
	function get_id() { return m_id; }
	function get_attribute(name) { return m_attributes.get(name); }
	function set_attribute(name, value) { m_attributes.put(name, value); }
}

//Local session object. Associated with a particular request and response pair.
class HttpSession {
	var m_session_global;
	var m_request;
	var m_response;

	new(session_global, request, response) {
		m_session_global = session_global;
		m_request = request;
		m_response = response;
	}

	function get_id() { return m_session_global.get_id(); }

	function close() {
		m_response.remove_cookie("SessionID");
		g_sessions_map.remove(get_id());
	}

	function get_attribute(name) { return m_session_global.get_attribute(name); }
	function set_attribute(name, value) { m_session_global.set_attribute(name, value); }
}

//Request.
class HttpRequest {
	var m_method;
	var m_url;
	var m_http_version;
	var m_headers;
	var m_path;
	var m_parameters;
	var m_cookies;
	var m_session;
	var m_response;
	var m_remote_host;
	var m_remote_port;

	new(socket, method, url, http_version, headers, path, parameters, cookies) {
		m_method = method;
		m_url = url;
		m_http_version = http_version;
		m_headers = headers;
		m_path = path;
		m_parameters = parameters;
		m_cookies = cookies;
		m_session = null;
		m_remote_host = socket.get_remote_host();
		m_remote_port = socket.get_remote_port();
		m_response = new HttpResponse(socket);
	}

	function get_session() {
		var s = get_session_opt();
		if (s != null) return s;

		var session_id = "" + (g_session_id_sequence++);
		var session_global = new HttpSessionGlobal(session_id);
		g_sessions_map.put(session_id, session_global);
		m_session = new HttpSession(session_global, this, m_response);
		m_response.set_cookie("SessionID", session_id);

		return m_session;
	}

	function get_session_opt() {
		if (m_session == null) {
			var cookie = m_cookies.get("SessionID");
			if (cookie != null) {
				var session_global = g_sessions_map.get(cookie);
				if (session_global != null) m_session = new HttpSession(session_global, this, m_response);
			}
		}
		return m_session;
	}

	function get_method() { return m_method; }
	function get_url() { return m_url; }
	function get_path() { return m_path; }
	function get_parameters() { return m_parameters.keys(); }
	function get_parameter(name) { return m_parameters.get(name); }
	function get_remote_host() { return m_remote_host; }
	function get_remote_port() { return m_remote_port; }
	function get_response() { return m_response; }
	function get_cookie(name) { return m_cookies.get(name); }
}

//Response.
class HttpResponse {
	var m_socket;
	var m_status_code;
	var m_status_message;
	var m_content_type;
	var m_headers;
	var m_cookies;
	var m_out;

	new(socket) {
		m_socket = socket;
		m_status_code = 200;
		m_status_message = "OK";
		m_headers = new sys.HashMap();
		m_cookies = new sys.HashMap();
		m_content_type = "text/html";
		m_out = null;

		add_header("Server", "TinyServer/1.0");
	}
	
	function is_committed() {
		return m_out != null;
	}
	
	private function check_not_committed() {
		if (is_committed()) throw "The response has already been committed!";
	}

	function set_status(code, message) {
		check_not_committed();
		m_status_code = code;
		m_status_message = message;
	}
	
	function set_content_type(content_type) {
		check_not_committed();
		m_content_type = content_type;
	}

	function add_header(header, value) {
		check_not_committed();
		add_to_list_map(m_headers, header, value);
	}

	function set_cookie(name, value) {
		check_not_committed();
		m_cookies.put(name, value);
	}

	function remove_cookie(name) {
		check_not_committed();
		m_cookies.put(name, "; expires=Thu, 01 Jan 1970 00:00:00 GMT");
	}

	function set_body(body) {
		var out = get_out();
		out.write(body);
	}
	
	private function send_header(socket, buf, name, value) {
		buf.clear();
		buf.append(name);
		buf.append_char(':');
		buf.append_char(' ');
		buf.append(value);
		write_line(socket, buf.to_string());
	}

	//Writes an HTTP response header into the socket.
	private function commit() {
		var buf = new sys.StringBuffer();
		buf.append("HTTP/1.1 ");
		buf.append("" + m_status_code);
		buf.append_char(' ');
		buf.append(m_status_message);
		write_line(m_socket, buf.to_string());

		send_header(m_socket, buf, "Content-Type", m_content_type);
		
		for (var header : m_headers.keys()) {
			for (var value : m_headers.get(header)) send_header(m_socket, buf, header, value);
		}

		for (var cookie : m_cookies.keys()) {
			var value = m_cookies.get(cookie);
			send_header(m_socket, buf, "Set-Cookie", cookie + "=" + value + "; Path=/");
		}

		write_line(m_socket, "");
	}
	
	//Commits headers and returns an output stream to write the response body to.
	function get_out() {
		if (m_out == null) {
			commit();
			m_out = new (class {
				function write(str) {
					m_socket.write(str.get_bytes());
				}
				function write_bytes(bytes) {
					m_socket.write(bytes);
				}
			})();
		}
		return m_out;
	}
	
	function close() {
		get_out(); //This will call commit() if it has not been called yet.
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//Low-level request processing functions.
///////////////////////////////////////////////////////////////////////////////////////////////////

//Reads a single HTTP request header from a socket, and adds it into a map.
function parse_request_header(line, headers) {
	const len = line.length();
	var pos = line.index_of(':');
	if (pos == -1) throw "Invalid HTTP request header (no colon)";
	if (pos == 0) throw "Invalid HTTP request header (header name is empty)";
	var header = line.substring(0, pos);
	
	++pos;
	if (pos < len && line[pos] == ' ') ++pos;
	var value = line.substring(pos);
	
	add_to_list_map(headers, header, value);
}

//Parses a URL string. Returns path and parameters substrings.
function parse_url(url) {
	function parse_parameter(i, j, params) {
		var s = url.substring(i, j);
		var k = s.index_of('=');
		if (k == -1) throw "Invalid URL parameter";
		var name = s.substring(0, k);
		var value = decode_parameter(s.substring(k + 1));
		var old_value = params.put(name, value);
		if (old_value != null) throw "Duplicated URL parameter: " + name;
	}
	
	;;;

	var params = new sys.HashMap();

	var i = url.index_of('?');
	if (i == -1) return [ url, params ];

	var path = url.substring(0, i);
	const len = url.length();
	++i;

	for (;;) {
		var j = url.index_of('&', i);
		if (j == -1) {
			parse_parameter(i, len, params);
			break;
		} else {
			parse_parameter(i, j, params);
			i = j + 1;
		}
	}

	return [ path, params ];
}

//Returns a map of cookies.
function parse_cookies(headers) {
	function parse_cookie_single(cookies, str, ofs) {
		const len = str.length();
		
		const name_ofs = ofs;
		while (ofs < len && str[ofs] != '=') ++ofs;
		if (ofs == len || ofs == name_ofs) throw "Invalid cookie string";
		var name = str.substring(name_ofs, ofs);
		++ofs;

		const value_ofs = ofs;
		while (ofs < len && str[ofs] != ';' && str[ofs] != ' ') ++ofs;
		var value = str.substring(value_ofs, ofs);

		if (ofs < len) ++ofs;
		while (ofs < len && str[ofs] == ' ') ++ofs;

		var old_value = cookies.put(name, value);
		if (old_value != null) throw "Duplicated cookie: " + name;

		return ofs;
	}

	function parse_cookie(cookies, str) {
		const len = str.length();
		var ofs = 0;
		while (ofs < len) ofs = parse_cookie_single(cookies, str, ofs);
	}
	
	;;;

	var cookies = new sys.HashMap();
	var values = headers.get("Cookie");
	if (values != null) {
		for (var value : values) parse_cookie(cookies, value);
	}
	return cookies;
}

//Reads HTTP request from a socket and returns an HttpRequest object.
function read_request(socket) {
	var lines = read_request_lines(socket);
	var line_cnt = lines.length;
	if (line_cnt == 0) throw "Empty HTTP request";

	var top_words = split_words(lines[0]);
	if (top_words.length != 3) throw "Invalid HTTP request line: " + top_words;

	var method = top_words[0];
	var url = top_words[1];
	var http_version = top_words[2];

	var headers = new sys.HashMap();
	for (var i = 1; i < line_cnt; ++i) parse_request_header(lines[i], headers);

	var cookies = parse_cookies(headers);

	var url_parts = parse_url(url);
	var path = url_parts[0];
	var parameters = url_parts[1];

	return new HttpRequest(socket, method, url, http_version, headers, path, parameters, cookies);
}

//Handles a server error.
function handle_error(socket, e) {
	var msg = "" + e;

	var lines = new sys.ArrayList();
	lines.add("<html>");
	lines.add("<head><title>Error</title></head>");
	lines.add("<body>");
	lines.add("</body>");
	lines.add("<h2>Error</h2>");
	lines.add(msg);
	lines.add("</html>");
	var body = lines_to_string(lines);

	var resp = new HttpResponse(socket);
	resp.set_status(400, "Bad request");
	resp.set_content_type("text/html");
	resp.set_body(body);
	resp.close();

	return resp;
}

//Handles a single request.
function process_connection(socket) {
	//Read and parse the request. If an error occurs, the connection will be closed.
	var req;
	var resp;
	try {
		req = read_request(socket);
		log_request(socket, req);
		resp = req.get_response();
	} catch (e) {
		log_error(e);
		return;
	}

	//Handle the request. An HTTP response will be sent if an error occurs.
	try {
		handle_request(req, resp);
		resp.close();
	} catch (e) {
		log_error(e);
		if (!resp.is_committed()) handle_error(socket, e);
	}
}

var g_server_stopping = false;

//Accepts connections and processes requests.
function listen() {
	const port = COMMAND_LINE.port;
	var ss = new sys.ServerSocket(port);
	try {
		log("Listening on port " + port);
		while (!g_server_stopping) {
			var s = ss.accept();
			try {
				process_connection(s);
			} catch (e) {
				log_error(e);
			} finally {
				s.close();
			}
		}
	} finally {
		ss.close();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//Utility functions.
///////////////////////////////////////////////////////////////////////////////////////////////////

//Parses command line arguments, returns a command line object.
function parse_command_line() {
	if (sys.args.length < 1) throw "Port and web directory path must be specified in the command line";
	if (sys.args.length > 2) throw "Too many command line arguments";
	
	var port0;
	try {
		port0 = sys.str_to_int(sys.args[0]);
	} catch (e) {
		throw "Invalid port number: " + e;
	}
	
	var path = sys.args[1];
	var dir0 = new sys.File(path);
	if (!dir0.is_directory()) throw "Web directory not found: " + path;
	dir0 = dir0.get_absolute_file();
	
	return new (class {
		public const port = port0;
		public const dir = dir0;
	})();
}

//Reads HTTP request header lines.
function read_request_lines(socket) {
	var list = new sys.ArrayList();
	var buf = new sys.StringBuffer();
	var size = 0;
	
	for (;;) {
		var k = socket.read_byte();
		if (k == -1 || k == '\n') {
			if (buf.length() == 0) break;
			
			var str = buf.to_string();
			var len = str.length();
			if (len > REQUEST_SIZE_LIMIT || REQUEST_SIZE_LIMIT - len < size) {
				throw "Request header is too long";
			}
			
			list.add(str);
			size += len;
			
			if (k == -1) break;
			buf.clear();
		} else if (k != '\r') {
			buf.append_char(k);
		}
	}
	
	log_request_lines(socket, list);

	return list.to_array();
}

//Logs the HTTP request header into the %REQUEST_LOG_FILE%.
function log_request_lines(socket, lines) {
	var out = REQUEST_LOG_FILE.text_out(true);
	try {
		out.print(sys.current_time_str());
		out.print(" ");
		out.print(socket.get_remote_host());
		out.print(":");
		out.println(socket.get_remote_port());
		for (var line : lines) out.println(line);
		out.println();
	} finally {
		out.close();
	}
}

//Reads a sequence of words from a socket.
function split_words(line) {
	var words = new sys.ArrayList();
	
	var pos = 0;
	const len = line.length();
	for (;;) {
		var next_pos = line.index_of(' ', pos);
		if (next_pos == -1) {
			words.add(line.substring(pos));
			break;
		}
		
		words.add(line.substring(pos, next_pos));
		pos = next_pos + 1;
	}
	
	return words.to_array();
}

//Decodes a URL parameter.
function decode_parameter(v) {
	function char_to_hex_digit(c) {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'A' && c <= 'F') return 10 + c - 'A';
		if (c >= 'a' && c <= 'f') return 10 + c - 'A';
		throw "Invalid parameter";
	}

	var buf = new sys.StringBuffer();
	const len = v.length();
	var ofs = 0;
	while (ofs < len) {
		var c = v[ofs++];
		if (c == '%') {
			if (ofs + 1 >= len) throw "Invalid parameter";
			c = char_to_hex_digit(v[ofs]) * 16 + char_to_hex_digit(v[ofs + 1]);
			ofs += 2;
		} else if (c == '+') {
			c = ' ';
		}
		buf.append_char(c);
	}
	return buf.to_string();
}

//Writes the given string into the socket, adding CR, LF in the end.
function write_line(socket, s) {
	socket.write(s.get_bytes());
	socket.write_byte('\r');
	socket.write_byte('\n');
}

//Combines a list of strings into a single string using LF as a separator.
function lines_to_string(lines) {
	var buf = new sys.StringBuffer();
	for (var line : lines) {
		buf.append(line);
		buf.append_char('\n');
	}
	var str = buf.to_string();
	return str;
}

//Logs a single HTTP request.
function log_request(socket, req) {
	var buf = new sys.StringBuffer();
	buf.append(socket.get_remote_host());
	buf.append_char(':');
	buf.append(socket.get_remote_port());
	buf.append_char(' ');
	buf.append(req.get_method());
	buf.append_char(' ');
	buf.append(req.get_url());
	log(buf);
}

//Logs a message.
function log(str) {
	var msg = sys.current_time_str() + " " + str;
	log_to_stream((out) {
		out.println(msg);
	});
}

//Logs an exception.
function log_error(e) {
	var msg = sys.current_time_str() + " ";
	log_to_stream((out) {
		out.print(msg);
		e.print(out);
	});
}

//Logs to both the STDOUT and the log file.
function log_to_stream(callback) {
	callback(sys.out);

	var out = LOG_FILE.text_out(true);
	try {
		callback(out);
	} finally {
		out.close();
	}
}

//Checks if a string ends with the given suffix.
function ends_with(s, suffix) {
	const len = s.length();
	const suflen = suffix.length();
	if (suflen > len) return false;
	var ofs = len - suflen;
	for (var i = 0; i < suflen; ++i) if (s[ofs + i] != suffix[i]) return false;
	return true;
}

//Adds a value into a map of lists.
function add_to_list_map(map, key, value) {
	var list = map.get(key);
	if (list == null) {
		list = new sys.ArrayList();
		map.put(key, list);
	}
	list.add(value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//Entry point.
///////////////////////////////////////////////////////////////////////////////////////////////////

listen();
