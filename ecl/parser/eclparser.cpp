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
#include <typeinfo>
#include "eclgram.h"
#include "ecllex.hpp"

//----------------------------------SyntaxTree--------------------------------------------------------------------
SyntaxTree::SyntaxTree()
{
	attributes.lineNumber = 0;
    left = NULL;
    right = NULL;
}

SyntaxTree::SyntaxTree(TokenData node)
{
	attributes = node;
	left = NULL;
	right = NULL;
}

SyntaxTree::SyntaxTree(TokenData parent, TokenData leftTok, TokenData rightTok)
{
	attributes = parent;
	left = new SyntaxTree(leftTok);
	right = new SyntaxTree(rightTok);
}

SyntaxTree::~SyntaxTree()
{
     if(left)
         delete left;
     if(right)
         delete right;
}

void SyntaxTree::bifurcate(SyntaxTree * leftBranch, TokenData rightTok)
{
	left = leftBranch;
	right = new SyntaxTree(rightTok);
}

void SyntaxTree::bifurcate(TokenData leftTok, TokenData rightTok)
{
	left = new SyntaxTree(leftTok);
	right = new SyntaxTree(rightTok);
}

void SyntaxTree::bifurcate(SyntaxTree * leftBranch, SyntaxTree * rightBranch)
{
	left = leftBranch;
	right = rightBranch;
}

SyntaxTree * SyntaxTree::setRight(TokenData rightTok)
{
	right = new SyntaxTree(rightTok);
	return this;
}

bool SyntaxTree::printTree()
{
	int ioStat;
	int parentNodeNum = 0, nodeNum = 0;

	std::cout << "graph \"Abstract Syntax Tree\"\n{\n";
	ioStat = this->printBranch(& parentNodeNum, & nodeNum);
	std::cout << "}\n";
	return ioStat;
}

bool  SyntaxTree::printBranch(int * parentNodeNum, int * nodeNum)
{
	bool ioStatL;
	bool ioStatR;
	int parentNodeNumm = *nodeNum;

	this->printNode(nodeNum);

	if(left)
	{
		this->printEdge(parentNodeNumm, *nodeNum);
		ioStatL = left->printBranch(parentNodeNum, nodeNum);
	}
	if(right)
	{
		this->printEdge(parentNodeNumm, *nodeNum);
		ioStatR = right->printBranch(parentNodeNum, nodeNum);
	}

	return !(ioStatL && ioStatR);
}

bool SyntaxTree::printEdge(int parentNodeNum, int nodeNum)
{
	std::cout << parentNodeNum << " -- " << nodeNum << " [style = solid]\n";
	return true;
}

bool SyntaxTree::printNode(int * nodeNum)
{
	std::cout << *nodeNum << " [label = \"";

	symbolKind kind = this->attributes.attributeKind;
	switch(kind){
	case integerKind : std::cout << this->attributes.integer; break;
	case realKind : std::cout << this->attributes.real; break;
	case lexemeKind : std::cout << this->attributes.lexeme; break;
	default : std::cout << "KIND not yet defined!"; break;
	}

	std::cout << "\\nLine: " << this->attributes.lineNumber << "\"]\n";
	(*nodeNum)++;
	return true;
}
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
//---------------------------------------------------------------------------------------------------------------------
