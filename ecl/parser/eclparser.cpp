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
#include "jfile.hpp"
#include "eclparser.hpp"
#include <iostream>
#include "eclgram.h"
#include "ecllex.hpp"
#include "syntaxtree.hpp"

#include <cstring>
#include <iostream>

class IFile;

//----------------------------------EclParser--------------------------------------------------------------------
EclParser::EclParser() {}
EclParser::EclParser(IFileContents * queryContents, IErrorReceiver * errs)
{
    lexer = new EclLexer(queryContents);
    errorHandler = LINK(errs);
}

EclParser::~EclParser()
{
    if (lexer)
        delete lexer;
    //if(ast)
        //ast->Release();
    //if(errorHandler)
      //  errorHandler->Release();
}

int EclParser::parse()
{
    return ecl2yyparse(this, lexer->queryScanner());
}

void EclParser::setRoot(ISyntaxTree * node)
{
    ast.setown(node);
}

void EclParser::printAST()
{
    ast->printTree();
}

ISyntaxTree * EclParser::queryAST()
{
    return ast;
}

EclLexer & EclParser::queryLexer()
{
    return *lexer;
}

void EclParser::reportError(int errNo, const char *msg, const char * _sourcePath, int _lineno, int _column, int _position)
{
    errorHandler->reportError(errNo, msg, _sourcePath, _lineno, _column, _position);
}

//----------------------------------EclLexer--------------------------------------------------------------------
EclLexer::EclLexer() {}
EclLexer::EclLexer(IFileContents * queryContents)
{
    init(queryContents);
}

EclLexer::~EclLexer()
{
    ecl2yylex_destroy(scanner);
    scanner = NULL;
    delete[] yyBuffer;
}

void EclLexer::init(IFileContents * queryContents)
{
    text.set(queryContents);
    size32_t len = queryContents->length();
    yyBuffer = new char[len+2]; // Include room for \0 and another \0 that we write beyond the end null while parsing
    memcpy(yyBuffer, text->getText(), len);
    yyBuffer[len] = '\0';
    yyBuffer[len+1] = '\0';

    if (ecl2yylex_init(&scanner) != 0)
        std::cout << "uh-oh\n";
    ecl2yy_scan_buffer(yyBuffer, len+2, scanner);

    yyPosition = 0;
    yyColumn = 0;
    sourcePath = queryContents->querySourcePath();

    ecl2yyset_lineno(1, scanner);
}

yyscan_t & EclLexer::queryScanner()
{
    return scanner;
}

void EclLexer::updatePos(unsigned delta)
{
    yyPosition += delta;
    yyColumn += delta;
}

void EclLexer::resetPos()
{
    //yyPosition = 0;
    yyColumn = 0;
}
