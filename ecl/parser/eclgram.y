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

%parse-param {EclParser * parser}
%parse-param {yyscan_t scanner}
%lex-param {EclParser * parser}
%lex-param {yyscan_t scanner}

%{
class EclParser;

#include "platform.h"
#include "eclparser.hpp"
#include "eclgram.h"
#include <iostream>

#define YYSTYPE ParserData

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

//=========================================== tokens ====================================

%token
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

/*%type <>
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
    : eclQuery                      { parser->setRoot($1.node.get()); }
    ;

eclQuery
    : line_of_code ';'              { $$.clear($2).add($1); }
    | eclQuery line_of_code ';'     { $$.clear($3).add($2).add($1); }
    ;

line_of_code
    : expr                          { $$.clear($1); }
    | IMPORT imports                { $$.clear($1).add($2); }
    | lhs ASSIGN rhs                { $$.clear($2).add($1).add($3); }
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

expr
    : expr '+' expr                 { $$.clear($2).add($1).add($3); }
    | expr '-' expr                 { $$.clear($2).add($1).add($3); }
    | expr '*' expr                 { $$.clear($2).add($1).add($3); }
    | expr '/' expr                 { $$.clear($2).add($1).add($3); }
    | INTEGER                       { $$.clear($1); }
    | REAL                          { $$.clear($1); }
    | ID                            { $$.clear($1); }
    | set                           { $$.clear($1); }
    | ID '(' expr_list ')'          { $$.clear($1).add($2).add($3).add($4);}
    | '(' expr ')'                  { $$.clear($2); }
    ;

expr_list
    : expr_list ',' expr            { $$.clear($1).add($3); }
    | expr                          { $$.clear(',', $1.pos).add($1); }
    ;

field
    : type ID ';'                   { $$.clear($3).add($1).add($2); }

fields
    : fields field                  { $$.clear($1).add($2); }
    | field                         { $$.clear(',', $1.pos).add($1); }
    ;

imports
    : module_list                   { $$.clear($1); }
    | ID AS ID                      { $$.clear($2).add($1).add($3); }
    | module_list FROM  module_from { $$.clear($2).add($1).add($3); }

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
    : ID                            { $$.clear($1); }
    ;

module_from
    : ID                            { $$.clear($1); }
    | DIR                           { $$.clear($1); }
    ;

module_list
    : module_list ',' module_symbols
                                    { $$.clear($1).add($3); }
    | module_symbols                { $$.clear(',', $1.pos).add($1); }
    ;

module_symbols
    : ID                            { $$.clear($1); }
    | '$'                           { $$.clear($1); }
    ;

inline_recordset
    : '{' inline_fields '}'         { }
    ;

records
    :                               { }
    ;

recordset
    : RECORD fields END             { $$.clear($1).add($2).add($3); } //MORE/NOTE inclusion of END for possible #if fix i.e. delay syntax check till semantics
    ;

rhs
    : expr                          { $$.clear($1); }
    | recordset                     { $$.clear($1); }
    | inline_recordset              { $$.clear($1); }
    ;

set
    : '[' records ']'               { $$.clear($1).add($2).add($3); }
    ;

type
    : ID                            { $$.clear($1); }
    ;

%%

void appendParserTokenText(StringBuffer & target, unsigned tok)
{
    if (tok < 256)
        target.append((char)tok);
    else
        target.append(yytname[tok-258+3]); //NOTE:+3 to buffer from '$end', '$error', and '$undefined' that Bison prefix is the list - yytname -with.
}                                          //-258 rather than 256 as Bison's 1st token starts at 258.
