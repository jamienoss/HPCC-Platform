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

#include "tokendata.hpp"
#include "syntaxtree.hpp"
#include "asyntaxtree.hpp"
#include "atokendata.hpp"
#include <string>
#include <vector>

class SyntaxTree;

class AnalyserST : public SyntaxTree
{
public :
    static ISyntaxTree * createSyntaxTree(TokenData & tok);
    static ISyntaxTree * createSyntaxTree(TokenKind & _token, const ECLlocation & _pos);

    virtual void printTree();
    virtual void printNode(unsigned * nodeNum,  IIOStream * out);
    virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);

    virtual void extractSymbols(std::vector <std::string> & symbolList, TokenKind & kind);
    void setSymbolList(std::vector <std::string> & list);
    void printSymbolList();

    static std::vector <std::string> * symbolList;

private:
    AnalyserST(TokenData & tok);
    AnalyserST(TokenKind & _token, const ECLlocation & pos);

};

#endif
