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

class IFile;

//----------------------------------EclParser--------------------------------------------------------------------
EclParser::EclParser(IFileContents * queryContents)
{
    init(queryContents);
}

EclParser::~EclParser()
{
	if (lexer)
		delete lexer;
    if (ast)
        delete ast;
}

int EclParser::parse()
{
	return lexer->parse(this);
}

void EclParser::init(IFileContents * queryContents)
{
    lexer = new EclLexer(queryContents);
    ast = ast->createSyntaxTree();
}

void EclParser::setRoot(SyntaxTree * node)
{
	ast = node;
	node = NULL;
}

bool EclParser::printAST()
{
	return ast->printTree();
}

SyntaxTree * EclParser::releaseAST()
{
	SyntaxTree * temp = ast;
	ast = NULL;
	return temp;
}

void printStringVector(std::vector <std::string> vector);

void EclParser::analyseGrammar(SyntaxTree * tree)
{
    std::vector <std::string> terminalSymbols;
    createSymbolList(tree, terminalSymbols);
    printStringVector(terminalSymbols);




}

void EclParser::createSymbolList(SyntaxTree *  tree, std::vector <std::string> & symbolList)
{
    tree->extractSymbols(symbolList);

}

void printStringVector(std::vector <std::string> vector)
{
    unsigned n = vector.size();
    for (unsigned i = 0; i < n; ++i)
    {
        std::cout << vector[i] << "\n";
    }
}

//----------------------------------EclLexer--------------------------------------------------------------------
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

void EclLexer::init(IFileContents * _text)
{
    text.set(_text);
    size32_t len = _text->length();
    yyBuffer = new char[len+2]; // Include room for \0 and another \0 that we write beyond the end null while parsing
    memcpy(yyBuffer, text->getText(), len);
    yyBuffer[len] = '\0';
    yyBuffer[len+1] = '\0';

    if (ecl2yylex_init(&scanner) != 0)
        std::cout << "uh-oh\n";
    ecl2yy_scan_buffer(yyBuffer, len+2, scanner);

    //std::cout << _text->queryFile()->queryFilename() << "\n";
    //std::cout << _text->querySourcePath()<< "\n";

}

int EclLexer::parse(EclParser * parser)
{
     return ecl2yyparse(parser, scanner);
}
