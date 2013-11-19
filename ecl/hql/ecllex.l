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

%{
#include "eclgram.hpp"

#include "eclgram.h"

#define YY_DECL int EclLex::doyyFlex(YYSTYPE & returnToken, yyscan_t yyscanner, EclLex * lexer, bool lookup, const short * activeState)

//declare this rather than including the header because VC++ seems to get very upset running out of heap if you do.
extern int rtlSingleUtf8ToCodepage(char * out, unsigned inlen, char const * in, char const * outcodepage);

//#define DEBUG_TOKEN 

#ifdef DEBUG_TOKEN
#define TraceReturnToken(id) PrintLog("Returning " #id " %s", name->str())
#else 
#define TraceReturnToken(id)
#endif

#ifdef FLEX_SCANNER
//#define updatepos1          lexer->updatePosition(yyleng)
#define updatepos1          
#define CUR_TOKEN_TEXT      yytext
#define CUR_TOKEN_LENGTH    yyleng
#else
#define updatepos1          lexer->updatePosition(CUR_TOKEN_LENGTH)
#define CUR_TOKEN_TEXT      lexer->yyText
#define CUR_TOKEN_LENGTH    lexer->yyLeng
#endif

#define setupdatepos        { }
//#define setupdatepos        { lexer->setTokenPosition(returnToken); updatepos1; }
extern void hex2str(char * target, const char * digits, unsigned len);

#define RETURNSYM(sym) \
        setupdatepos; return sym;


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
%x CPP
%x SLSL
%%

"/*"                { BEGIN(COMMENT); }
<COMMENT>"*/"       { 
                        BEGIN(0); 
                    }
<COMMENT>[^*\n]+    { }
<COMMENT>\n         { }
<COMMENT>"*"        { }

"*/"                { 
                    }                   

\r                  { }
\n                  { 
                    }

{blank}+            { }

"//"                { BEGIN(SLSL); }
<SLSL>\n            { BEGIN(0); }
<SLSL>[^\n]+        { }

    
(\$|{letter}){alphanum}* {
                        setupdatepos;
                        return ID;
                    }
\357\273\277        {
                    }

.                   { setupdatepos; return (CUR_TOKEN_TEXT[0]); }

%%
