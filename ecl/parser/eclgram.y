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
    '$'
    ';'
    AS
    ASSIGN
    DIR
    DIVIDE
    END
    FROM
    ID
    IMPORT
    PLUS
    MINUS
    MULTIPLY
    REAL
    RECORD
    UNSIGNED

    YY_LAST_TOKEN

%type <treeNode>
    eclQuery
    expr
    fields
    imports
    lhs
    line_of_code
    module_list
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
    : line_of_code ';'              { $$ = new SyntaxTree(); $$->bifurcate($1, $2); }
    |  eclQuery line_of_code ';'    { $$ = new SyntaxTree($3); $$->bifurcate($2, $1); }
    ;

line_of_code
    : expr                          { $$ = $1; }
    | IMPORT imports                { $$ = new SyntaxTree($1); $$->takeAux($2); }
    | lhs ASSIGN rhs                { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

expr
    : expr PLUS expr                { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    | expr MINUS expr               { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    | expr MULTIPLY expr            { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    | expr DIVIDE expr              { $$ = new SyntaxTree($2); $$->bifurcate($1, $3); }
    | UNSIGNED                      { $$ = new SyntaxTree($1); }
    | REAL                          { $$ = new SyntaxTree($1); }
    | ID                            { $$ = new SyntaxTree($1); }
    | '(' expr ')'                  { $$ = $2; }
    ;

fields
    : type ID ';'                   { if($$->getAuxLength() < 1){$$ = new SyntaxTree();}; $$->add2Aux( new SyntaxTree($3, $1, $2) ); }
    | fields type ID ';'            { if($$->getAuxLength() < 1){$$ = new SyntaxTree();}; $$->add2Aux( new SyntaxTree($4, $2, $3) ); }
    ;

imports
    : module_list                   { $$ = $1; }
    | ID AS ID                     { $$ = new SyntaxTree(); SyntaxTree * temp = new SyntaxTree($2); temp->bifurcate($1, $3); $$->add2Aux(temp); } // folder AS alias
    | module_list FROM  DIR         {  }
//  r/r error with ID in module_list  | ID                            { } // language - can this not be listed in module_list?
    ;

lhs
    : ID                            { $$ = new SyntaxTree($1); }
    ;

module_list
    : '$'                           { if($$->getAuxLength() < 1){$$ = new SyntaxTree();}; $$->add2Aux( new SyntaxTree($1) ); } // do we want to allow this to be repeated?
    | ID                            { if($$->getAuxLength() < 1){$$ = new SyntaxTree();}; $$->add2Aux( new SyntaxTree($1) ); }
    | module_list ',' ID            { if($$->getAuxLength() < 1){$$ = new SyntaxTree();}; $$->add2Aux( new SyntaxTree($3) ); }
    ;

recordset
    : RECORD fields END             { $$ = new SyntaxTree($1, $2); }
    ;

rhs
    : expr                          { $$ = $1; }
    | recordset                     { $$ = $1; }
    ;

type
    : ID                            { $$ = new SyntaxTree($1); }
    ;

%%
