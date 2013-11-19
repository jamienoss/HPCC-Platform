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

#include "platform.h"
#include "eclgram.h"
#include "eclgram.hpp"


EclGram::EclGram()
{
}

int EclGram::yyLex(symTree * yylval, const short * activeState)
{
    EclLex * lexer= NULL;
    return lexer->yyLex(*yylval, false, activeState);
}

extern int ecl2yyparse (EclGram* parser);

symTree * EclGram::parse()
{
    ecl2yyparse(this);
    return NULL;
}

//---------------------------------------------------------------------------------------------------------------------

EclLex::EclLex(IFileContents *)
{
}

int EclLex::yyLex(YYSTYPE & returnToken, bool lookup, const short * activeState)
{
    yyscan_t scanner;
    int ret = doyyFlex(returnToken, scanner, this, lookup, activeState);
    return 0;
}


