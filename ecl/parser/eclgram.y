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

%}

%union
{
    TokenData returnToken;
    SyntaxTree * treeNode;
}

//=========================================== tokens ====================================

%token <returnToken>
    '{'
    '}'
    ','
    ':'
    ';'
    '|'

    CODE
    DOUBLE_PERCENT
    NONTERMINAL
    PREC
    STUFF
    TERMINAL


    YY_LAST_TOKEN

%type <treeNode>
    grammar_item
    grammar_rule
    grammar_rules
    production
    terminals
    terminal_list
    terminal_productions


%left '.'
%left '{'

%%
//================================== beginning of syntax section ==========================

bison_file
    : grammar_item       { parser->setRoot($1); }
    ;

grammar_item
    : grammar_item grammar_rule     { $$ = $1; $$->addChild($2); }
    | grammar_rule                  { $$ = $$->createSyntaxTree(); $$->addChild($1); }
    ;

grammar_rule
    : NONTERMINAL grammar_rules ';'
                                    { $$ = $$->createSyntaxTree($1); $$->transferChildren($2); }
    ;

grammar_rules
    : grammar_rules '|' terminal_productions
                                    {
                                        $$ = $1;
                                        SyntaxTree * temp = temp->createSyntaxTree($2);
                                        temp->transferChildren($3);
                                        $$->addChild(temp);
                                    }
    | grammar_rules '|'
                                    {
                                        $$ = $1;
                                        SyntaxTree * temp = temp->createSyntaxTree($2);
                                        $$->addChild(temp);
                                    }
    | ':' terminal_productions      {
                                        $$ = $$->createSyntaxTree();
                                        SyntaxTree * temp = temp->createSyntaxTree($1);
                                        temp->transferChildren($2);
                                        $$->addChild(temp);
                                    }
    | ':'                           {
                                        $$ = $$->createSyntaxTree();
                                        SyntaxTree * temp = temp->createSyntaxTree($1);
                                        $$->addChild(temp);
                                    }
    ;

terminal_productions
    : terminal_list                 { $$ = $1; }
  //  | terminal_list production      { $$ = $1; $$->addChild($2); }
  //  | production                    { $$ = $$->createSyntaxTree(); $$->addChild($1); }
    ;

production
    : CODE                          { $$ = $$->createSyntaxTree($1); /*std::cout << $$->getLexeme() << "\n";*/ }
    ;

terminals
    : NONTERMINAL                   { $$ = $$->createSyntaxTree($1); }
    | TERMINAL                      { $$ = $$->createSyntaxTree($1); }
    | PREC                          { $$ = $$->createSyntaxTree($1); }
    | production                    { $$ = $1; }
    ;

terminal_list
    : terminal_list terminals       { $$ = $1; $$->addChild($2); }
    | terminals                     { $$ = $$->createSyntaxTree(); $$->addChild($1); }
    ;

%%









/*
code
    : eclQuery                      { parser->setRoot($1); }
    ;

eclQuery
    : line_of_code ';'              { $$ = $$->createSyntaxTree($2); $$->setLeft($1); }
    |  eclQuery line_of_code ';'    { $$ = $$->createSyntaxTree($3, $2, $1); }
    ;

line_of_code
    : expr                          { $$ = $1; }
    | IMPORT imports                { $$ = $$->createSyntaxTree($1); $$->transferChildren($2); }
    | lhs ASSIGN rhs                { $$ = $$->createSyntaxTree($2); $$->bifurcate($1, $3); }
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

expr
    : expr PLUS expr                { $$ = $$->createSyntaxTree($2, $1, $3); }
    | expr MINUS expr               { $$ = $$->createSyntaxTree($2, $1, $3); }
    | expr MULTIPLY expr            { $$ = $$->createSyntaxTree($2, $1, $3); }
    | expr DIVIDE expr              { $$ = $$->createSyntaxTree($2, $1, $3); }
    | UNSIGNED                      { $$ = $$->createSyntaxTree($1); }
    | REAL                          { $$ = $$->createSyntaxTree($1); }
    | ID                            { $$ = $$->createSyntaxTree($1); }
    | set                           { $$ = $1; }
    | ID '(' expr_list ')'          { $$ = $$->createSyntaxTree($1, $2, $4); $$->transferChildren($3); }
    | '(' expr ')'                  { $$ = $2; }
    ;

expr_list
    : expr_list ',' expr            { $$ = $1; $$->add2Aux($3); }
    | expr                          { $$ = $$->createSyntaxTree(); $$->add2Aux($1); }
    ;

fields
    : fields type ID ';'
                                    {
                                        $$ = $1;
                                        SyntaxTree * newField = newField->createSyntaxTree($4, $2, $3);
                                        $$->add2Aux( newField );
                                    }
    | type ID ';'
                                    {
                                        $$ = $$->createSyntaxTree();
                                        SyntaxTree * newField = newField->createSyntaxTree($3, $1, $2);
                                        $$->add2Aux( newField );
                                    }
    ;

imports
    : module_list                   { $$ = $1; }
    | ID AS ID                      {   // folder AS alias
                                        $$ = $$->createSyntaxTree();
                                        SyntaxTree * temp = temp->createSyntaxTree($1);
                                        $$->add2Aux(temp);
                                        temp = temp->createSyntaxTree($2);
                                        temp->setRight($3);
                                        $$->add2Aux(temp);
                                    }
    | module_list FROM  module_from
                                    {
                                        $$ = $$->createSyntaxTree();
                                        SyntaxTree * temp = temp->createSyntaxTree($2);
                                        temp->setRight($3);
                                        $$->transferChildren($1);
                                        $$->add2Aux(temp);
                                     }

//  r/r error with ID in module_list  | ID                            { } // language - can this not be listed in module_list?
    ;

inline_field
    : ID
    | INTEGER
    ;

inline_fields
    : records                       { }
    | inline_field
    ;

lhs
    : ID                            { $$ = $$->createSyntaxTree($1); }
    ;

module_from
    : ID                            { $$ = $$->createSyntaxTree($1); }
    | DIR                           { $$ = $$->createSyntaxTree($1); }
    ;

module_list
    : module_list ',' module_symbols
                                    { $$ = $1; $$->add2Aux( $3 ); }
    | module_symbols                { $$ = $$->createSyntaxTree(); $$->add2Aux($1); }
    ;

module_symbols
    : ID                            { $$ = $$->createSyntaxTree($1); }
    | '$'                           { $$ = $$->createSyntaxTree($1); }
    ;

inline_recordset
    : '{' inline_fields '}'         { }
    | ID                            { }
    |

recordset
    : RECORD fields END             { $$ = $$->createSyntaxTree($1); $$->transferChildren($2); }
    ;

rhs
    : expr                          { $$ = $1; }
    | recordset                     { $$ = $1; }
    | inline_recordset              { $$ = $1; }}
    ;

set
    : '[' records ']'               { $$ = $$->createSyntaxTree(); $$->transferChildren($2); }
    ;

type
    : ID                            { $$ = $$->createSyntaxTree($1); }
    ;

%%
*/