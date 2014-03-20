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
    AND
    ASSIGN ":="
    BOOLEAN_CONST
    DECIMAL_CONST
    DIR
    DIV "Division of integers as reals"
    DOTDOT ".."
    END
    EQ "="
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
    NOT
    OR
    PARSE_ID
    REAL
    RECORD
    SERVICE
    STRING_CONST
    TYPE
    XOR

    _EOF_ 0 "End of File"
    YY_LAST_TOKEN

%left LOWEST_PRECEDENCE

%left ':' ';' ',' '.'

%left '+' '-'
%left NE EQ LE GE LT GT
%left XOR //not sure about precedence?
%left OR
%left AND
%left NOT
%left '*' '/'

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
    | '(' expr ')' record_options  %prec LOWEST_PRECEDENCE { }//Could add '(' to list and "( )" as the parent node.
    ;

assignment
    : lhs ASSIGN rhs                { }
    ;

//GH: No need to distinguish these
constant
    : BOOLEAN_CONST                 { }
    | STRING_CONST                  { }
    | DECIMAL_CONST                 { }
    | FLOAT_CONST                   { }
    | INTEGER_CONST                 { }
    ;

low_prec_op
    : '+'                           { }
    | '-'                           { }
    | LT                            { }
    | GT                            { }
    | GE                            { }
    | LE                            { }
    | EQ                            { }
    | NE                            { }
    ;

high_prec_op
    : '/'                           { }
    | DIV                           { }
    | '%'                           { }
    | '*'                           { }
    | NOT                           { }
    | AND                           { }
    | OR                            { }
    | XOR                           { }
    ;

expr
    : expr low_prec_op term         { }
    | term                          { }
    ;

term
    : term high_prec_op factor      { }
    | factor                        { }
    ;

factor
    : paren_encaps_expr_expr        { }
    | '+' factor                    { }
    | '-' factor                    { }
    | constant                      { }
    | set                           { }
    | id_list                       { }
    | record_definition             { }
    | module_definition             { }
    | service_definition            { }
    ;

expr_list
    : expr_list ',' expr            { }
    | expr                          { }
    ;

field
    : expr                          { }
    | expr '{' parameters '}'       { }
    | assignment                    { }
    | ifblock                       { }
    | { }
    ;

fields
    : fields ';' field              { }
    | fields ',' field              { }
    | field                         { }
    ;

function
    : ID '(' parameters ')'         { }
    | ID '[' index_range ']'        { }
    ;

id_list
    : id_list identifier            { }
    | identifier                    { }
    ;

identifier
    : identifier '.' identifier     { }
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

paren_encaps_expr_expr
    : paren_encaps_expr_expr '(' expr ')'
                                    { }
    | '(' expr ')'                  { }
    ;

parameter
    :                               { }
    | expr                          { }
    | assignment                    { }
    ;

parameters
    : parameters ',' parameter      { }
    | parameter                     { }
    ;

range_op
    : DOTDOT                        { }
    ;

record_definition
    : RECORD all_record_options fields END
                                    { }
    | '{' all_record_options fields '}'
                                    { }
    ;

record_options
    : record_options ',' identifier { }
    |                               %prec LOWEST_PRECEDENCE
                                    { }
    ;

rhs
    : expr                          { }
    | type                          { }
    ;

set
    : '[' expr_list ']'             { }
    | '[' ']'                       { }
    ;

service_attribute
    : function ':' service_keywords { }
    ;

service_attributes
    : service_attributes service_attribute ';'
                                    { }
    | service_attribute ';'         { }
    ;

service_keyword
    : ID EQ expr                    { }
    ;

service_keywords
    : service_keywords ',' service_keyword
                                    { }
    | service_keyword               { }
    ;

service_definition
    : SERVICE service_default_keywords service_attributes END
                                    { }
    ;

service_default_keywords
    : ':' service_keywords ';'      { }//';' not present in current grammar
    |                               { }
    ;

type
    : TYPE fields END               { }
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
