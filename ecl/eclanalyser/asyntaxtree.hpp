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
#ifndef AASyntaxTree_HPP
#define AASyntaxTree_HPP

//#include "jiface.hpp"
#include "jlib.hpp"
#include "jfile.hpp"

#include "parserdata.hpp"
#include "syntaxtree.hpp"
#include "analyserpd.hpp"
#include <string>

class SyntaxTree;

class AnalyserSymbols
{
public :
    void setSymbolList(StringArray & list);
    void printSymbolList();

    static StringArray symbolList;

protected:
    AnalyserSymbols();
};

//----------------------------------------------------------------------------------------------------------------------
class AnalyserPuncST : public AnalyserSymbols, PuncSyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const TokenKind & _kind);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void appendSTvalue(StringBuffer & str);

private:
    AnalyserPuncST(const ECLlocation & _pos, const TokenKind & _kind);
};
//-----------------------------------------------------------------------------------------------------------------------
class AnalyserIdST : public AnalyserSymbols, IdSyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, IIdAtom * _id, const TokenKind & _kind);
    //virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);
    virtual TokenKind getKind() { return kind; }
    virtual void extractSymbols(StringArray & symbolList, TokenKind & _kind);

private:
    AnalyserIdST(const ECLlocation & _pos, IIdAtom * _id, const TokenKind & _kind);
    TokenKind kind;
};
//-----------------------------------------------------------------------------------------------------------------------
class AnalyserStringST : public AnalyserSymbols, SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const StringBuffer & _text, const TokenKind & _kind);
    //virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual TokenKind getKind() { return kind; }

private:
    AnalyserStringST(const ECLlocation & _pos, const StringBuffer & _text, const TokenKind & _kind);
    TokenKind kind;
    StringBuffer text;
};
//-----------------------------------------------------------------------------------------------------------------------

//inline ISyntaxTree * createAnalyserSymbols(){ return AnalyserSymbols::createSyntaxTree(); }
//inline ISyntaxTree * createAnalyserSymbols(const ECLlocation & _pos){ return AnalyserSymbols::createSyntaxTree(_pos); }
ISyntaxTree * createAnalyserPuncST(const ECLlocation & _pos, const TokenKind & _kind);
ISyntaxTree * createAnalyserIdST(const ECLlocation & _pos, IIdAtom * _id, const TokenKind & _kind);
ISyntaxTree * createAnalyserStringST(const ECLlocation & _pos, const StringBuffer & _text, const TokenKind & _kind);





#endif
