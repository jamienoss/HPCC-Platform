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
    char full_msg[128];
    strcpy(full_msg,msg);
    parser->reportError(ERR_EXPECTED, strcat(full_msg,"jamies"), parser->queryLexer().sourcePath->str(), token.pos.lineno, token.pos.column, token.pos.position);
    return 0;
}

%}


//=========================================== tokens ====================================

%token
    AS
    AND
    ASSIGN ":="
    BEGINCPP
    BOOLEAN_CONST
    CPP
    DECIMAL_CONST
    DIR
    DIV "Division of integers as reals"
    DOTDOT ".."
    END
    ENDCPP
    ENDMACRO
    EQ "="
    FLOAT_CONST
    FROM
    FUNCTION
    FUNCTIONMACRO
    GE ">="
    GT ">"
    ID
    IFBLOCK
    IMPORT
    IN
    INTEGER_CONST
    LE "<="
    LT "<"
    MACRO
    MODULE
    NE "!="
    NOT
    OR
    PARSE_ID
    PATTERN
    REAL
    RECORD
    RESULTS_IN
    RETURN
    RULE
    SERVICE
    STRING_CONST
    TOKEN
    TRANSFORM
    TYPE
    XOR

    _EOF_ 0 "End of File"
    YY_LAST_TOKEN

%left LOWEST_PRECEDENCE
//%right ID

%left ':' ';' ',' '.'

%left '+' '-'
%left NE EQ LE GE LT GT
%left XOR //not sure about precedence?
%left OR
%left AND
%left NOT
%left IN
%left '*' '/'

%left '('
%left '['

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


nested_line_of_code
    : expr                          { }
    | import                        { }
    | assignment                    { }
    ;

un-nested_line_of_code
   : function_definition            { }
   | parse_support                  { }
   ;

line_of_code
    : un-nested_line_of_code        { }
    | nested_line_of_code           { }
    | /*EMPTY*/                     { }
    ;

nested_eclQuery
    : nested_eclQuery ';' nested_line_of_code
                                    { }
    | nested_line_of_code           { }
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

assignment
    : compound_id ASSIGN rhs        { }
    | compound_id ASSIGN expr ':' function
                                    { }
    ;

begincpp
    : compound_id ASSIGN BEGINCPP CPP ENDCPP
                                    { }
    ;

compound_id
    : compound_id identifier        { }
    | compound_id ':' identifier    { }
    | identifier                    { }
   // | '(' expr ')'                  { }
   // | '(' '>' expr '<' ')'          { }
    ;

constant
    : '(' expr ')' const { }
    | const { }
    ;

//GH: No need to distinguish these
const
    : BOOLEAN_CONST                 { }
    | STRING_CONST                  { }
    | DECIMAL_CONST                 { }
    | FLOAT_CONST                   { }
    | INTEGER_CONST                 { }
    ;

def_body
    : nested_eclQuery ';'             { }
    ;


expr
    : expr low_prec_op term         { }
    | term                          { }
    ;

expr_list
    : expr_list ',' expr            { }
    | expr                          { }
    ;

factor
    : paren_encaps_expr_expr        { }
    | '+' factor                    { }
    | '-' factor                    { }
    | constant                      { }
    | set                           { }
    | compound_id                   { }
    | record_definition             { }
    | module_definition             { }
    | service_definition            { }
    | transform                     { }
    | paren_encaps_expr_expr compound_id { }
    | RETURN identifier             { }
    | RETURN MODULE %prec LOWEST_PRECEDENCE { }
    ;

field
    : expr                          { }
    | expr '{' parameters '}'       { }
    | assignment                    { }
    | ifblock                       { }
    | /*EMPTY*/                     { }
    ;

fields
    : fields ';' field              { }
    | fields ',' field              { }
    | field                         { }
    ;

function
    : id '(' parameters opt_rec ')'  opt_set       { }
    | id '[' index_range ']'        { } // perhaps move this
    ;

opt_set
    : '[' index_range ']'            { }
    | /*EMPTY*/ %prec LOWEST_PRECEDENCE
                                     { }
    ;

function_definition
    : function_def                  { }
    | functionmacro_def             { }
    //| macro_def                     { }
    | begincpp                      { }
    ;

function_def
    : compound_id ASSIGN FUNCTION def_body RETURN expr ';' END
                                    { }
    ;

functionmacro_def
    : compound_id ASSIGN FUNCTIONMACRO def_body RETURN expr ';' ENDMACRO
                                    { }
    ;

high_prec_op
    : '/'                           { }
    | DIV                           { }
    | '%'                           { }
    | '*'                           { }
    | NOT %prec HIGHEST_PRECEDENCE  { }
    | AND                           { }
    | OR                            { }
    | XOR                           { }
    | NOT IN                        { }
    | IN                            { }
    ;

identifier
    : identifier '.' identifier     { }
    | function                      { }
    | id                            %prec LOWEST_PRECEDENCE    // Ensure that '(' gets shifted instead of causing a s/r error
                                    { }
    ;

id
    : ID {}
    | reserved_words   { }
    ;

reserved_words
   // : sort                          { }
  //  | TRANSFORM %prec LOWEST_PRECEDENCE { }
    : NOT %prec LOWEST_PRECEDENCE   { }
    ;

//sort
//    : SORT '(' parameters opt_rec ')' { }
//    | SORT %prec LOWEST_PRECEDENCE{ } 
//    ;

opt_rec
    : /*EMPTY*/ %prec LOWEST_PRECEDENCE{ }
    | ',' RECORD { }
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
    | module_path AS id             { }
    | module_list FROM module_path  { }
    ;

index_range
    : expr range_op expr            { }
    | expr range_op                 { }
    | range_op expr                 { }
    | parameters                    { }
    ;
/*
lhs
    : compound_id                   { }
    ;
*/

low_prec_op
    : '+'                           { }
    | '-'                           { }
    | LT                            { }
    | GT                            { }
    | GE                            { }
    | LE                            { }
    | EQ                            { }
    | NE                            { }
    | '?'                           { }
    | RESULTS_IN                    { }
  //  | NOT IN                        { }
  //  | IN                            { }
    ;

module_definition
    : MODULE record_options_all fields END
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
    : id                            { }
    ;

parameter
    : /*EMPTY*/                     { }
    | expr                          { }
    | assignment                    { }
    ;

parameters
    : parameters ',' parameter      { }
    | parameter                     { }
    ;

paren_encaps_expr_expr
    : paren_encaps_expr_expr  '(' expr ')'  { }
//   // : '(' expr ')'   factor 
    | '(' expr ')'                  { }
    | '(' '>' expr '<' ')'          { }
    ;

parse_support
    : PATTERN id ASSIGN pattern     { }
    | TOKEN id ASSIGN pattern       { }
    | RULE parse_support_rule_option id ASSIGN pattern
                                    { }
    ;

parse_support_rule_option
    : /*EMPTY*/   %prec LOWEST_PRECEDENCE                  { }
    ;

pattern
    : pattern pattern_definitions   { }
    | pattern_definitions           { }
    ;

pattern_definitions
    : PARSE_ID                      { }
    | PATTERN '(' expr ')'          { }
    | identifier                    { }
    //| ID     %prec LOWEST_PRECEDENCE                       { }
 //   | pat_id { }
 //   | ID '(' pattern ')' {}
 // | ID '(' parameters ')' {}
    | STRING_CONST                  { }
    | '*'                           { }
    | '+'                           { }
    | OR                            { }
    | '?'                           { }
    | '(' pattern ')'               { }
    | '[' pattern ']'               { }
    ;

//pat_id
 //   : pat_id '.' ID { }
 //   | ID %prec LOWEST_PRECEDENCE { }
 //   ;

range_op
    : DOTDOT                        { }
    ;

record_definition
    : RECORD record_options_all fields END
                                    { }
    | '{' record_options_all fields '}'
                                    { }
    ;

record_options
    : record_options ',' identifier { }
    | /*EMPTY*/                     %prec LOWEST_PRECEDENCE
                                    { }
    ;

record_options_all
    : record_options                %prec LOWEST_PRECEDENCE     // Ensure that '(' gets shifted instead of causing a s/r error
                                    { }
    | '(' expr ')' record_options  %prec LOWEST_PRECEDENCE { }//Could add '(' to list and "( )" as the parent node.
    ;

rhs
    : expr                          { }
    | type                          { }
    ;

set
    : '[' expr_list ']'             { } //Use parameters instead of expr_list??? 
    | '[' ']'                       { }
    ;

service_attribute
    : compound_id ':' service_keywords
                                    { }
    ;

service_attributes
    : service_attributes service_attribute ';'
                                    { }
    | service_attribute ';'         { }
    ;

service_keyword
    : id EQ expr                    { }
    | identifier                    { }
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

//************************************************************************
service_default_keywords
    : ':' service_keywords         { }// trailing ';' not present in current grammar
    | /*EMPTY*/   %prec LOWEST_PRECEDENCE                 { }
    ;
//************************************************************************

term
    : term high_prec_op factor      { }
    | factor                        { }
    ;

transform
    : TRANSFORM '(' transform_result ',' transform_parameters opt_semi ')'
    | compound_id ASSIGN TRANSFORM def_body END
                                    { }
    ;

trans_parameter
    : expr                          { }
    | assignment                    { }
    ;

opt_semi
    : ';'                           { }
    | /* EMPTY*/  %prec LOWEST_PRECEDENCE                  { }
    ;

transform_parameters
    : transform_parameters ';' trans_parameter 
                                    { }
    | trans_parameter               { }
    ;

transform_result
    : transform_result ',' trans_parameter { }
    | trans_parameter               { }
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
