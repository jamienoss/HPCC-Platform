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
    SyntaxTree * treeNode;
}

//=========================================== tokens ====================================

%token <returnToken>
    ';'
    ASSIGN
    DIVIDE
    END
    ID
    INTEGER
    PLUS
    REAL
    RECORD
    MINUS
    MULTIPLY

    YY_LAST_TOKEN

%type <treeNode>
    eclQuery
    expr
    fields
    lhs
    line_of_code
    maths
    recordset
    rhs
    type

%left '.'
%left '('
%left '['
%left DIVIDE
%left MINUS
%left MULTIPLY
%left PLUS

%%
//================================== begin of syntax section ==========================

code
    : eclQuery                      { parser->setRoot($1); }
    ;

eclQuery
    : line_of_code ';'              { $$ = newNode(); $$->bifurcate($1, $2); }
    |  eclQuery line_of_code ';'    { $$ = new SyntaxTree($3); $$->bifurcate($2, $1); }
    ;

line_of_code
    : expr                          { $$ = $1; }
    | lhs ASSIGN rhs                { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
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
    | '(' maths ')'                 { $$ = $2; }
    ;

lhs
    : ID                            { $$ = new SyntaxTree($1); }
    ;

rhs
    : maths                         { $$ = $1; }
    | recordset                     { $$ = $1; }
    ;

//-----------------------------RECORDSETS-------------------------------------------------
recordset
    : RECORD fields END             { $$ = new SyntaxTree($1, $2); }
    ;

fields
    : type ID ';'                   { if($$->getAuxLength() < 1){$$ = new SyntaxTree();}; $$->add2Aux( new SyntaxTree($3, $1, $2) ); }
    | fields type ID ';'            { if($$->getAuxLength() < 1){$$ = new SyntaxTree();}; $$->add2Aux( new SyntaxTree($4, $2, $3) ); }
    ;
//-----------------------------------------------------------------------------------------

type
    : ID                            { $$ = new SyntaxTree($1); } //put actual reserved types in here
    ;

%%
