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
    parser->reportError(ERR_EXPECTED, msg, parser->getLexer().sourcePath->str(), token.pos.lineno, token.pos.column, token.pos.position);
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
    REAL
    RECORD
    STRING_CONST
    TYPE
    UPDIR ".^"

    _EOF_ 0 "End of File"
    YY_LAST_TOKEN

%left ';' ',' '.'
%left '+' '*' '/'
%left UPDIR
%left NE EQ LE GE LT GT

%right '-'
%right '='

%%
//================================== begin of syntax section ==========================

code
    : eclQuery                      { parser->setRoot($1.queryNode()); }
    ;

eclQuery
    : eclQuery ';' eclQuery _EOF_   { $$.setNode($2).addChild($1).addChild($3); }
    | eclQuery ';'                  { $$.setNode($2).addChild($1); }
    | line_of_code                  { $$.setNode($1); }
    | ';' line_of_code              { $$.setNode($2); }
    ;

line_of_code
    : expr                          { $$.setNode($1); }
    | IMPORT import                { $$.setNode($1).addChild($2); }
    | assignment                    { $$.setNode($1); }
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

all_record_options
    : record_options                { $$.setNode($1); }
    | '(' expr ')'                  { $$.setEmptyNode().addChild($1).addChild($2).addChild($3); }//Could add '(' to list and "( )" as the parent node.
    ;

assignment
    : lhs ASSIGN rhs                { $$.setNode($2).addChild($1).addChild($3); } /*should this be an expr???*/
    ;

constant
    : BOOLEAN_CONST                 { $$.setNode($1); }
    | STRING_CONST                  { $$.setNode($1); }
    | DECIMAL_CONST                 { $$.setNode($1); }
    | FLOAT_CONST                   { $$.setNode($1); }
    | INTEGER_CONST                 { $$.setNode($1); }
    ;

expr
    : '+' expr                      { $$.setNode($1).addChild($2); }
    | '-' expr                      { $$.setNode($1).addChild($2); }
    | expr_op_expr                  { $$.setNode($1); }
    | constant                      { $$.setNode($1); }
    | set                           { $$.setNode($1); }
    | id_list                       { $$.setNode($1); }
    | '(' expr ')'                  { $$.setNode($2); } /*might want to re-think discarding parens - I don't think so!*/
    | '(' ')'                       { $$.setNode($1).addChild($2); }
    ;

expr_op_expr
    : expr '+' expr                 { $$.setNode($2).addChild($1).addChild($3); }
    | expr '-' expr                 { $$.setNode($2).addChild($1).addChild($3); }
    | expr '*' expr                 { $$.setNode($2).addChild($1).addChild($3); }
    | expr '/' expr                 { $$.setNode($2).addChild($1).addChild($3); }
    | expr '=' expr                 { $$.setNode($2).addChild($1).addChild($3); }
    | expr NE expr                  { $$.setNode($2).addChild($1).addChild($3); }
    | expr EQ expr                  { $$.setNode($2).addChild($1).addChild($3); }
    | expr LE expr                  { $$.setNode($2).addChild($1).addChild($3); }
    | expr LT expr                  { $$.setNode($2).addChild($1).addChild($3); }
    | expr GE expr                  { $$.setNode($2).addChild($1).addChild($3); }
    | expr GT expr                  { $$.setNode($2).addChild($1).addChild($3); }
    ;

field
    : id_list                       { $$.setNode($1); }
    | assignment                    { $$.setNode($1); }
    | ifblock                       { $$.setNode($1); }
 //   | identifier                    { $$.setNode($1); }
    ;

fields
    : fields field ';'              { $$.setNode($1).addChild($2); }
    | field ';'                     { $$.setNode(',', $1.queryNodePosition()).addChild($1); }
    ;

function
    : ID '(' parameters ')'         { $$.setNode($1).addChild($2).addChild($3).addChild($4); }
    | ID '(' ')'                    { $$.setNode($1).addChild($2).addChild($3); }
    | ID '{' parameters '}'         { $$.setNode($1).addChild($2).addChild($3).addChild($4); }
    | ID '[' index_range ']'        { $$.setNode($1).addChild($2).addChild($3).addChild($4); }
    ;

id_list
    : id_list identifier            { $$.setNode($1).addChild($2); }  /*This needs further thought inc. whether to swap order of $1 & $2*/
    | identifier                    { $$.setNode($1); }
    ;

identifier
    : identifier '.' identifier     { $$.setNode($2).addChild($1).addChild($3); } //Might want to make '.' abstract
    | function                     { $$.setNode($1); }
    | ID                            { $$.setNode($1); }
    ;

ifblock
    : IFBLOCK '(' expr ')' fields END
                                    { $$.setNode($1).addChild($2).addChild($3).addChild($4).addChild($5); }
    ;

import
    : module_list                   { $$.setNode($1); }
    | module_symbols AS ID          { $$.setNode($2).addChild($1).addChild($3); }
    | module_list FROM  module_from { $$.setNode($2).addChild($1).addChild($3); }
    ;

index
    : expr                          { $$.setNode($1); }
    //: INTEGER_CONST                 { $$.setNode($1); }
    //| identifier                    { $$.setNode($1); }//maybe reduce to just ID
    ;

index_range
    : index_range range_op index    { $$.setNode($2).addChild($1).addChild($3); }
    | index_range range_op          { $$.setNode($2).addChild($1); }
    | range_op index                { $$.setNode($1).addChild($2); }
    | index                         { $$.setNode($1); }
    ;

lhs
    : id_list                       { $$.setNode($1); }
    ;

module_from
    : identifier                    { $$.setNode($1); }
    | DIR                           { $$.setNode($1); }
    ;

module_list
    : module_list ',' module_symbols
                                    { $$.setNode($1).addChild($3); }
    | module_symbols                { $$.setNode(',', $1.queryNodePosition()).addChild($1); }
    ;

module_symbols
//    : module_symbols '.' identifier  { $$.setNode($1).addChild($2).addChild($3); }
    : module_symbols UPDIR module_symbols
                                    { $$.setNode($2).addChild($1).addChild($3); }
    | module_symbols UPDIR          { $$.setNode($2).addChild($1); } //MORE might need to consider strings and not just char tokens
    | identifier                    { $$.setNode($1); }
    | '$'                           { $$.setNode($1); }
//    | ID                            { $$.setNode($1); }
    ;

parameter
    : expr                          { $$.setNode($1); }
    | ','                           { $$.setNode($1); }
    | ',' assignment                { $$.setNode($2); } /* not obvious why you'd want to shape the ST like this, i.e. miss out the ','*/
    | assignment                    { $$.setNode($1); }
    ;

parameters
    : parameters ',' parameter      { $$.setNode($1).addChild($2).addChild($3); }
    | parameters ','                { $$.setNode($1).addChild($2); }
    | parameter                     { $$.setNode(',', $1.queryNodePosition()).addChild($1); } /*perhaps re-think - this creates a comma list even if only one parameter*/
    ;

range_op
    : DOTDOT                        { $$.setNode($1); }
    | ':'                           { $$.setNode($1); }//If this conflicts with existing ecl then take out
                                                     //otherwise might help, most math lang uses this.
    ;

record
    : expr                          { $$.setNode($1); }
    ;

records
    : records ',' record            { $$.setNode($1).addChild($2).addChild($3); }
    | record                        { $$.setNode(',', $1.queryNodePosition()).addChild($1); }
    ;

recordset
    : RECORD all_record_options fields END
                                    { $$.setNode($1).addChild($2).addChild($3); } //MORE/NOTE inclusion of END for possible #if fix i.e. delay syntax check till semantics
    | '{' fields '}'                { $$.setNode($1).addChild($2).addChild($3); }
//   'records' used to be, still could/should be, fields
    ;

record_options
    : record_options ',' identifier { $$.setNode($2).addChild($1).addChild($3); }
    |                               { $$.setEmptyNode(); }//Need to change first & add to not account for empty
    ;

rhs
    : expr                          { $$.setNode($1); }
    | recordset                     { $$.setNode($1); }
    | type                          { $$.setNode($1); }
    ;

set
    : '[' records ']'               { $$.setNode($2).addChild($1).addChild($3); }
    | '[' ']'                       { $$.setEmptyNode().addChild($1).addChild($2); }
    ;

type
    : TYPE fields END             { $$.setNode($1).addChild($2); } //MORE/NOTE inclusion of END for possible #if fix i.e. delay syntax check till semantics
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
