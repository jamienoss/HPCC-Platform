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
%error-verbose

//%param {EclParser * parser} {yyscan_t scanner}
%parse-param {EclParser * parser}
%parse-param {yyscan_t scanner}
%lex-param {EclParser * parser}
%lex-param {yyscan_t scanner}

%{
class TokenData;
class SyntaxTree;
class EclParser;
class EclLexer;

#include "platform.h"
#include "eclparser.hpp"
#include "eclgram.h"
//#include "ecllex.hpp"
#include <iostream>

extern int ecl2yylex(YYSTYPE * yylval_param, EclParser * parser, yyscan_t yyscanner);

int yyerror(EclParser * parser, yyscan_t scanner, const char *msg);
int syntaxerror(const char *msg, short yystate, YYSTYPE token);
#define ecl2yyerror(parser, scanner, msg)   syntaxerror(msg, yystate, yylval)

int syntaxerror(const char *msg, short yystate, YYSTYPE token)
{
    std::cout << msg <<  " near line "  << token.returnToken.pos->lineno << \
                     ", nearcol:" <<  token.returnToken.pos->column <<  "\n";
    return 0;
}


%}

//GCH May need to rethink this so that position can be a member of the attribute class instead of a pointer.
%union
{
    TokenData returnToken;
    ISyntaxTree * treeNode;
}

//=========================================== tokens ====================================

%token <returnToken>
    '['
    ']'
    '{'
    '}'
    '('
    ')'
    ','
    '$'
    ';'
    '+'
    '-'
    '*'
    '/'
    AS
    ASSIGN
    DIR
    END
    FROM
    ID
    IMPORT
    INTEGER
    REAL
    RECORD
    UNSIGNED

    YY_LAST_TOKEN

%type <treeNode>
    eclQuery
    expr
    expr_list
    fields
    imports
    inline_recordset
    lhs
    line_of_code
    module_from
    module_list
    module_symbols
    records
    recordset
    rhs
    set
    type

%left '.'
%left '('
%left '{'
%left '['
%left '/'
%left '-'
%left '*'
%left '+'

%%
//================================== begin of syntax section ==========================

code
    : eclQuery                      { parser->setRoot($1); }
    ;

eclQuery
    : line_of_code ';'              { $$ = SyntaxTree::createSyntaxTree($2, $1); }
    |  eclQuery line_of_code ';'    { $$ = SyntaxTree::createSyntaxTree($3, $2, $1); }
    ;

line_of_code
    : expr                          { $$ = $1; }
    | IMPORT imports                { $$ = SyntaxTree::createSyntaxTree($1); $$->transferChildren($2); $2->Release(); }
    | lhs ASSIGN rhs                { $$ = SyntaxTree::createSyntaxTree($2, $1, $3); }
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

expr
    : expr '+' expr                 { $$ = SyntaxTree::createSyntaxTree($2, $1, $3); }
    | expr '-' expr                 { $$ = SyntaxTree::createSyntaxTree($2, $1, $3); }
    | expr '*' expr                 { $$ = SyntaxTree::createSyntaxTree($2, $1, $3); }
    | expr '/' expr                 { $$ = SyntaxTree::createSyntaxTree($2, $1, $3); }
    | UNSIGNED                      { $$ = SyntaxTree::createSyntaxTree($1); }
    | REAL                          { $$ = SyntaxTree::createSyntaxTree($1); }
    | ID                            { $$ = SyntaxTree::createSyntaxTree($1); }
    | set                           { $$ = $1; }
    | ID '(' expr_list ')'          { $$ = SyntaxTree::createSyntaxTree($1, $2, $4); $$->transferChildren($3); $3->Release(); }
    | '(' expr ')'                  { $$ = $2; }
    ;

expr_list
    : expr_list ',' expr            { $$ = $1; $$->addChild($3); }
    | expr                          { $$ = SyntaxTree::createSyntaxTree(); $$->addChild($1); }
    ;

fields
    : fields type ID ';'
                                    {
                                        $$ = $1;
                                        ISyntaxTree * newField = SyntaxTree::createSyntaxTree($4, $2, $3);
                                        $$->addChild( newField );
                                    }
    | type ID ';'
                                    {
                                        $$ = SyntaxTree::createSyntaxTree();
                                        ISyntaxTree * newField = SyntaxTree::createSyntaxTree($3, $1, $2);
                                        $$->addChild( newField );
                                    }
    ;

imports
    : module_list                   { $$ = $1; }
    | ID AS ID                      {   // folder AS alias
                                        $$ = SyntaxTree::createSyntaxTree($2, $1, $3);
                                    }
    | module_list FROM  module_from
                                    {
                                        $$ = SyntaxTree::createSyntaxTree($2, $1, $3);
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
    : ID                            { $$ = SyntaxTree::createSyntaxTree($1); }
    ;

module_from
    : ID                            { $$ = SyntaxTree::createSyntaxTree($1); }
    | DIR                           { $$ = SyntaxTree::createSyntaxTree($1); }
    ;

module_list
    : module_list ',' module_symbols
                                    { $$ = $1; $$->addChild( $3 ); }
    | module_symbols                { $$ = SyntaxTree::createSyntaxTree(',', $1->queryPosition()); $$->addChild($1); }
    ;

module_symbols
    : ID                            { $$ = SyntaxTree::createSyntaxTree($1); }
    | '$'                           { $$ = SyntaxTree::createSyntaxTree($1); }
    ;

inline_recordset
    : '{' inline_fields '}'         { }
    ;

records
    :                               { }
    ;

recordset
    : RECORD fields END             { $$ = SyntaxTree::createSyntaxTree($1); $$->transferChildren($2); $2->Release(); }
    ;

rhs
    : expr                          { $$ = $1; }
    | recordset                     { $$ = $1; }
    | inline_recordset              { $$ = $1; }
    ;

set
    : '[' records ']'               { $$ = SyntaxTree::createSyntaxTree(); $$->transferChildren($2); $2->Release(); }
    ;

type
    : ID                            { $$ = SyntaxTree::createSyntaxTree($1); }
    ;

%%

void appendParserTokenText(StringBuffer & target, unsigned tok)
{
    if (tok < 256)
        target.append((char)tok);
    else
        target.append(yytname[tok-256]);
}
