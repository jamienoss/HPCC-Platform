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
#ifndef ECLPARSER_HPP
#define ECLPARSER_HPP

#include "platform.h"
#include "jhash.hpp"
#include "hqlexpr.hpp"  // MORE: Split IFileContents out of this file

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

class EclParser;
class EclLexer;
class SyntaxTree;
class TokenData;


enum symbolKind
{
	integerKind,
	realKind,
	lexemeKind
};
//----------------------------------TokenData--------------------------------------------------------------------
class TokenData
{
public:
	symbolKind attributeKind;
	union
	{
		int integer;
		float real;
		char lexeme[256];
		//char * lexeme[];
	};
};
//----------------------------------SyntaxTree--------------------------------------------------------------------
class SyntaxTree
{

public:
    SyntaxTree();
    SyntaxTree(TokenData node);
    SyntaxTree(TokenData parent, TokenData leftTok, TokenData rightTok);
    ~SyntaxTree();
    bool printTree();
    bool printBranch(int * parentNodeNum, int * nodeNum);
    bool printEdge(int parentNodeNum, int nodeNum);
    bool printNode(int * nodeNum);

    SyntaxTree * setRight(TokenData rightTok);
    void bifurcate(SyntaxTree * leftBranch, TokenData rightTok);
    void bifurcate(SyntaxTree * leftBranch, SyntaxTree * rightBranch);
    void bifurcate(TokenData leftTok, TokenData rightTok);

private:
    TokenData attributes;
    SyntaxTree * left;
    SyntaxTree * right;
};
//----------------------------------EclParser--------------------------------------------------------------------
class EclParser
{
    friend int ecl2yyparse(EclParser * parser, yyscan_t scanner);

public:
    EclParser(IFileContents * queryContents);
    ~EclParser();

    void setRoot(SyntaxTree * node);
    SyntaxTree * bifurcate(TokenData parent, TokenData left, TokenData right);
    bool printAST();
    int parse();

private:
    void init(IFileContents * queryContents);

private:
    EclLexer * lexer;
    SyntaxTree * ast;
};
//----------------------------------EclLexer--------------------------------------------------------------------
class EclLexer
{
public:
    EclLexer(IFileContents * queryContents);
    ~EclLexer();

    int parse(EclParser * parser);

private:
    yyscan_t scanner;
    Owned<IFileContents> text;
    char *yyBuffer;
    TokenData token;

    void init(IFileContents * queryContents);
};
//--------------------------------------------------------------------------------------------------------------

//Next stages:
// Separate or same dll? Up to you.
// Fill in EclLexer with code from HqlLex
// Go through and make sure class names, file names etc. are as meaningful as possible.
// rename symTree
// modify EclCC::processSingleQuery to conditionally call the new parser, and return xml/dot/json
// get a very simple query - a single id working.
// Add constants and '+' operator, brackets (1) (1+2)
// Flesh out symTree - members, position, handling constants, types, generating xml


#endif
