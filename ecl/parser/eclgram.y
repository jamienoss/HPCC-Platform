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
    BOOLEAN
    CHARACTER
    DECIMAL
    DIR
    DOTDOT ".."
    END
    FLOAT
    FROM
    ID
    IMPORT
    INTEGER
    REAL
    RECORD
    TYPE
    UPDIR ".^"

    _EOF_ 0 "End of File"
    YY_LAST_TOKEN

%left ';' ',' '.'
%left '+' '*' '/'
%left UPDIR


%right '-'
%right '='


// When writing actions $$.first($n) must be used before $$.add($n)


%%
//================================== begin of syntax section ==========================

code
    : eclQuery                      { parser->setRoot($1.getNode()); }
    ;

eclQuery
    : eclQuery ';' eclQuery _EOF_   { $$.first($2).add($1).add($3); }
    | eclQuery ';'                  { $$.first($2).add($1); }
    | line_of_code                  { $$.first($1); }
    | ';' line_of_code              { $$.first($2); }
    ;

line_of_code
    : expr                          { $$.first($1); }
    | IMPORT imports                { $$.first($1).add($2); }
    | assignment                    { $$.first($1); }
    ;

//-----------Listed Alphabetical from here on in------------------------------------------

assignment
    : lhs ASSIGN rhs                { $$.first($2).add($1).add($3); } /*should this be an expr???*/
    ;

constant
    : BOOLEAN                       { $$.first($1); }
    | CHARACTER                     { $$.first($1); }
    | DECIMAL                       { $$.first($1); }
    | FLOAT                         { $$.first($1); }
    | INTEGER                       { $$.first($1); }
    ;

expr
    : '+' expr                      { $$.first($1).add($2); }
    | '-' expr                      { $$.first($1).add($2); }
    | expr '+' expr                 { $$.first($2).add($1).add($3); }
    | expr '-' expr                 { $$.first($2).add($1).add($3); }
    | expr '*' expr                 { $$.first($2).add($1).add($3); }
    | expr '/' expr                 { $$.first($2).add($1).add($3); }
    | expr '=' expr                 { $$.first($2).add($1).add($3); }
    | constant                      { $$.first($1); }
    | set                           { $$.first($1); }
    | id_list                           { $$.first($1); } /* ugly, re-think, especially since rhs can be an expr!!! */
    | '(' expr ')'                  { $$.first($2); } /*might want to re-think discarding parens - I don't think so!*/
    | '(' ')'                       { $$.first($1).add($2); }
    ;

field
    : id_list                           { $$.first($1); }
    | assignment                    { $$.first($1); }
 //   | identifier                    { $$.first($1); }
    ;

fields
    : fields field ';'              { $$.first($1).add($2); }
    | field ';'                     { $$.first(',', $1.queryNodePosition()).add($1); }
    ;

functions
    : ID '(' parameters ')'         { $$.first($1).add($2).add($3).add($4); }
    | ID '(' ')'                    { $$.first($1).add($2).add($3); }
    | ID '{' parameters '}'         { $$.first($1).add($2).add($3).add($4); }
    | ID '[' index_range ']'        { $$.first($1).add($2).add($3).add($4); }
    ;

identifier
    : identifier '.' identifier     { $$.first($2).add($1).add($3); } //Might want to make '.' abstract
    | functions                     { $$.first($1); }
    | ID                            { $$.first($1); }
    ;

imports
    : module_list                   { $$.first($1); }
    | module_symbols AS ID          { $$.first($2).add($1).add($3); }
    | module_list FROM  module_from { $$.first($2).add($1).add($3); }
    ;

id_list
    : id_list identifier            { $$.first($1).add($2); }  /*This needs further thought inc. whether to swap order of $1 & $2*/
    | identifier                    { $$.first($1); }
    ;

index
    : INTEGER                       { $$.first($1); }
    | identifier                    { $$.first($1); }//maybe reduce to just ID
    ;

index_range
    : index_range range_op index    { $$.first($2).add($1).add($3); }
    | index_range range_op          { $$.first($2).add($1); }
    | range_op index                { $$.first($1).add($2); }
    | index                         { $$.first($1); }
    ;

lhs
    : id_list                       { $$.first($1); }
    ;

module_from
    : identifier                    { $$.first($1); }
    | DIR                           { $$.first($1); }
    ;

module_list
    : module_list ',' module_symbols
                                    { $$.first($1).add($3); }
    | module_symbols                { $$.first(',', $1.queryNodePosition()).add($1); }
    ;

module_symbols
//    : module_symbols '.' identifier  { $$.first($1).add($2).add($3); }
    : module_symbols UPDIR module_symbols
                                    { $$.first($2).add($1).add($3); }
    | module_symbols UPDIR          { $$.first($2).add($1); } //MORE might need to consider strings and not just char tokens
    | identifier                    { $$.first($1); }
    | '$'                           { $$.first($1); }
//    | ID                            { $$.first($1); }
    ;

parameter
    : expr                          { $$.first($1); }
    | ','                           { $$.first($1); }
    | ',' assignment                { $$.first($2); } /* not obvious why you'd want to shape the ST like this, i.e. miss out the ','*/
    | assignment                    { $$.first($1); }
    ;

parameters
    : parameters ',' parameter      { $$.first($1).add($2).add($3); }
    | parameters ','                { $$.first($1).add($2); }
    | parameter                     { $$.first(',', $1.queryNodePosition()).add($1); } /*perhaps re-think - this creates a comma list even if only one parameter*/
    ;

range_op
    : DOTDOT                        { $$.first($1); }
    | ':'                           { $$.first($1); }//If this conflicts with existing ecl then take out
                                                     //otherwise might help, most math lang uses this.
    ;

record
    : expr                          { $$.first($1); }
    ;

records
    : records ',' record            { $$.first($1).add($2).add($3); }
    | record                        { $$.first(',', $1.queryNodePosition()).add($1); }
    ;

recordset
    : RECORD all_record_options fields END
                                    { $$.first($1).add($2).add($3).add($4); } //MORE/NOTE inclusion of END for possible #if fix i.e. delay syntax check till semantics
    | '{' fields '}'                { $$.first($1).add($2).add($3); }
//   'records' used to be, still could/should be, fields
    ;

all_record_options
    : record_options                { $$.first($1); }
    | '(' expr ')'                  { $$.first($1).add($2).add($3); }//Could add '(' to list and "( )" as the parent node.
    ;

record_options
    : record_options ',' identifier { $$.first($2).add($1).add($3); }
    |                               { }//Need to change first & add to not account for empty
    ;

rhs
    : expr                          { $$.first($1); }
    | recordset                     { $$.first($1); }
    | type                          { $$.first($1); }
    ;

set
    : '[' records ']'               { $$.first($2).add($1).add($3); }
    | '[' ']'                       { $$.first().add($1).add($2); }
    ;

type
    : TYPE fields END             { $$.first($1).add($2).add($3); } //MORE/NOTE inclusion of END for possible #if fix i.e. delay syntax check till semantics
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
    else if (tok > 0 && tok < 256)
        target.append((char)tok);
    else if (tok > 258)
    {
        StringBuffer tokenName = yytname[tok-258+3];
        tokenName.replaceString("\"", "");
        target.append(tokenName.str()); //NOTE:+3 to buffer from '$end', '$error', and '$undefined' that Bison prefix is the list - yytname -with.
                                          //-258 rather than 256 as Bison's 1st token starts at 258.
    }
}
