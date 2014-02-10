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
AnalyserParser::AnalyserParser(IFileContents * queryContents, IErrorReceiver * errs)
{
    lexer = new AnalyserLexer(queryContents);
    errorHandler = LINK(errs);
}

void AnalyserParser::analyseGrammar()
{
    std::vector <std::string> terminalSymbols;
    createSymbolList(ast.get(), terminalSymbols, 300);
    //printStringVector(terminalSymbols);

    //ast->setSymbolList(terminalSymbols);
    //ast->printSymbolList();


    ast->printTree();
}

void AnalyserParser::createSymbolList(ISyntaxTree *  tree, std::vector <std::string> & symbolList, TokenKind kind)
{
    tree->extractSymbols(symbolList, kind);
}

void printStringVector(std::vector <std::string> vector)
{
    unsigned n = vector.size();
    for (unsigned i = 0; i < n; ++i)
    {
        std::cout << vector[i] << "\n";
    }
}
//----------------------------------AnalyserLexer--------------------------------------------------------------------
AnalyserLexer::AnalyserLexer(IFileContents * queryContents) : EclLexer(queryContents)
{
    nestCounter = 0;
    productionLineNo = 0;
}
