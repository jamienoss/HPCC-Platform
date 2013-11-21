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
#include "eclparser.hpp"
#include <iostream>
#include "ecllex.hpp"


EclParser::EclParser(IFileContents * queryContents)
{
	init(queryContents);
}

int EclParser::yyLex(syntaxTree * yylval, const short * activeState)
{
    //EclLex * lexer= NULL;
    return lexer->yyLex(*yylval, false, activeState);
}

syntaxTree * EclParser::parse()
{
    ecl2yyparse(this);
    return NULL;
}

void EclParser::init(IFileContents * queryContents)
{
	lexer = new EclLex(queryContents);
	ast = new syntaxTree();
}


//---------------------------------------------------------------------------------------------------------------------

EclLex::EclLex(IFileContents * queryContents)
{
	init(queryContents);
}

EclLex::~EclLex()
{
    ecl2yylex_destroy(scanner);
    scanner = NULL;
    delete[] yyBuffer;
}

void EclLex::init(IFileContents * _text)
{
    text.set(_text);
    size32_t len = _text->length();
    yyBuffer = new char[len+2]; // Include room for \0 and another \0 that we write beyond the end null while parsing
    memcpy(yyBuffer, text->getText(), len);
    yyBuffer[len] = '\0';
    yyBuffer[len+1] = '\0';

    if(ecl2yylex_init(&scanner) != 0)
        std::cout << "uh-oh\n";
    ecl2yy_scan_buffer(yyBuffer, len+2, scanner);

}
int EclLex::yyLex(YYSTYPE & returnToken, bool lookup, const short * activeState)
{
    //yyscan_t scanner;
    int ret = doyyFlex(returnToken, scanner, this, lookup, activeState);
    return 0;
}
