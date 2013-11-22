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
#ifndef ECLGRAM_HPP
#define ECLGRAM_HPP

#include "platform.h"
#include "jhash.hpp"
#include "hqlexpr.hpp"  // MORE: Split IFileContents out of this file
//#include "ecllex.hpp"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

class EclParser;
class EclLexer;
class SyntaxTree;
class TokenData;

#define YYSTYPE	 TokenData
//SyntaxTree
/*
extern int ecl2yylex_init(yyscan_t * scanner);
extern int ecl2yylex_destroy(yyscan_t scanner);
extern int ecl2yyparse(EclParser * parser);
*/

//----------------------------------TokenData--------------------------------------------------------------------
class TokenData
{
public:
	TokenData();
	~TokenData();
	int dummy;
	union
	{
		int integer;
		float real;
		char * lexeme[];
	};
};
//----------------------------------SyntaxTree--------------------------------------------------------------------
class SyntaxTree
{
public:
	TokenData attributes;

public:
    SyntaxTree();
    ~SyntaxTree();
    bool printTree();
    bool printNode();

	//SyntaxTree & release();

private:
    SyntaxTree * left;
    SyntaxTree * right;
};
//----------------------------------EclParser--------------------------------------------------------------------
class EclParser
{
    //friend class EclLexer;
    friend int ecl2yyparse(EclParser * parser);

public:
    EclParser(IFileContents * queryContents);
    ~EclParser();

    void setRoot(SyntaxTree * node);
    bool printTree(SyntaxTree * tree);
    int yyLex(YYSTYPE * yylval, const short * activeState);
    SyntaxTree * parse();

private:
    void init(IFileContents * queryContents);

public:
    EclLexer * lexer;
    SyntaxTree * ast;
};
//----------------------------------EclLexer--------------------------------------------------------------------
class EclLexer
{
public:
    EclLexer(IFileContents * queryContents);
    ~EclLexer();

    static int doyyFlex(YYSTYPE & returnToken, yyscan_t yyscanner, EclLexer * lexer, bool lookup, const short * activeState);
    int yyLex(YYSTYPE & returnToken, bool lookup, const short * activeState);    /* lexical analyzer */

private:
    void init(IFileContents * queryContents);

    yyscan_t scanner;
    Owned<IFileContents> text;
    char *yyBuffer;

public:
    TokenData token;
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
