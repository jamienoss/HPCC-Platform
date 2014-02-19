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
#include "syntaxtree.hpp"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

class EclParser;
class EclLexer;

//----------------------------------EclParser--------------------------------------------------------------------
class EclParser : public CInterface
{
     friend int ecl2yyparse(EclParser * parser, yyscan_t scanner);

public:
    EclParser();
    EclParser(IFileContents * queryContents, IErrorReceiver * errs);
    ~EclParser();
    IMPLEMENT_IINTERFACE

    ISyntaxTree * releaseAST();

    virtual void analyseGrammar() { };
    void setRoot(ISyntaxTree * node);
    void printAST();
    virtual int parse();
    EclLexer & getLexer();
    void reportError(int errNo, const char *msg, const char * _sourcePath, int _lineno, int _column, int _position);

protected:
    IErrorReceiver * errorHandler;
    Owned<ISyntaxTree> ast;

private:
    EclLexer * lexer;
};
//----------------------------------EclLexer--------------------------------------------------------------------
class EclLexer
{
public:
    EclLexer();
    EclLexer(IFileContents * queryContents);
    virtual ~EclLexer();

    void updatePos(unsigned delta);
    void resetPos();
    yyscan_t & getScanner();

    int yyColumn;
    int yyPosition;
    ISourcePath * sourcePath;

protected:
    yyscan_t scanner;
    Owned<IFileContents> text;
    char *yyBuffer;

    virtual void init(IFileContents * queryContents);
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
