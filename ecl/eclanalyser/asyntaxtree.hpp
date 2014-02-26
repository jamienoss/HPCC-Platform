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

class AnalyserSymbols
{
public :
    virtual void setIdNameList(IdTable * list);
    virtual void printIdNameList();
    virtual IdTable * queryIdTable();
    virtual IIdTableItem * queryIdTable(aindex_t pos);

protected:
    AnalyserSymbols();
    static IdTable * idNameList;
};



//----------------------------------------------------------------------------------------------------------------------
class AnalyserPuncST : public AnalyserSymbols, PuncSyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, TokenKind _kind);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void appendSTvalue(StringBuffer & str);
    virtual void printIdNameList() { AnalyserSymbols::printIdNameList(); }
    virtual void setIdNameList(IdTable * symbolList) { AnalyserSymbols::setIdNameList(symbolList); }
    virtual IdTable * queryIdTable() { return AnalyserSymbols::queryIdTable(); }
    virtual IIdTableItem * queryIdTable(aindex_t pos) { return AnalyserSymbols::queryIdTable(pos); }

private:
    AnalyserPuncST(const ECLlocation & _pos, TokenKind _kind);
};
//-----------------------------------------------------------------------------------------------------------------------
class AnalyserIdST : public AnalyserSymbols, IdSyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, IIdAtom * _id, TokenKind _kind);
    //virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);
    virtual TokenKind queryKind() { return kind; }
    virtual void createIdNameList(IdTable & symbolList, TokenKind _kind);
    virtual void printIdNameList() { AnalyserSymbols::printIdNameList(); }
    virtual void setIdNameList(IdTable * symbolList) { AnalyserSymbols::setIdNameList(symbolList); }
    virtual IdTable * queryIdTable() { return AnalyserSymbols::queryIdTable(); }
    virtual IIdTableItem * queryIdTable(aindex_t pos) { return AnalyserSymbols::queryIdTable(pos); }

private:
    AnalyserIdST(const ECLlocation & _pos, IIdAtom * _id, TokenKind _kind);
    TokenKind kind;
};
//-----------------------------------------------------------------------------------------------------------------------
class AnalyserStringST : public AnalyserSymbols, SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const StringBuffer & _text, TokenKind _kind);
    //virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual TokenKind queryKind() { return kind; }
    virtual void printIdNameList() { AnalyserSymbols::printIdNameList(); }
    virtual void setIdNameList(IdTable * symbolList) { AnalyserSymbols::setIdNameList(symbolList); }

private:
    AnalyserStringST(const ECLlocation & _pos, const StringBuffer & _text, TokenKind _kind);
    TokenKind kind;
    StringBuffer text;
};
//-----------------------------------------------------------------------------------------------------------------------

//inline ISyntaxTree * createAnalyserSymbols(){ return AnalyserSymbols::createSyntaxTree(); }
//inline ISyntaxTree * createAnalyserSymbols(const ECLlocation & _pos){ return AnalyserSymbols::createSyntaxTree(_pos); }
ISyntaxTree * createAnalyserPuncST(const ECLlocation & _pos, TokenKind _kind);
ISyntaxTree * createAnalyserIdST(const ECLlocation & _pos, IIdAtom * _id, TokenKind _kind);
ISyntaxTree * createAnalyserStringST(const ECLlocation & _pos, const StringBuffer & _text, TokenKind _kind);

//-----------------------------------------------------------------------------------------------------------------------

class PatchGrammar : public TreeWalker
{
public :
    PatchGrammar(IdTable * _idTable) : TreeWalker(true) { idTable = _idTable; }
    virtual bool action(ISyntaxTree * node);

protected :
    IdTable * idTable;
};



#endif
