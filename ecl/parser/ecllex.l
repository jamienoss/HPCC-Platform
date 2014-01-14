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
#include <cstring>

#define YY_DECL extern int ecl2yylex(YYSTYPE * yylval_param, EclParser * parser, yyscan_t yyscanner)

  //--------------------------------------------------------------------------------
static inline void setInt(YYSTYPE * yylval, char * tokenText, unsigned txtLen, int lineno, EclLexer & lexer)
{
yylval->returnToken.attributeKind = integerKind;
yylval->returnToken.integer = atoi(tokenText);

lexer.updatePos(txtLen);
yylval->returnToken.setEclLocations(lineno+1, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
}
  //--------------------------------------------------------------------------------
static inline void setLex(YYSTYPE * yylval, char * tokenText, unsigned txtLen, int lineno, EclLexer & lexer)
{
yylval->returnToken.attributeKind = lexemeKind;
lexer.updatePos(txtLen);
yylval->returnToken.setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);

yylval->returnToken.lexeme = new char [++txtLen];
memcpy(yylval->returnToken.lexeme, tokenText, txtLen);

lexer.updatePos(txtLen);
yylval->returnToken.setEclLocations(lineno+1, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
}
  //--------------------------------------------------------------------------------

#define setInt setInt(yylval, yytext, yyleng, yylineno, parser->getLexer())
#define setLex setLex(yylval, yytext, yyleng, yylineno, parser->getLexer())
#define updatepos parser->getLexer().updatePos(yyleng)
#define resetpos parser->getLexer().resetPos()

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

"/*"                     { updatepos; BEGIN(COMMENT); }
<COMMENT>[^*\n]+         { updatepos; }
<COMMENT>\n              { resetpos; }
<COMMENT>"*"             { updatepos; }
<COMMENT>"*/"            { updatepos; BEGIN(0); }

"//"                     { updatepos; BEGIN(SLCOMMENT); }
<SLCOMMENT>[^\n]+        { updatepos; BEGIN(0); }

AS                       { setLex; return AS; }
END                      { setLex; return END; }
FROM                     { setLex; return FROM; }
IMPORT                   { setLex; return IMPORT; }
RECORD                   { setLex; return RECORD; }

{digit}+                 { setInt; return UNSIGNED; } /* complete for UNSIGNED[n]*/
(\$|{letter}){alphanum}* { setLex; return ID; }
","                      { setLex; return ','; }
";"                      { setLex; return ';'; }
"+"                      { setLex; return PLUS; }
"-"                      { setLex; return MINUS; }
"*"                      { setLex; return MULTIPLY; }
"/"                      { setLex; return DIVIDE; }
"("                      { setLex; return '('; }
")"                      { setLex; return ')'; }
":="                     { setLex; return ASSIGN; }
\n                       { resetpos; }
{blank}+                 { updatepos; }

.                        { updatepos; }
%%

int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s\n",msg); return 0;
}