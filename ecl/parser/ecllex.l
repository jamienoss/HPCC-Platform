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

%{
#include "eclparser.hpp"
#include "eclgram.h"
#include <iostream>

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

%%
{digit}+                 { yylval->returnToken.attributeKind = integerKind; yylval->returnToken.integer = atoi(yytext); return INTEGER; }
(\$|{letter}){alphanum}* { yylval->returnToken.attributeKind = lexemeKind; strcpy(yylval->returnToken.lexeme, yytext); return ID; }
";"                      { yylval->returnToken.attributeKind = lexemeKind; strcpy(yylval->returnToken.lexeme, yytext); return ';'; }
"+"                      { yylval->returnToken.attributeKind = lexemeKind; strcpy(yylval->returnToken.lexeme, yytext); return PLUS; }
.                        { }

%%

int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s\n",msg); return 0;
}