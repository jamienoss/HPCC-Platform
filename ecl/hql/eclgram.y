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

//Either api.pure of c++ skeleton could be used, not both (which causes an error).
//Note the c++ generation still generates a separate class for the raw processing from the HqlGram class, so whichever is
//used the productions need to use parser->... to access the context
%define api.pure
//%error-verbose
%lex-param {EclGram* parser}
%lex-param {int * yyssp}
%parse-param {EclGram* parser}
%name-prefix "ecl2yy"
//
%destructor {$$.release();} <>
//Could override destructors for all tokens e.g.,
//%destructor {} ABS
//but warnings still come out, and improvement in code is marginal.
//Only defining destructors for those productions that need them would solve it, but be open to missing items.
//Adding a comment to reference unused parameters also fails to solve it because it ignores references in comments (and a bit ugly)
//fixing bison to ignore destructor {} for need use is another alternative - but would take a long time to feed into a public build.
%{
#include "platform.h"
#include "eclgram.hpp"

inline int ecl2yylex(symTree * yylval, EclGram* parser, const short int * yyssp)
{
    return parser->yyLex(yylval, yyssp);
}

static void reportsyntaxerror(EclGram * parser, const char * s, short yystate, int token);
#define ecl2yyerror(parser, s)       reportsyntaxerror(parser, s, yystate, yychar)
#define ignoreBisonWarning(x)
#define ignoreBisonWarnings2(x,y)
#define ignoreBisonWarnings3(x,y,z)

%}

//=========================================== tokens ====================================

%token
/* remember to add any new tokens to the error reporter and lexer too! */
/* If they clash with other #defines etc then use TOK_ as a prefix */

  ID

  /* add new token before this! */
  YY_LAST_TOKEN

%left LOWEST_PRECEDENCE

%left '.'
%left '('
%left '['

%left HIGHEST_PRECEDENCE

%%

//================================== begin of syntax section ==========================

eclQuery
    : ID
    ;

%%

static void reportsyntaxerror(EclGram * parser, const char * s, short yystate, int token)
{
//MORE
}
