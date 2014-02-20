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
%name-prefix "ecl3yy"
%error-verbose

%parse-param {AnalyserParser * parser}
%parse-param {yyscan_t scanner }
%lex-param {AnalyserParser * parser}
%lex-param {yyscan_t scanner}

%{
class AnalyserParser;

#include "platform.h"
#include "analyserparser.hpp"
#include "hqlerrors.hpp"
#include <iostream>

#define YYSTYPE AnalyserPD //NOTE: if you include eclgram.h this needs to be placed after this definition

extern int ecl3yylex(YYSTYPE * yylval_param, AnalyserParser * parser, yyscan_t yyscanner);

int yyerror(AnalyserParser * parser, yyscan_t scanner, const char *msg);
int syntaxerror(const char *msg, short yystate, YYSTYPE token);
#define ecl3yyerror(parser, scanner, msg)   syntaxerror(msg, yystate, yylval, parser)

int syntaxerror(const char *msg, short yystate, YYSTYPE token, AnalyserParser * parser)
{
    parser->reportError(ERR_EXPECTED, msg, parser->getLexer().sourcePath->str(), token.pos.lineno, token.pos.column, token.pos.position);
    return 0;
}

%}


//=========================================== tokens ====================================

%token
    CODE
    DOUBLE_PERCENT
    NONTERMINAL
    NONTERMINALDEF
    PREC
    STUFF
    TERMINAL

    YY_LAST_TOKEN

/*%type <treeNode>
    grammar_item
    grammar_rule
    grammar_rules
    production
    terminals
    terminal_list
    terminal_productions
*/

%left '.'
%left '{'

%%
//================================== beginning of syntax section ==========================

bison_file
    : grammar_item                  { parser->setRoot($1.getNode()); }
    ;

grammar_item
    : grammar_item grammar_rule     { $$.setNode($1).addChild($2); }
    | grammar_rule                  { $$.setNode($1); }
    ;

grammar_rule
    : NONTERMINAL grammar_rules ';'
                                    { $1.kind = NONTERMINALDEF; $$.setNode($1).addChild($2);}
    ;

grammar_rules
    : grammar_rules '|' terminal_productions
                                    { $2.addChild($2).addChild($3); $$.setEmptyNode().addChild($1).addChild($2); }
    | grammar_rules '|'
                                    { $$.setNode($1).addChild($2); }
    | ':' terminal_productions      { $$.setNode($1).addChild($2); }
    | ':'                           { $$.setNode($1); }
    ;

terminal_productions
    : terminal_list                 { $$.setNode($1); }
  //  | terminal_list production      { $$ = $1; $$->addChild($2); }
  //  | production                    { $$ = $$->createASyntaxTree(); $$->addChild($1); }
    ;

production
    : CODE                          { $$.setNode($1); }
    ;

terminals
    : NONTERMINAL                   { $$.setNode($1); }
    | TERMINAL                      { $$.setNode($1); }
    | PREC                          { $$.setNode($1); }
    | production                    { $$.setNode($1); }
    ;

terminal_list
    : terminal_list terminals       { $$.setNode($1).addChild($2); }
    | terminals                     { $$.setEmptyNode().addChild($1); }
    ;

%%

void analyserAppendParserTokenText(StringBuffer & target, unsigned tok)
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