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
    PARSE_ID
    REAL
    RECORD
    SERVICE
    STRING_CONST
    TYPE

    T_LEFT
    T_OUTER
    T_UNSIGNED

    _EOF_ 0 "End of File"
    YY_LAST_TOKEN

//The grammar contains some shift reduce conflicts because expressions can be followed by other expressions.
//The examples are:
//   abc(123)      vs     abc     (123)   // id then brackets
//   abc[1]        vs     abc     [3]     // id then set
//   abc + def     vs     abc     +def    // id then unary plus
//
// All these want to choose the first form (shift) rather than reduce.
//
//However there is also the following
//  ,abc def       vs    ,abc      def    // a multi-word attribute on a record vs a single word attribute followed by the start of a definition.
//Here we want to choose the second form (reduce)
//The NEXT_EXPR_PRECEDENCE is before after the tokens (lower priority) that should be shifted, but before those that should be reduced.

%left LOWEST_PRECEDENCE
%left NEXT_EXPR_PRECEDENCE

%left ':' ';' ',' '.'

%left '+' '-'
%left '*' '/'
%left NE EQ LE GE LT GT

%left '('
%left '['
%left '{'

%right '='

//The following is a list of identifiers which can occur as the second (or more) identifiers in a compound-id
%left T_OUTER

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
    : record_options                %prec NEXT_EXPR_PRECEDENCE     // Ensure that '(' gets shifted instead of causing a s/r error
                                    { }
    | '(' expr ')' record_options  %prec NEXT_EXPR_PRECEDENCE { }//Could add '(' to list and "( )" as the parent node.
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
    | expr '(' parameters ')'       // function call [ or definition ]
    | expr '.' anyID                %prec NEXT_EXPR_PRECEDENCE
    | expr '[' index_range ']'
    | compound_id                   %prec NEXT_EXPR_PRECEDENCE     // Ensure that '(' gets shifted instead of causing a s/r error
    | '(' expr ')'                  { } /*might want to re-think discarding parens - I don't think so!*/
    | record_definition             { }
    | module_definition             { }
    | service_definition            { }
    ;

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
    : fields ';' field              { }
    | fields ',' field              { }
    | field              { }
    ;

function
    : anyID '(' parameters ')'         { }
    ;

compound_id
    : anyID                            %prec NEXT_EXPR_PRECEDENCE     // Ensure that '(' gets shifted instead of causing a s/r error
// A general rule for compound_id of
//    compound_id : compound_id ID
// causes greif.  That's because if attributes can be represented by compound ids, it would be impossible to
// distinguish between
//  record,x y      -- a multi id identifier
//  record,x
//     y            -- a single id indentifier, followed by a field.
// Therefore I think we will need to special case any combination of identifiers that can be used as part of a compound id
//   VIRTUAL RECORD, RULE TYPE, EXPORT|SHARED VIRTUAL
//   SORT KEYED, SORT ALL,ANY DATASET, VIRTUAL DATASET, LEFT|RIGHT|FULL OUTER, ONLY [LEFT|RIGHT|FULL], MANY LOOKUP,
//   PARTITION RIGHT, ASSERT SORTED, SCAN ALL, MANY [BEST|MIN|MAX], NOT MATCHED [ONLY], WHOLE RECORD
// note, we need to special case "NOT MATCHED" - we can't special case NOT ID since 99% of the time it will be wrong.
// Although for some - like NOT we could always match the expression, and then invert it.
//   Many of these would be better as OUTER(LEFT|RIGHT|FULL)
// Known prefixes:
//   SET OF, UNSIGNED, PACKED< BIG, LITTLE, ASCII, EBCDIC
//   LINKCOUNTED, STREAMED, EMBEDDED, _ARRAY_

//For all compound ids specified here, the second id must be included in the precedence list at the start of the file,
//with a higher precedence than NEXT_EXPR_PRECEDENCE, and all leading ids must have a precedence in anyID
    | T_LEFT T_OUTER
    ;

anyID
    : ID
    | T_LEFT                        %prec NEXT_EXPR_PRECEDENCE
    | T_OUTER
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
    | module_path AS anyID          { }
    | module_list FROM module_path  { }
    ;

index_range
    : expr range_op expr            { }
    | expr range_op                 { }
    | range_op expr                 { }
    | expr                          { }
    ;

lhs
    : expr                         %prec NEXT_EXPR_PRECEDENCE
    | lhs expr                     %prec NEXT_EXPR_PRECEDENCE // if +({ follows then shift instead of reducing
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
    : anyID                         { }
    ;

parameter
    :                               { }
    | expr                          { }
    | anyID ASSIGN expr             { }
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
    ;

record_options
    : record_options ',' expr { }
    |                               %prec NEXT_EXPR_PRECEDENCE
                                    { }//Need to change first & add to not account for empty
    ;

rhs
    : expr                          { }
    | type                          { }
    ;

set
    : '[' expr_list ']'             { }
    | '[' ']'                       { }
    ;
//---------------------------------------------
keyword
    : anyID EQ expr                    { }
    | anyID '(' parameters ')'         { }
    | anyID                            { }
    ;


keywords
    : keywords ',' keyword          { }
    | keyword                       { }
    ;

service_attribute
    : function ':' keywords         { }
    |                               { }
    ;

service_attributes
    : service_attributes ';' service_attribute
                                    { }
    | service_attribute             { }
    ;

service_default_keywords
    : ':' keywords                  { }
    |                               { }
    ;

service_definition
    : SERVICE service_default_keywords service_attributes END
                                    { }
    ;
//---------------------------------------------------
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
