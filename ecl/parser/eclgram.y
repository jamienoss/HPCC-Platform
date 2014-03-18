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
    MODULE
    NE "!="
    PARSE_ID
    REAL
    RECORD
    STRING_CONST
    TYPE

    _EOF_ 0 "End of File"
    YY_LAST_TOKEN

%left LOWEST_PRECEDENCE

%left ';' ',' '.'

%left '+' '-'
%left '*' '/'
%left NE EQ LE GE LT GT

%left '('
%left '['

%right '='

%left HIGHEST_PRECEDENCE

%%

//================================== begin of syntax section ==========================

code
    : eclQuery                      { }
    ;

eclQuery
    : eclQuery ';' line_of_code     { }
    | line_of_code                  { }
    ;

line_of_code
    : expr                          { }
    | import                        { }
    | assignment                    { }
    |
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

//We need to add some precedence rules to avoid some shift reduce errors.
//RECORD(base) matches
//   '(' expr ')' in all_record_options
//   expr in field.
//We want the first to be used, so we ensure the '(' gets shifted instead of reduced
//
//  RECORD,ID(3) matches
//   id in record_options
//   ID in record options and expr in field.
// Again we want the first to be used, so shift
//
// ID(3), ID[3] - if you are allowed expr expr then it is ambiguous

all_record_options
    : record_options                %prec LOWEST_PRECEDENCE     // Ensure that '(' gets shifted instead of causing a s/r error
                                    { }
    | '(' expr ')' record_options   { }//Could add '(' to list and "( )" as the parent node.
    ;

assignment
    : lhs ASSIGN rhs                { } /*should this be an expr???*/
    ;

//GH: No need to distinguish these
constant
    : BOOLEAN_CONST                 { }
    | STRING_CONST                  { }
    | DECIMAL_CONST                 { }
    | FLOAT_CONST                   { }
    | INTEGER_CONST                 { }
    ;

//GH: Note this is a general object expression - not just a scalar expression
expr
    : '+' expr                      { }//not left rec, hmmmm?!? MORE: could make '+' abstract here!!!
    | '-' expr                      { }//not left rec, hmmmm?!? MORE:this may no longer work since '-' is now left rather than right prec
    | expr_op_expr                  { }
    | constant                      { }
    | set                           { }
    | id_list                       { }
    | '(' expr ')'                  { } /*might want to re-think discarding parens - I don't think so!*/
    | record_definition             { }
    | module_definition             { }
//GH deleted    | '(' ')'                       { }
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
//    : '+' factor                    { }
//    | '-' factor                    { }
//    | constant                      { }
//    | set                           { }
//    | id_list                       { }
//    | '(' ')'                       { }
//    ;

expr_list
    : expr_list ',' expr            { }
    | expr                          { }
    ;

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
    : expr                          { }
    | expr '{' parameters '}'       { }
    | assignment                    { }
    | ifblock                       { }
    ;

fields
    : fields field ';'              { }
    |                               { }
    ;

function
    : ID '(' parameters ')'         { }
    | ID '[' index_range ']'        { }
    ;

id_list
    : id_list identifier            { }  /*This needs further thought inc. whether to swap order of $1 & $2*/
    | identifier                    { }
    ;

identifier
    : identifier '.' identifier     { } //Might want to make '.' abstract
    | function                      { }
    | ID                            %prec LOWEST_PRECEDENCE     // Ensure that '(' gets shifted instead of causing a s/r error
                                    { }
    ;

ifblock
    : IFBLOCK '(' expr ')' fields END
                                    { }
    ;

import
    : IMPORT import_reference       { }
    ;

import_reference
    : module_list                   { }
    | module_path AS ID             { }
    | module_list FROM module_path  { }
    ;

index_range
    : expr range_op expr            { }
    | expr range_op                 { }
    | range_op expr                 { }
    | expr                          { }
    ;

lhs
    : id_list                       { }
    ;

module_definition
    : MODULE all_record_options fields END
                                    { }
    ;

module_list
    : module_list ',' module_path   { }
    | module_path                   { }
    ;

module_path
    : '$'                           { }
    | module_symbol                 { }
    | module_path '.' module_symbol { }
    | module_path '.' '^'           { }
    ;

module_symbol
    : ID                            { }
    ;

parameter
    :                               { }
    | expr                          { }
    | assignment                    { }
    ;

parameters
    : parameters ',' parameter      { }
    | parameter                     { } /*perhaps re-think - this creates a comma list even if only one parameter*/
    ;

range_op
    : DOTDOT                        { }
//GH delete    | ':'                           { }//If this conflicts with existing ecl then take out
//                                                     //otherwise might help, most math lang uses this.
    ;

record_definition
    : RECORD all_record_options fields END
                                    { } //MORE/NOTE inclusion of END for possible #if fix i.e. delay syntax check till semantics
    | '{' all_record_options fields '}'
                                    { }
//   'records' used to be, still could/should be, fields
    ;

record_options
    : record_options ',' identifier { }
    |                               %prec LOWEST_PRECEDENCE
                                    { }//Need to change first & add to not account for empty
    ;

rhs
    : expr                          { }
    | type                          { }
    ;

set
    : '[' expr_list ']'               { }
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
