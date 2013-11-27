//
//    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//############################################################################## */

%define api.pure
%name-prefix "ecl2yy"

%parse-param {EclParser * parser}
%parse-param {yyscan_t scanner }
%lex-param {yyscan_t scanner}


%{
class TokenData;
class SyntaxTree;
class EclParser;
class EclLexer;

#include "platform.h"
#include "eclparser.hpp"
#include "eclgram.h"
#include "ecllex.hpp"
#include <iostream>

int yyerror(EclParser * parser, yyscan_t scanner, const char *msg) {
    //std::cout << *msg << "\n";
    std::cout << "Doh! Incorrect syntax\n";
    return 0;
}

inline SyntaxTree * newNode() {return new SyntaxTree(); }

%}

%union
{
    TokenData returnToken;
    SyntaxTree * node;
}

//=========================================== tokens ====================================

%token <returnToken>
  ID
  INTEGER
  REAL
  ';'
  PLUS
  MINUS
  MULTIPLY
  DIVIDE

  YY_LAST_TOKEN

%type <node>
    eclQuery
    line_of_code
    expr
    maths

%left '.'
%left '('
%left '['

%%
//================================== begin of syntax section ==========================

code
    : eclQuery                      { parser->setRoot($1); }
    ;

eclQuery
    : line_of_code ';'              { $$ = newNode(); $$->bifurcate($1, $2); }
    | line_of_code ';' eclQuery     { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    ;

line_of_code
    : expr                          { $$ = $1; }
    ;

expr
    : maths                         { $$ = $1; }
    ;

maths
    : maths PLUS maths              { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    | maths MINUS maths             { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    | maths MULTIPLY maths          { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    | maths DIVIDE maths            { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    | INTEGER                       { $$ = new SyntaxTree($1); }
    | REAL                          { $$ = new SyntaxTree($1); }
    | ID                            { $$ = new SyntaxTree($1); }
    ;
%%
