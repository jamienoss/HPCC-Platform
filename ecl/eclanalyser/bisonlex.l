/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */
%option 8bit never-interactive
%option prefix="ecl3yy"
%option nounistd
%option reentrant
%option noyywrap
%option bison-bridge
%option yylineno
%option stack
%{
#include "analyserparser.hpp"
#include "bisongram.h"
#include <iostream>
#include <cstring>

  /*YYLTYPE  yylloc;

#define YY_USER_ACTION {yylloc.first_line = yylineno; } */

static void setLexeme(YYSTYPE * tokenVal, const char * tokenText)
{
    unsigned txtLen = strlen(tokenText)+1;
    tokenVal->returnToken.lexeme = new char [txtLen];
    memcpy(tokenVal->returnToken.lexeme, tokenText, txtLen);

    //std::cout << tokenText << "\n";

}

static void setInt(YYSTYPE * tokenVal, char * tokenText, int lineno)
{
    tokenVal->returnToken.lineNumber = lineno + 1;
    tokenVal->returnToken.attributeKind = integerKind;
    tokenVal->returnToken.integer = atoi(tokenText);
}

static void setLex(YYSTYPE * tokenVal, char * tokenText, int lineno)
{
    tokenVal->returnToken.lineNumber = lineno + 1;
    tokenVal->returnToken.attributeKind = lexemeKind;
    setLexeme(tokenVal, tokenText);
}

static void setNonTerminal(YYSTYPE * tokenVal, char * tokenText, int lineno)
{
    tokenVal->returnToken.lineNumber = lineno + 1;
    tokenVal->returnToken.attributeKind = nonTerminalKind;
    setLexeme(tokenVal, tokenText);
}

static void setTerminal(YYSTYPE * tokenVal, char * tokenText, int lineno)
{
    tokenVal->returnToken.lineNumber = lineno + 1;
    tokenVal->returnToken.attributeKind = terminalKind;
    setLexeme(tokenVal, tokenText);
}

static void setProduction(YYSTYPE * tokenVal, const char * tokenText, int lineno)
{
    tokenVal->returnToken.lineNumber = lineno + 1;
    tokenVal->returnToken.attributeKind = productionKind;
    setLexeme(tokenVal, tokenText);

    //std::cout <<  " : " << lineno << "\n";
}



#define setInt setInt(yylval, yytext, yylineno)
#define setLex setLex(yylval, yytext, yylineno)

//global code used for returning productions within grammar file
unsigned nestCounter = 0;
unsigned productionLineNo = 0;
std::string productionText;

static inline void productionLexeme(const char * tokenText)
{
    productionText.append(tokenText);
    productionText.append("\\n");
}


%}

digit         [0-9]
bindigit      [0-1]
hexdigit      [a-fA-F0-9]
alphanum      [a-z_A-Z$_0-9]
alphanumcolon [a-z_A-Z$_0-9:@]
blank         [ \t\r]

lower_letter [a-z]
upper_letter [A-Z]
upper_alphanum [A-Z$_0-9]

%x COMMENT
%x SLCOMMENT
%x PRODUCTION
%x BISON

%%

"%%"                     { BEGIN(BISON); }
<BISON>"%%"              { BEGIN(0); }

<BISON>"/*"              { BEGIN(COMMENT); }
<COMMENT>[^*\n]+         { }
<COMMENT>\n              { }
<COMMENT>"*"             { }
<COMMENT>"*/"            { BEGIN(BISON); }

<BISON>"//"              { BEGIN(SLCOMMENT); }
<SLCOMMENT>[^\n]+        { BEGIN(BISON); }

<BISON>#?"'"?"_"{0,2}{upper_alphanum}*"'"? { ;
                         setTerminal(yylval, yytext, yylineno); return TERMINAL;
                         }

<BISON>"_"{0,2}{alphanum}* {
                         setNonTerminal(yylval, yytext, yylineno); return NONTERMINAL;
                         }


<BISON>"'"."'"           { setTerminal(yylval, yytext, yylineno); return TERMINAL; }

<BISON>"\{"              { BEGIN(PRODUCTION); productionLineNo = yylineno; }
<PRODUCTION>"\{"         { nestCounter++; productionLexeme(yytext); }
<PRODUCTION>[^\t }\n\"]+ { productionText.append(yytext); }
<PRODUCTION>[ \t]+       { }
<PRODUCTION>\"           { productionText.append("\\\""); }
<PRODUCTION>\n           { productionText.append("\\n"); }
<PRODUCTION>"\}"         {
                            if(!nestCounter)
                            {
                                setProduction(yylval, productionText.data(), productionLineNo);
                                productionText.clear();
                                productionLineNo = 0; // helpful for debugging this code.
                                BEGIN(BISON);
                                return CODE;
                            }
                            else
                            {
                                nestCounter--;
                                productionLexeme(yytext);
                            }
                          }


  /*    "\{"(.|\n)+/"\}"               { setProduction(yylval, yytext, yylineno); return CODE; }   */

<BISON>"%prec"            { setLex; return PREC; }
<BISON>":"                { setLex; return ':'; }
<BISON>","                { setLex; return ','; }
<BISON>";"                { setLex; return ';'; }
<BISON>"|"                { setLex; return '|'; }

<INITIAL,BISON>\n         { }
<INITIAL,BISON>{blank}+   { }

.                         { }
%%

  /*int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s \n",msg); return 0;
 } */
