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
%option prefix="ecl2yy"
%option nounistd
%option reentrant
%option noyywrap
%option bison-bridge
%option yylineno
%option stack
%{
#include "eclparser.hpp"
#include "eclgram.h"
#include <iostream>
#include <cstring>

static void setLexeme(YYSTYPE * tokenVal, char * tokenText)
{
    unsigned txtLen = strlen(tokenText)+1;
    tokenVal->returnToken.lexeme = new char [txtLen];
    memcpy(tokenVal->returnToken.lexeme, tokenText, txtLen);
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

static void setProduction(YYSTYPE * tokenVal, char * tokenText, int lineno)
{
    tokenVal->returnToken.lineNumber = lineno + 1;
    tokenVal->returnToken.attributeKind = productionKind;
    setLexeme(tokenVal, tokenText);
}



static inline void pText(char * tokenText)
{
    std::cout << tokenText << "\n";
}
#define pText pText(yytext)

#define setInt setInt(yylval, yytext, yylineno)
#define setLex setLex(yylval, yytext, yylineno)

%}

digit         [0-9]
bindigit      [0-1]
hexdigit      [a-fA-F0-9]
alphanum      [a-z_A-Z$_0-9]
alphanumcolon [a-z_A-Z$_0-9:@]
blank         [ \t\r]
slash         [/]
star          [*]
percent       [%]
lcurly        [\{]
rcurly        [\}]
dot           [\.]
hexpairs      ({hexdigit}{hexdigit})+
err_hexpairs  {hexdigit}({hexdigit}{hexdigit})*
xpathchars    [a-z_A-Z$_0-9:/\[\]@=!]
xpathseq      ([^}\r\n])+

lower_letter [a-z]
upper_letter [A-Z]
upper_alphanum [A-Z$_0-9]

%x COMMENT
%x SLCOMMENT

%%

"/*"                     { BEGIN(COMMENT); }
<COMMENT>[^*/]           { }
<COMMENT>"*"             { }
<COMMENT>"*/"            { BEGIN(0); }

"//"                     { BEGIN(SLCOMMENT); }
<SLCOMMENT>[^\n]+        { BEGIN(0); }


"_"{0,2}{lower_letter}{alphanum}* {
                         setNonTerminal(yylval, yytext, yylineno); return NONTERMINAL;
                       }
#?"'"?"_"{0,2}{upper_letter}{upper_alphanum}*"'"? { ;
                         setTerminal(yylval, yytext, yylineno); return TERMINAL;
                       }

"'"."'"                  { setTerminal(yylval, yytext, yylineno); return TERMINAL; }
.*"%%"                   { return STUFF; }
"%%".*                   { return STUFF; }

\{[\s\S]*\}                   { setProduction(yylval, yytext, yylineno); return CODE; }

"%prec"                  { setLex; return PREC; }
":"                      { setLex; return ':'; }
","                      { setLex; return ','; }
";"                      { setLex; return ';'; }
"|"                      { setLex; return '|'; }
"{"                      { setLex; return '{'; }
"}"                      { setLex; return '}'; }

\n                       { }
{blank}+                 { }

.                        { }
%%

int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s \n",msg); return 0;
}