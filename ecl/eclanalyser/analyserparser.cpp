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
#include "analyserparser.hpp"
#include <iostream>
#include "bisongram.h"
#include "bisonlex.hpp"
//#include "ASyntaxTree.hpp"

#include "eclparser.hpp"

class IFile;

//----------------------------------EclParser--------------------------------------------------------------------
AnalyserParser::AnalyserParser(IFileContents * queryContents, IErrorReceiver * errs) : EclParser()
{
    lexer = new AnalyserLexer(queryContents);
    errorHandler = LINK(errs);
}

int AnalyserParser::parse()
{
    return ecl3yyparse(this, lexer->getScanner());
}

void AnalyserParser::analyseGrammar()
{
    StringBufferArray nonTerminals;
    createSymbolList(ast.get(), nonTerminals, TERMINAL);
    printSymbolList(nonTerminals);

    //ast->setSymbolList(terminalSymbols);
    //ast->printSymbolList();


    ast->printTree();
}

void AnalyserParser::createSymbolList(ISyntaxTree *  tree, StringBufferArray & symbolList, TokenKind kind)
{
    tree->extractSymbols(symbolList, kind);
}

void AnalyserParser::printSymbolList(const StringBufferArray & list)
{
    if(list.ordinality())
    {
        ForEachItemIn(i, list)
        {
            std::cout << list.item(i).str() << "\n";
        }
    }
}

AnalyserLexer & AnalyserParser::getLexer()
{
    return *lexer;
}

//----------------------------------AnalyserLexer--------------------------------------------------------------------
AnalyserLexer::AnalyserLexer(IFileContents * queryContents) : EclLexer()
{
    init(queryContents);
    nestCounter = 0;
    productionLineNo = 0;
}

AnalyserLexer::~AnalyserLexer()
{
    ecl3yylex_destroy(scanner);
    scanner = NULL;
    delete[] yyBuffer;
}

void AnalyserLexer::init(IFileContents * queryContents)
{
    text.set(queryContents);
    size32_t len = queryContents->length();
    yyBuffer = new char[len+2]; // Include room for \0 and another \0 that we write beyond the end null while parsing
    memcpy(yyBuffer, text->getText(), len);
    yyBuffer[len] = '\0';
    yyBuffer[len+1] = '\0';

    if (ecl3yylex_init(&scanner) != 0)
        std::cout << "uh-oh\n";
    ecl3yy_scan_buffer(yyBuffer, len+2, scanner);

    yyPosition = 0;
    yyColumn = 0;
    sourcePath = queryContents->querySourcePath();

    ecl3yyset_lineno(1, scanner);
}
