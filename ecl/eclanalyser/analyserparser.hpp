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
#include "eclparser.hpp"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

class AnalyserParser;
class AnalyserLexer;

//----------------------------------AnalyserParser--------------------------------------------------------------------
class AnalyserParser : public EclParser
{
    friend int ecl3yyparse(AnalyserParser * parser, yyscan_t scanner);

public:
    AnalyserParser(IFileContents * queryContents, IErrorReceiver * errs);

    virtual int parse();
    virtual void analyseGrammar();
    void createSymbolList(ISyntaxTree * tree, StringBufferArray & symbolList, TokenKind kind);
    void printSymbolList(const StringBufferArray & list);
    AnalyserLexer & getLexer();

private:
    AnalyserLexer * lexer;
};
//----------------------------------AnalyserLexer--------------------------------------------------------------------
class AnalyserLexer : public EclLexer
{
public:
    AnalyserLexer(IFileContents * queryContents);
    virtual ~AnalyserLexer();
    virtual void init(IFileContents * queryContents);

    unsigned nestCounter;
    unsigned productionLineNo;
    StringBuffer productionText;
};
//--------------------------------------------------------------------------------------------------------------

#endif
