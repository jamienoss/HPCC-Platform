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

//#define YYSTYPE TokenData


extern int ecl2yylex(YYSTYPE * yylval_param, EclParser * parser, yyscan_t yyscanner);

int yyerror(EclParser * parser, yyscan_t scanner, const char *msg);
int syntaxerror(const char *msg, short yystate, YYSTYPE token);
#define ecl2yyerror(parser, scanner, msg)   syntaxerror(msg, yystate, yylval)

int syntaxerror(const char *msg, short yystate, YYSTYPE token)
{
    std::cout << msg <<  " near line "  << token.pos.lineno << \
                     ", nearcol:" <<  token.pos.column <<  "\n";
    return 0;
}

%}

//GCH May need to rethink this so that position can be a member of the attribute class instead of a pointer.
/*%union
{
    TokenData returnToken;
    ISyntaxTree * treeNode;
}
*/
//=========================================== tokens ====================================

%token /*<YYSTYPE>*/
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

    YY_LAST_TOKEN

/*%type <YYSTYPE>
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
*/

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
    : line_of_code ';'              { $$.createSyntaxTree($2, $1.node); }
    |  eclQuery line_of_code ';'    { $$.createSyntaxTree($3, $2.node, $1.node); }
    ;

line_of_code
    : expr                          { $$.node = $1.node; }
    | IMPORT imports                { $$.createSyntaxTree($1); $$.node->transferChildren($2.node); }
    | lhs ASSIGN rhs                { $$.createSyntaxTree($2, $1.node, $3.node); }
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

expr
    : expr '+' expr                 { $$.createSyntaxTree($2, $1.node, $3.node); }
    | expr '-' expr                 { $$.createSyntaxTree($2, $1.node, $3.node); }
    | expr '*' expr                 { $$.createSyntaxTree($2, $1.node, $3.node); }
    | expr '/' expr                 { $$.createSyntaxTree($2, $1.node, $3.node); }
    | INTEGER                       { $$.createSyntaxTree($1); }
    | REAL                          { $$.createSyntaxTree($1); }
    | ID                            { $$.createSyntaxTree($1); }
    | set                           { $$.node = $1.node; }
    | ID '(' expr_list ')'          { $$.createSyntaxTree($1, $2, $4); $$.node->transferChildren($3.node); }
    | '(' expr ')'                  { $$.node = $2.node; }
    ;

expr_list
    : expr_list ',' expr            { $$.node = $1.node; $$.node->addChild($3.node); }
    | expr                          { $$.createSyntaxTree(); $$.node->addChild($1.node); }
    ;

fields
    : fields type ID ';'
                                    {
                                        $$.node = $1.node;
                                        ISyntaxTree * newField = SyntaxTree::createSyntaxTree($4, $2.node, $3.node);
                                        $$.node->addChild( newField );
                                    }
    | type ID ';'
                                    {
                                        $$.createSyntaxTree();
                                        ISyntaxTree * newField = SyntaxTree::createSyntaxTree($3, $1.node, $2);
                                        $$.node->addChild( newField );
                                    }
    ;

imports
    : module_list                   { $$.node = $1.node; }
    | ID AS ID                      {   // folder AS alias
                                        $$.createSyntaxTree($2, $1, $3);
                                    }
    | module_list FROM  module_from
                                    {
                                        //$$.createSyntaxTree($2.node, $1, $3.node);
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
    : ID                            { $$.createSyntaxTree($1); }
    ;

module_from
    : ID                            { $$.createSyntaxTree($1); }
    | DIR                           { $$.createSyntaxTree($1); }
    ;

module_list
    : module_list ',' module_symbols
                                    { $$.node = $1.node; $$.node->addChild($3.node); }
    | module_symbols                { $$.createSyntaxTree(',', $1.node->queryPosition()); $$.node->addChild($1.node); }
    ;

module_symbols
    : ID                            { $$.createSyntaxTree($1); }
    | '$'                           { $$.createSyntaxTree($1); }
    ;

inline_recordset
    : '{' inline_fields '}'         { }
    ;

records
    :                               { }
    ;

recordset
    : RECORD fields END             { $$.createSyntaxTree($1); $$.node->transferChildren($2.node); }
    ;

rhs
    : expr                          { $$.node = $1.node; }
    | recordset                     { $$.node = $1.node; }
    | inline_recordset              { $$.node = $1.node; }
    ;

set
    : '[' records ']'               { $$.createSyntaxTree(); $$.node->transferChildren($2.node); }
    ;

type
    : ID                            { $$.createSyntaxTree($1); }
    ;

%%

void appendParserTokenText(StringBuffer & target, unsigned tok)
{
    if (tok < 256)
        target.append((char)tok);
    else
        target.append(yytname[tok-256]);
}
