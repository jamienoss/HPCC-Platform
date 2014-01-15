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

class IFile;

//----------------------------------EclParser--------------------------------------------------------------------
AnalyserParser::AnalyserParser(IFileContents * queryContents)
{
    init(queryContents);
}

AnalyserParser::~AnalyserParser()
{
	if (lexer)
		delete lexer;
    if (ast)
        delete ast;
}

int AnalyserParser::parse()
{
	return lexer->parse(this);
}

void AnalyserParser::init(IFileContents * queryContents)
{
    lexer = new AnalyserLexer(queryContents);
    ast = ast->createASyntaxTree();
}

void AnalyserParser::setRoot(ASyntaxTree * node)
{
	ast = node;
	node = NULL;
}

bool AnalyserParser::printAST()
{
	return ast->printTree();
}

ASyntaxTree * AnalyserParser::releaseAST()
{
	ASyntaxTree * temp = ast;
	ast = NULL;
	return temp;
}

void AnalyserParser::analyseGrammar()
{
    std::vector <std::string> terminalSymbols;
    createSymbolList(ast, terminalSymbols, nonTerminalDefKind);
    //printStringVector(terminalSymbols);

    ast->setSymbolList(terminalSymbols);
    ast->printSymbolList();


    ast->printTree();



}

void AnalyserParser::createSymbolList(ASyntaxTree *  tree, std::vector <std::string> & symbolList, symbolKind kind)
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
AnalyserLexer::AnalyserLexer(IFileContents * queryContents)
{
    init(queryContents);
}

AnalyserLexer::~AnalyserLexer()
{
    ecl3yylex_destroy(scanner);
    scanner = NULL;
    delete[] yyBuffer;
}

void AnalyserLexer::init(IFileContents * _text)
{
    fileIn.set(_text);
    size32_t len = _text->length();
    yyBuffer = new char[len+2]; // Include room for \0 and another \0 that we write beyond the end null while parsing
    memcpy(yyBuffer, fileIn->getText(), len);
    yyBuffer[len] = '\0';
    yyBuffer[len+1] = '\0';

    if (ecl3yylex_init(&scanner) != 0)
        std::cout << "uh-oh\n";
    ecl3yy_scan_buffer(yyBuffer, len+2, scanner);
}

int AnalyserLexer::parse(AnalyserParser * parser)
{
     return ecl3yyparse(parser, scanner);
}
