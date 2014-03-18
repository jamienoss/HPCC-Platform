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
#include "hqlerrors.hpp"
#include <iostream>

#define YYSTYPE ParserData //NOTE: if you include eclgram.h this needs to be placed after this definition

extern int ecl2yylex(YYSTYPE * yylval_param, EclParser * parser, yyscan_t yyscanner);

int yyerror(EclParser * parser, yyscan_t scanner, const char *msg);
int syntaxerror(const char *msg, short yystate, YYSTYPE token);
#define ecl2yyerror(parser, scanner, msg)   syntaxerror(msg, yystate, yylval, parser)

int syntaxerror(const char *msg, short yystate, YYSTYPE token, EclParser * parser)
{
    parser->reportError(ERR_EXPECTED, msg, parser->queryLexer().sourcePath->str(), token.pos.lineno, token.pos.column, token.pos.position);
    return 0;
}

%}

//=========================================== tokens ====================================

%token
    AS
    ASSIGN ":="
    BOOLEAN_CONST
    DECIMAL_CONST
    DIR
    DOTDOT ".."
    END
    EQ "=="
    FLOAT_CONST
    FROM
    GE ">="
    GT ">"
    ID
    IFBLOCK
    IMPORT
    INTEGER_CONST
    LE "<="
    LT "<"
    NE "!="
    PARSE_ID
    REAL
    RECORD
    STRING_CONST
    TYPE
    UPDIR ".^"

    _EOF_ 0 "End of File"
    YY_LAST_TOKEN


%left ';' ',' '.'
%left '+' '-'
%left '*' '/'

%left UPDIR
%left NE EQ LE GE LT GT


%right '='

%%
//================================== begin of syntax section ==========================

code
    : eclQuery                      { }
    ;

eclQuery
    : eclQuery ';' eclQuery  _EOF_  { }
    | eclQuery ';'                  { }
    | line_of_code                  { }
    | ';' line_of_code              { }
    ;

line_of_code
    : expr                          { }
    | IMPORT import                 { }
    | assignment                    { }
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

all_record_options
    : record_options                { }
    | '(' expr ')'                  { }//Could add '(' to list and "( )" as the parent node.
    ;

assignment
    : lhs ASSIGN rhs                { } /*should this be an expr???*/
    ;

constant
    : BOOLEAN_CONST                 { }
    | STRING_CONST                  { }
    | DECIMAL_CONST                 { }
    | FLOAT_CONST                   { }
    | INTEGER_CONST                 { }
    ;

expr
    : '+' expr                      { }//not left rec, hmmmm?!? MORE: could make '+' abstract here!!!
    | '-' expr                      { }//not left rec, hmmmm?!?
    | expr_op_expr                  { }
    | constant                      { }
    | set                           { }
    | id_list                       { }
    | '(' expr ')'                  { } /*might want to re-think discarding parens - I don't think so!*/
    | '(' ')'                       { }
    ;

//expr
//    : expr '+' term                 { }
//    | expr '-' term                 { }
//    | term                          { }
//    ;

//term
//    : term '*' factor               { }
//    | term '/' factor               { }
//    | factor                        { }
//    ;

//factor
//    : '(' expr ')'                  { }
//    | constant                      { }
//    | set                           { }
//    | id_list                       { }
//    | '(' ')'                       { }
//    ;

expr_op_expr
    : expr '+' expr                 { }
    | expr '-' expr                 { }
    | expr '*' expr                 { }
    | expr '/' expr                 { }
    | expr '=' expr                 { }
    | expr NE expr                  { }
    | expr EQ expr                  { }
    | expr LE expr                  { }
    | expr LT expr                  { }
    | expr GE expr                  { }
    | expr GT expr                  { }
    ;

field
    : id_list                       { }
    | assignment                    { }
    | ifblock                       { }
 //   | identifier                    { }
    ;

fields
    : fields field ';'              { }
    | field ';'                     { }
    ;

function
    : ID '(' parameters ')'         { }
    | ID '(' ')'                    { }
    | ID '{' parameters '}'         { }
    | ID '[' index_range ']'        { }
    ;

id_list
    : id_list identifier            { }  /*This needs further thought inc. whether to swap order of $1 & $2*/
    | identifier                    { }
    ;

identifier
    : identifier '.' identifier     { } //Might want to make '.' abstract
    | function                      { }
    | ID                            { }
    ;

ifblock
    : IFBLOCK '(' expr ')' fields END
                                    { }
    ;

import
    : module_list                   { }
    | module_symbols AS ID          { }
    | module_list FROM  module_from { }
    ;

index
    : expr                          { }
    //: INTEGER_CONST                 { }
    //| identifier                    { }//maybe reduce to just ID
    ;

index_range
    : index_range range_op index    { }
    | index_range range_op          { }
    | range_op index                { }
    | index                         { }
    ;

lhs
    : id_list                       { }
    ;

module_from
    : identifier                    { }
    | DIR                           { }
    ;

module_list
    : module_list ',' module_path   { }
    | module_path                   { }
    ;

module_path
    : '$' module_symbols            { }
    | '$' '.' module_symbols        { }
    | '$' UPDIR module_symbols      { }
    | module_symbols                { }
    ;

module_symbols
    : module_symbols UPDIR module_symbols
                                    { }
    | module_symbols UPDIR          { } //MORE might need to consider strings and not just char tokens
    | identifier                    { }
    ;

parameter
    : expr                          { }
    | ','                           { }
    | ',' assignment                { } /* not obvious why you'd want to shape the ST like this, i.e. miss out the ','*/
    | assignment                    { }
    ;

parameters
    : parameters ',' parameter      { }
    | parameters ','                { }
    | parameter                     { } /*perhaps re-think - this creates a comma list even if only one parameter*/
    ;

range_op
    : DOTDOT                        { }
    | ':'                           { }//If this conflicts with existing ecl then take out
                                                     //otherwise might help, most math lang uses this.
    ;

record
    : expr                          { }
    ;

records
    : records ',' record            { }
    | record                        { }
    ;

recordset
    : RECORD all_record_options fields END
                                    { } //MORE/NOTE inclusion of END for possible #if fix i.e. delay syntax check till semantics
    | '{' all_record_options fields '}'
                                    { }
//   'records' used to be, still could/should be, fields
    ;

record_options
    : record_options ',' identifier { }
    |                               { }//Need to change first & add to not account for empty
    ;

rhs
    : expr                          { }
    | recordset                     { }
    | type                          { }
    ;

set
    : '[' records ']'               { }
    | '[' ']'                       { }
    ;

type
    : TYPE fields END             { } //MORE/NOTE inclusion of END for possible #if fix i.e. delay syntax check till semantics
    ;

%%

void appendParserTokenText(StringBuffer & target, unsigned tok)
{
    if (tok == 0)
    {
        StringBuffer tokenName = yytname[tok];
        tokenName.replaceString("\"", "");
        target.append(tokenName.str());
    }
    else if (tok > 0 && tok < 258)
        target.append((char)tok);
    else if (tok > 257)
    {
        StringBuffer tokenName = yytname[tok-258+3];
        tokenName.replaceString("\"", "");
        target.append(tokenName.str()); //NOTE:+3 to buffer from '$end', '$error', and '$undefined' that Bison prefix is the list - yytname -with.
                                          //-258 rather than 256 as Bison's 1st token starts at 258.
    }
}
