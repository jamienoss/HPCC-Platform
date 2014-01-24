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
#include "bisongram.h"
//#include "bisonlex.hpp"
#include <iostream>

//typedef class AnalyserTD YYSTYPE;
//#undef YYSTYPE_IS_DECLARED
//#define YYSTYPE_IS_DECLARED 1

#define YYSTYPE AnalyserPD

//extern int ecl3yylex(YYSTYPE * yylval_param, AnalyserParser * parser, yyscan_t yyscanner);

int yyerror(AnalyserParser * parser, yyscan_t scanner, const char *msg);
int syntaxerror(const char *msg, short yystate, YYSTYPE token);
#define ecl3yyerror(parser, scanner, msg)   syntaxerror(msg, yystate, yylval)

int syntaxerror(const char *msg, short yystate, YYSTYPE token)
{
    std::cout << msg <<  " near line "  << token.pos.lineno << \
                     ", nearcol:" <<  token.pos.column <<  "\n";
    return 0;
}

%}


//=========================================== tokens ====================================

%token
    CODE
    DOUBLE_PERCENT
    NONTERMINAL
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
    : grammar_item                  { parser->setRoot($1); }
    ;

grammar_item
    : grammar_item grammar_rule     { $$.clear($1).add($2); }
    | grammar_rule                  { $$.clear(',', $1.pos).add($1); }
    ;

grammar_rule
    : NONTERMINAL grammar_rules ';'
                                    { $1.tokenKind = 300;/*nonTerminalDefKind*/ $$.clear($1).add($2);}
    ;

grammar_rules
    : grammar_rules '|' terminal_productions
                                    { $$.clear($2).add($3).add($1); }/*
                                        /*$$ = $1;
                                        ASyntaxTree * temp = temp->createASyntaxTree($2);
                                        temp->transferChildren($3);
                                        $$->addChild(temp);
                                    }*/
    | grammar_rules '|'
                                    { $$.clear($2).add($1); }/*
                                        /*$$ = $1;
                                        ASyntaxTree * temp = temp->createASyntaxTree($2);
                                        $$->addChild(temp);
                                    }*/
    | ':' terminal_productions      { $$.clear($1).add($2); }/*
                                        /*$$ = $$->createASyntaxTree();
                                        ASyntaxTree * temp = temp->createASyntaxTree($1);
                                        temp->transferChildren($2);
                                        $$->addChild(temp);
                                    }*/
    | ':'                           { $$.clear($1); }/*
                                        /*$$ = $$->createASyntaxTree();
                                        ASyntaxTree * temp = temp->createASyntaxTree($1);
                                        $$->addChild(temp);
                                    }*/
    ;

terminal_productions
    : terminal_list                 { $$.clear($1); }
  //  | terminal_list production      { $$ = $1; $$->addChild($2); }
  //  | production                    { $$ = $$->createASyntaxTree(); $$->addChild($1); }
    ;

production
    : CODE                          { $$.clear($1); }
    ;

terminals
    : NONTERMINAL                   { $$.clear($1); }
    | TERMINAL                      { $$.clear($1); }
    | PREC                          { $$.clear($1); }
    | production                    { $$.clear($1); }
    ;

terminal_list
    : terminal_list terminals       { $$.clear($1).add($2); }
    | terminals                     { $$.clear(',', $1.pos).add($1); }
    ;

%%
