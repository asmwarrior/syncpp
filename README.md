Syn
===

## Overview

A parser generator for context-free grammars, written in C++. Accepts a grammar in an EBNF notation, generates parser code in C++. The parser produces an AST at runtime.

Features:
* Grammar is written in an EBNF notation.
* Generation of AST with attributes.
* Mapping AST nodes to objects of arbitrary C++ classes.
* Uses GLR parsing algorithm.
* Supports Windows and Linux.

## Getting Started

Consider a fragment of a grammar:
```Java
@Statement : IfStatement | OtherStatement ;
IfStatement : "if" "(" expr=Expression ")" tStmt=Statement ("else" fStmt=Statement)? ;
```

And C++ classes:
```C++
class Statement {
public:
    Statement(){}
    virtual void execute() = 0;
};

class IfStatement : public Statement {
public:
    std::shared_ptr<Expression> m_expr;
    std::shared_ptr<Statement> m_tStmt;
    std::shared_ptr<Statement> m_fStmt;
    
    IfStatement(){}

    void execute() override {
        if (m_expr->evaluate()) {
            m_tStmt->execute();
        } else if (!!m_fStmt) {
            m_fStmt->execute();
        }
    }   
};
```

Syn generates a parser that converts a text into corresponding C++ objects:
```C++
Scanner scanner(text);
std::shared_ptr<Statement> statement = syngen::SynParser::parse_Statement(scanner);
statement->execute();
```

## Sample: a Simple Scripting Language

A simple Javascript-like language interpreter.

Features:

* Garbage collection.
* Dynamic type-checking and binding.
* Lambda expressions.
* File and socket API.
* Supports Windows and Linux.

See the [Syn grammar of the language](https://github.com/antkar/syncpp/blob/master/sample/core/grammar.txt).

### Code Examples

Try-catch-finally block:

```JavaScript
var in = open_file();
try {
    in.read();
} catch (e) {
    sys.out.println("Failed!");
    e.print(sys.out);   // Prints the stack trace.
} finally {
    in.close();
}
```

Classes, constants, access modifiers:

```JavaScript
class Foo {
    // Constants and variables are private by default.
    public const count = get_count();
    
    var name;
    var misc = 0;
    
    new(str) { // Constructor.
        name = str;
        misc = 12.34;
    }
    
    // Functions are public by default.
    function bar() {
        return name + "," + misc;
    }
    
    private function get_count() {
        return 123;
    }
}
```

Lambda expressions:

```JavaScript
function repeat(n, fn) {
    for (var i = 0; i < n; ++i) fn(i);
}

repeat(5, (x){
    sys.out.println("x=" + x);
});
```

Anonymous classes:

```JavaScript
function make_class(prop) {
    return class {
        var tag;
    
        new(z) { // Constructor.
            tag = z;
        }
        
        function get_tag() { return tag; }
        function get_prop() { return prop; }
    };
}

var cls = make_class(123);
var obj = new cls(456);
sys.out.println(obj.get_prop() + " " + obj.get_tag());
```

Typeof expression:

```JavaScript
function print(value) {
    if ("array" == typeof(value)) {
        for (var e : value) print(e);
    } else {
        sys.out.println(value);
    }
}
```

String class:

```JavaScript
const str = "Hello World";
print(str.length());
print(str.substring(0, 5));
print(str.index_of('w'));
print(str.char_at(7));
```

StringBuffer class:

```JavaScript
var buf = new sys.StringBuffer();
buf.append("Hello ");
buf.append("World");
print(buf.to_string());   // Prints "Hello World"
buf.clear();
buf.append("Foo");
print(buf.to_string());   // Prints "Foo".
```

ArrayList class:

```JavaScript
var list = new sys.ArrayList();
list.add("Apple");
list.add("Orange");
list.add("Banana");
list.sort();
for (var s : list) print(s);
```

HashMap class:

```JavaScript
var map = new sys.HashMap();
map.put("Apple", 5);
map.put("Orange", 10);
map.put("Banana", 15);
map.put("Apple", 33);
map.remove("Banana");
for (var k : map.keys()) print(k + " : " + map.get(k));
```

File class:

```JavaScript
var file = new sys.File("file.txt");
if (file.exists()) {
    print("File: " + file.get_absolute_path());
    print("Size: " + file.get_size());
    print("Text: " + file.read_text());
}
```

Stream API:

```JavaScript
var file = new sys.File("file.txt");
var out = file.text_out();
try {
    out.println("Hello ");
    out.println("World");
} finally {
    out.close();
}
print(file.read_text());
file.delete();
```

Socket API:

```JavaScript
var ss = new sys.ServerSocket(12345);
try {
    for (;;) {
        var s = ss.accept();
        try {
            s.write("Hello World".get_bytes());
        } finally {
            s.close();
        }
    }
} finally {
    ss.close();
}
```

Function sys.execute():

```JavaScript
var scope = new sys.HashMap();
scope.put("x", 5);
scope.put("y", 10);

var s = sys.execute("filename.s", "return x + y;", scope);
sys.out.println(s);   // Prints 15.
```

## Sample HTTP Server

A simlpe HTTP server written in the sample scripting language ([source code](https://github.com/antkar/syncpp/blob/master/sample/main/httpserver.s)).

Supports dynamic server pages written in the sample scripting language itself. An API similar to Java Servlet API is provided for dynamic server pages.

Sample dynamic page: Fibonacci Numbers (source code: [fibonacci.s](https://github.com/antkar/syncpp/blob/master/sample/web/root/fibonacci.s), [html.s](https://github.com/antkar/syncpp/blob/master/sample/web/lib/html.s)).

### Dynamic Pages Code Examples

Setting HTTP response headers and body:

```JavaScript
http.resp.set_content_type("text/plain; charset=UTF-8");
http.resp.add_header("Content-Language", "en");
http.resp.set_body("Hello World");
```

Writing response body as a stream:

```JavaScript
var out = http.resp.get_out(); // The content type is "text/html" by default.
out.write("<html>");
out.write("<head><title>Hello World</title></head>");
out.write("<body><h1>Hello World</h1></body>");
out.write("</html>");
```

Getting URL parameters:

```JavaScript
http.resp.set_content_type("text/plain");
var out = http.resp.get_out();

var params = http.req.get_parameters(); // Returns an array of parameter names, the order is undefined.
params.sort();
for (var param : params) {
    var value = http.req.get_parameter(param);
    out.write(param + " = " + value + "\n");
}
```

Context attributes (exist during server lifetime):

```JavaScript
var cnt = http.context.get_attribute("visit_count");
if (cnt == null) cnt = 0;
http.context.set_attribute("visit_count", cnt + 1);

http.resp.set_content_type("text/plain");
http.resp.set_body("This page has already been visited " + cnt + " time(s).");
```

Cookies:

```JavaScript
var body;

var cookie = http.req.get_cookie("my_cookie");
if (cookie == null) {
    body = "No cookie.";
    cookie = "0";
} else {
    body = "Cookie : " + cookie;
}

http.resp.set_content_type("text/plain");
http.resp.set_cookie("my_cookie", cookie + cookie.length());
http.resp.set_body(body);
```

Sessions and session attributes (sessions are maintained via cookies):

```JavaScript
var s = http.req.get_session();
var cnt = s.get_attribute("cnt");
if (cnt == null) cnt = 0;

var body = "You have already visited this page " + cnt + " time(s).";
if (cnt >= 5) {
    body += "\nClosing the session.";
    s.close();
} else {
    s.set_attribute("cnt", cnt + 1);
}

http.resp.set_content_type("text/plain");
http.resp.set_body(body);
```

### How to Run?

Download the binary distribution: [script-cpp-win32-1.0.zip](https://github.com/antkar/syncpp/releases/download/v1.0/script-cpp-win32-1.0.zip) (Windows only).

To execute an arbitrary script, run `script.cmd`, passing the script file name in the command line.

