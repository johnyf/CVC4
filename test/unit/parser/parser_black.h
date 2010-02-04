/*********************                                                        */
/** parser_black.h
 ** Original author: cconway
 ** Major contributors: dejan
 ** Minor contributors (to current version): mdeters
 ** This file is part of the CVC4 prototype.
 ** Copyright (c) 2009, 2010  The Analysis of Computer Systems Group (ACSys)
 ** Courant Institute of Mathematical Sciences
 ** New York University
 ** See the file COPYING in the top-level source directory for licensing
 ** information.
 **
 ** Black box testing of CVC4::parser::SmtParser.
 **/

#include <cxxtest/TestSuite.h>
//#include <string>
#include <sstream>

#include "expr/expr.h"
#include "expr/expr_manager.h"
#include "parser/parser.h"
#include "expr/command.h"

using namespace CVC4;
using namespace CVC4::parser;
using namespace std;

/************************** CVC test inputs ********************************/

const string goodCvc4Inputs[] = {
      "", // empty string is OK
      "ASSERT TRUE;",
      "QUERY TRUE;",
      "CHECKSAT FALSE;",
      "a, b : BOOLEAN;",
      "a, b : BOOLEAN; QUERY (a => b) AND a => b;",
      "%% nothing but a comment",
      "% a comment\nASSERT TRUE; %a command\n% another comment",
  };

const int numGoodCvc4Inputs = sizeof(goodCvc4Inputs) / sizeof(string);

const string cvc4ExprContext = "a,b,c:BOOLEAN;";

/* The following expressions are good in a context where a, b, and c have been declared as BOOLEAN. */
const string goodCvc4Exprs[] = {
    "a AND b",
    "a AND b OR c",
    "(a => b) AND a => b",
    "(a <=> b) AND (NOT a)",
    "(a XOR b) <=> (a OR b) AND (NOT (a AND b))"
};

const int numGoodCvc4Exprs = sizeof(goodCvc4Exprs) / sizeof(string);

const string badCvc4Inputs[] = {
      ";", // no command
      "ASSERT;", // no args
      "QUERY",
      "CHECKSAT",
      "a, b : boolean;", // lowercase boolean isn't a type
      "0x : INT;", // 0x isn't an identifier
      "a, b : BOOLEAN\nQUERY (a => b) AND a => b;", // no semicolon after decl
      "a : BOOLEAN; a: BOOLEAN;" // double decl
  };

const int numBadCvc4Inputs = sizeof(badCvc4Inputs) / sizeof(string);

/* The following expressions are bad even in a context where a, b, and c have been declared as BOOLEAN. */
const string badCvc4Exprs[] = {
    "a AND", // wrong arity
    "AND(a,b)", // not infix
    "(OR (AND a b) c)", // not infix
    "a IMPLIES b", // should be =>
    "a NOT b", // wrong arity, not infix
    "a and b" // wrong case
};

const int numBadCvc4Exprs = sizeof(badCvc4Exprs) / sizeof(string);

/************************** SMT test inputs ********************************/

const string goodSmtInputs[] = {
    "", // empty string is OK
    "(benchmark foo :assumption true)",
    "(benchmark bar :formula true)",
    "(benchmark qux :formula false)",
    "(benchmark baz :extrapreds ( (a) (b) ) )",
    "(benchmark zab :extrapreds ( (a) (b) ) :formula (implies (and (implies a b) a) b))",
    "(benchmark zub :extrasorts (a) :extrafuns ( (f a a) (x a) )  :formula (= (f x) x))",
    ";; nothing but a comment",
    "; a comment\n(benchmark foo ; hello\n  :formula true; goodbye\n)"
  };

const int numGoodSmtInputs = sizeof(goodSmtInputs) / sizeof(string);

const string smtExprContext = "(benchmark foo :extrapreds ((a) (b) (c)) )";

/* The following expressions are good in a context where a, b, and c have been declared as BOOLEAN. */
const string goodSmtExprs[] = {
    "(and a b)",
    "(or (and a b) c)",
    "(implies (and (implies a b) a) b)",
    "(and (iff a b) (not a))",
    "(iff (xor a b) (and (or a b) (not (and a b))))"
};

const int numGoodSmtExprs = sizeof(goodSmtExprs) / sizeof(string);

const string badSmtInputs[] = {
    "(benchmark foo)", // empty benchmark is not OK
    "(benchmark bar :assumption)", // no args
    "(benchmark bar :formula)",
    "(benchmark baz :extrapreds ( (a) (a) ) )", // double decl
    "(benchmark zub :extrasorts (a) :extrapreds (p a))" // (p a) needs parens
  };

const int numBadSmtInputs = sizeof(badSmtInputs) / sizeof(string);

/* The following expressions are bad even in a context where a, b, and c have been declared as BOOLEAN. */
const string badSmtExprs[] = {
    "(and a)", // wrong arity
    "(and a b", // no closing paren
    "(a and b)", // infix
    "(OR (AND a b) c)", // wrong case
    "(a IMPLIES b)", // infix AND wrong case
    "(not a b)", // wrong arity
    "not a" // needs parens
};

const int numBadSmtExprs = sizeof(badSmtExprs) / sizeof(string);

class SmtParserBlack : public CxxTest::TestSuite {
  ExprManager *d_exprManager;

  void tryGoodInputs(Parser::InputLanguage d_lang, const string goodInputs[], int numInputs) {
    for(int i = 0; i < numInputs; ++i) {
      try {
        // cout << "Testing good input: '" << goodInputs[i] << "'\n";
        istringstream stream(goodInputs[i]);
        Parser* smtParser = Parser::getNewParser(d_exprManager, d_lang, stream);
        TS_ASSERT( !smtParser->done() );
        Command* cmd;
        while((cmd = smtParser->parseNextCommand())) {
          // cout << "Parsed command: " << (*cmd) << endl;
        }
        TS_ASSERT( smtParser->parseNextCommand() == NULL );
        TS_ASSERT( smtParser->done() );
        delete smtParser;
      } catch (Exception& e) {
        cout << "\nGood input failed:\n" << goodInputs[i] << endl;
        throw e;
      }

    }
  }

  void tryBadInputs(Parser::InputLanguage d_lang, const string badInputs[], int numInputs) {
    for(int i = 0; i < numInputs; ++i) {
      // cout << "Testing bad input: '" << badInputs[i] << "'\n";
      istringstream stream(badInputs[i]);
      Parser* smtParser = Parser::getNewParser(d_exprManager, d_lang, stream);
      TS_ASSERT_THROWS
        ( while(smtParser->parseNextCommand());
          cout << "\nBad input succeeded:\n" << badInputs[i] << endl;, 
          ParserException );
      delete smtParser;
    }
  }

  void tryGoodExprs(Parser::InputLanguage d_lang,const string& context, const string goodBooleanExprs[], int numExprs) {
    // cout << "Using context: " << context << endl;
    for(int i = 0; i < numExprs; ++i) {
      try {
        // cout << "Testing good expr: '" << goodBooleanExprs[i] << "'\n";
        istringstream stream(context + goodBooleanExprs[i]);
        Parser* parser = Parser::getNewParser(d_exprManager, d_lang, stream);
        TS_ASSERT( !parser->done() );
        Command* cmd = parser->parseNextCommand();
        TS_ASSERT( !parser->done() );
        Expr e;
        while(e = parser->parseNextExpression()) {
          // cout << "Parsed expr: " << e << endl;
        }
        TS_ASSERT( parser->done() );
        TS_ASSERT( e.isNull() );
        delete cmd;
        delete parser;
      } catch (Exception& e) {
        cout << "\nGood expr failed:\n" << goodBooleanExprs[i] << endl;
        throw e;
      }
    }
  }

  void tryBadExprs(Parser::InputLanguage d_lang,const string badBooleanExprs[], int numExprs) {
    for(int i = 0; i < numExprs; ++i) {
      // cout << "Testing bad expr: '" << badBooleanExprs[i] << "'\n";
      istringstream stream(badBooleanExprs[i]);
      Parser* smtParser = Parser::getNewParser(d_exprManager, d_lang, stream);
      TS_ASSERT_THROWS
        ( smtParser->parseNextExpression();
          cout << "\nBad expr succeeded: " << badBooleanExprs[i] << endl;, 
          ParserException );
    }
  }

public:
  void setUp() {
    d_exprManager = new ExprManager();
  }

  void testGoodCvc4Inputs() {
    tryGoodInputs(Parser::LANG_CVC4,goodCvc4Inputs,numGoodCvc4Inputs);
  }

  void testBadCvc4Inputs() {
    tryBadInputs(Parser::LANG_CVC4,badCvc4Inputs,numBadCvc4Inputs);
  }

  void testGoodCvc4Exprs() {
    tryGoodExprs(Parser::LANG_CVC4,cvc4ExprContext,goodCvc4Exprs,numGoodCvc4Exprs);
  }

  void testBadCvc4Exprs() {
    tryBadExprs(Parser::LANG_CVC4,badCvc4Exprs,numBadCvc4Exprs);
  }

  void testGoodSmtInputs() {
    tryGoodInputs(Parser::LANG_SMTLIB,goodSmtInputs,numGoodSmtInputs);
  }

  void testBadSmtInputs() {
    tryBadInputs(Parser::LANG_SMTLIB,badSmtInputs,numBadSmtInputs);
  }

  void testGoodSmtExprs() {
    tryGoodExprs(Parser::LANG_SMTLIB,smtExprContext,goodSmtExprs,numGoodSmtExprs);
  }

  void testBadSmtExprs() {
    tryBadExprs(Parser::LANG_SMTLIB,badSmtExprs,numBadSmtExprs);
  }
};
