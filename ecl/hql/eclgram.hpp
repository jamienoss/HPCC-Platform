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
#ifndef ECLGRAM_HPP
#define ECLGRAM_HPP

#include "platform.h"
#include "jhash.hpp"
#include "hqlexpr.hpp"  // MORE: Split IFileContents out of this file

struct symTree
{
public:
    union 
    {
        IAtom * name;
    };

public:
    symTree & release() 
    { 
        return *this;
    }
};

#define YYSTYPE symTree

class EclGram;
class EclLex;


extern int ecl2yyparse(EclGram * parser);
class HQL_API EclGram
{
    friend class EclLex;
    friend int ecl2yyparse(EclGram * parser);

public:
    EclGram();

    int yyLex(symTree * yylval, const short * activeState);
    symTree * parse();
};



#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

class HQL_API EclLex
{
public:
    EclLex(IFileContents * _text);

    static int doyyFlex(YYSTYPE & returnToken, yyscan_t yyscanner, EclLex * lexer, bool lookup, const short * activeState);

    int yyLex(YYSTYPE & returnToken, bool lookup, const short * activeState);    /* lexical analyzer */
};

//Next stages:
// Separate or same dll? Up to you.
// Fill in EclLex with code from HqlLex
// Go through and make sure class names, file names etc. are as meaningful as possible.
// rename symTree
// modify EclCC::processSingleQuery to conditionally call the new parser, and return xml/dot/json
// get a very simple query - a single id working.
// Add constants and '+' operator, brackets (1) (1+2)
// Flesh out symTree - members, position, handling constants, types, generating xml


#endif
