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
%option case-insensitive
%option bison-bridge
%option yylineno
%option stack
%option nodefault

%{
#include "eclparser.hpp"
#include "eclgram.h"
#include <iostream>

static inline void setInt(YYSTYPE * tokenVal, char * tokenText, int lineno)
{
tokenVal->returnToken.lineNumber = lineno + 1;
tokenVal->returnToken.attributeKind = integerKind;
tokenVal->returnToken.integer = atoi(tokenText);
}

static inline void setLex(YYSTYPE * tokenVal, char * tokenText, int lineno)
{
tokenVal->returnToken.lineNumber = lineno + 1;
tokenVal->returnToken.attributeKind = lexemeKind;
strcpy(tokenVal->returnToken.lexeme, tokenText);
}

#define setInt setInt(yylval, yytext, yylineno)
#define setLex setLex(yylval, yytext, yylineno)

%}

letter        [a-z_A-Z]
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

%x COMMENT
%x SLCOMMENT

%%

"/*"                     { BEGIN(COMMENT); }
<COMMENT>[^*/]           { }
<COMMENT>"*"             { }
<COMMENT>"*/"            { BEGIN(0); }

"//"                     { BEGIN(SLCOMMENT); }
<SLCOMMENT>[^\n]+        { BEGIN(0); }

END                      { setLex; return END; }
IMPORT                   { setLex; return IMPORT; }
RECORD                   { setLex; return RECORD; }

{digit}+                 { setInt; return INTEGER; }
(\$|{letter}){alphanum}* { setLex; return ID; }
";"                      { setLex; return ';'; }
"+"                      { setLex; return PLUS; }
"-"                      { setLex; return MINUS; }
"*"                      { setLex; return MULTIPLY; }
"/"                      { setLex; return DIVIDE; }
"("                      { setLex; return '('; }
")"                      { setLex; return ')'; }
":="                     { setLex; return ASSIGN; }
\n                       { }
{blank}+                 { }

.                        { }
%%

int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s\n",msg); return 0;
}