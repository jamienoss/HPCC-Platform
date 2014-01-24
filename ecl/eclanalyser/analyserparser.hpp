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

//extern int ecl3yyparse(AnalyserParser * parser, yyscan_t scanner);

//----------------------------------AnalyserParser--------------------------------------------------------------------
class AnalyserParser
{
    friend int ecl3yyparse(AnalyserParser * parser, yyscan_t scanner);

public:
    AnalyserParser(IFileContents * queryContents);
    ~AnalyserParser();

    void setRoot(ISyntaxTree * node);
    void printAST();
    int parse();
    AnalyserLexer & getLexer();

    void analyseGrammar();
    void createSymbolList(AnalyserST * tree, std::vector <std::string> & symbolList, TokenKind kind);
    void printStringVector(std::vector <std::string> vector);

protected:
    void init(IFileContents * queryContents);

private:
    Owned<AnalyserST> ast;
    AnalyserLexer * lexer;
};
//----------------------------------AnalyserLexer--------------------------------------------------------------------
class AnalyserLexer
{
public:
    AnalyserLexer(IFileContents * queryContents);
    ~AnalyserLexer();

    int parse(AnalyserParser * parser);
    void updatePos(unsigned delta);
    void resetPos();

    int yyColumn;
    int yyPosition;
    ISourcePath * sourcePath;

    unsigned nestCounter;
    unsigned productionLineNo;
    StringBuffer productionText;

protected:
    yyscan_t scanner;
    Owned<IFileContents> text;
    char *yyBuffer;

    virtual void init(IFileContents * queryContents);
};
//--------------------------------------------------------------------------------------------------------------

#endif
