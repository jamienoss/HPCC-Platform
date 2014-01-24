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


class AnalyserPD;

#define YYSTYPE AnalyserPD

#define YY_DECL extern int ecl3yylex(YYSTYPE * yylval_param, AnalyserParser * parser, yyscan_t yyscanner)

 //--------------------------------------------------------------------------------
static inline void setId(YYSTYPE * yylval, const char * tokenText, unsigned txtLen, int lineno, AnalyserLexer & lexer, unsigned _tokenKind)
{
   std::cout << tokenText << "\n";


    yylval->tokenKind = _tokenKind;
    yylval->setEclLocations(lineno, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    yylval->name = createIdAtom(tokenText, txtLen);
    lexer.updatePos(txtLen);
}
 //--------------------------------------------------------------------------------
static inline void setLex(YYSTYPE * yylval, unsigned tokenKind, unsigned txtLen, int lineno, AnalyserLexer & lexer)
{
    std::cout << (char)tokenKind << "\n";


    yylval->tokenKind = tokenKind;
    yylval->setEclLocations(lineno+1, lexer.yyColumn, lexer.yyPosition, lexer.sourcePath);
    lexer.updatePos(txtLen);
}

#define setLex(x) setLex(yylval, x, yyleng, yylineno, parser->getLexer())
#define setTerminal setId(yylval, yytext, yyleng, yylineno, parser->getLexer(), TERMINAL)
#define setNonTerminal setId(yylval, yytext, yyleng, yylineno, parser->getLexer(), NONTERMINAL)
#define setProduction setId(yylval, parser->getLexer().productionText.str() , yyleng, parser->getLexer().productionLineNo, parser->getLexer(), PRODUCTION)

#define updatepos parser->getLexer().updatePos(yyleng)
#define resetpos parser->getLexer().resetPos()


static inline StringBuffer & productionLexeme(char * text, AnalyserLexer & lexer)
{
    return lexer.productionText.append(text);
}
#define appendProd(text) productionLexeme(text, parser->getLexer())

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

"%%"                     { updatepos; BEGIN(BISON); }
<BISON>"%%"              { updatepos; BEGIN(0); }

<BISON>"/*"              { updatepos; BEGIN(COMMENT); }
<COMMENT>[^*\n]+         { updatepos; }
<COMMENT>\n              { resetpos; }
<COMMENT>"*"             { updatepos; }
<COMMENT>"*/"            { updatepos; BEGIN(BISON); }

<BISON>"//"              { updatepos; BEGIN(SLCOMMENT); }
<SLCOMMENT>[^\n]+        { updatepos; BEGIN(BISON); }

<BISON>#?"'"?"_"{0,2}{upper_alphanum}*"'"? { ;
                         setTerminal; return TERMINAL;
                         }

<BISON>"_"{0,2}{alphanum}* {
                         setNonTerminal; return NONTERMINAL;
                         }


<BISON>"'"."'"           { setTerminal; return TERMINAL; }

<BISON>"\{"              { updatepos; BEGIN(PRODUCTION); parser->getLexer().productionLineNo = yylineno; }
<PRODUCTION>"\{"         { updatepos; parser->getLexer().nestCounter++; appendProd(yytext).append("\\n"); }
<PRODUCTION>[^\t }\n\"]+ { updatepos; appendProd(yytext); }
<PRODUCTION>[ \t]+       { updatepos; }
<PRODUCTION>\"           { updatepos; appendProd("\\\""); }
<PRODUCTION>\n           { updatepos; appendProd("\\n"); }
<PRODUCTION>"\}"         {
                            updatepos;
                            if(!parser->getLexer().nestCounter)
                            {
                                setProduction;
                                parser->getLexer().productionText.clear();
                                parser->getLexer().productionLineNo = 0; // helpful for debugging this code.
                                BEGIN(BISON);
                                return CODE;
                            }
                            else
                            {
                                parser->getLexer().nestCounter--;
                                appendProd(yytext);
                            }
                          }


<BISON>"%prec"            { setLex(PREC); return PREC; }
<BISON>":"                { setLex(':'); return ':'; }
<BISON>","                { setLex(','); return ','; }
<BISON>";"                { setLex(';'); return ';'; }
<BISON>"|"                { setLex('|'); return '|'; }

<INITIAL,BISON>\n         { resetpos; }
<INITIAL,BISON>{blank}+   { updatepos; }

.                         { updatepos; std::cout << "here" << yytext[0];/*setLex(yytext[0]); return (yytext[0]);*/ }
%%

 int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s \n",msg); return 0;
 }
