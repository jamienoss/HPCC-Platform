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
#ifndef ANALYSERPARSER_HPP
#define ANALYSERPARSER_HPP

#include "platform.h"
#include "jhash.hpp"
#include "hqlexpr.hpp"  // MORE: Split IFileContents out of this file
#include "asyntaxtree.hpp"
#include <vector>
#include <string>

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

class AnalyserParser;
class AnalyserLexer;

//----------------------------------AnalyserParser--------------------------------------------------------------------
class AnalyserParser
{
    friend int ecl3yyparse(AnalyserParser * parser, yyscan_t scanner);

public:
    AnalyserParser(IFileContents * queryContents);
    ~AnalyserParser();
    ASyntaxTree * releaseAST();


    void setRoot(ASyntaxTree * node);
    bool printAST();
    int parse();

    void analyseGrammar();
    void createSymbolList(ASyntaxTree * tree, std::vector <std::string> & symbolList, symbolKind kind);
    void printStringVector(std::vector <std::string> vector);

private:
    AnalyserLexer * lexer;
    ASyntaxTree * ast;

    void init(IFileContents * queryContents);
};
//----------------------------------AnalyserLexer--------------------------------------------------------------------
class AnalyserLexer
{
public:
    AnalyserLexer(IFileContents * queryContents);
    ~AnalyserLexer();

    int parse(AnalyserParser * parser);

private:
    yyscan_t scanner;
    Owned<IFileContents> fileIn;
    char *yyBuffer;

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
