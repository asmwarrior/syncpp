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
