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
#include "eclparser.hpp"
#include <iostream>
#include "eclgram.h"
#include "ecllex.hpp"
#include "syntaxtree.hpp"

//----------------------------------EclParser--------------------------------------------------------------------
EclParser::EclParser(IFileContents * queryContents)
{
    init(queryContents);
}

EclParser::~EclParser()
{
	if(lexer)
		delete lexer;
    if(ast)
        delete ast;
}

int EclParser::parse()
{
	return lexer->parse(this);
}

void EclParser::init(IFileContents * queryContents)
{
    lexer = new EclLexer(queryContents);
    ast = new SyntaxTree();
}

void EclParser::setRoot(SyntaxTree * node)
{
	ast = node;
}

SyntaxTree * EclParser::bifurcate(TokenData parent, TokenData left, TokenData right)
{
	return new SyntaxTree(parent, left, right);
}

bool EclParser::printAST()
{
	return ast->printTree();
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

    if(ecl2yylex_init(&scanner) != 0)
        std::cout << "uh-oh\n";
    ecl2yy_scan_buffer(yyBuffer, len+2, scanner);

    //std::cout << _text->queryFile()->queryFilename() << "\n";

}

int EclLexer::parse(EclParser * parser)
{
     return ecl2yyparse(parser, scanner);
}
