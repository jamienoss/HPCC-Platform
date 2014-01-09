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
#ifndef SYNTAXTREE_HPP
#define SYNTAXTREE_HPP

#include "tokendata.hpp"
#include <string>
#include <vector>

class linkedSTlist;


//----------------------------------SyntaxTree--------------------------------------------------------------------
class SyntaxTree
{

public:
    SyntaxTree * createSyntaxTree();
    SyntaxTree * createSyntaxTree(TokenData & token);
    SyntaxTree * createSyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok);
    SyntaxTree * createSyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch, TokenData & rightTok);
    SyntaxTree * createSyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch, SyntaxTree * righBranch);
    SyntaxTree * createSyntaxTree(TokenData & token, SyntaxTree * tempAux);
    ~SyntaxTree();

    SyntaxTree * release();

    bool printTree();
    bool printBranch(unsigned * parentNodeNum, unsigned * nodeNum);
    bool printEdge(unsigned parentNodeNum, unsigned nodeNum);
    bool printNode(unsigned * nodeNum);

    void bifurcate(SyntaxTree * leftBranch, TokenData & rightTok);
    void bifurcate(SyntaxTree * leftBranch, SyntaxTree * rightBranch);
    void bifurcate(TokenData & leftTok, TokenData & rightTok);

    void setLeft(TokenData & token);
    void setLeft(SyntaxTree * node);
    void setRight(TokenData & token);
    void setRight(SyntaxTree * node);

    void addChild(SyntaxTree * addition);
    SyntaxTree ** releaseAux();
    void transferChildren(SyntaxTree * node);

    const char * getLexeme();

    void extractSymbols(std::vector <std::string> & symbolList, symbolKind kind);

private:
    SyntaxTree();
    SyntaxTree(TokenData & token);

private:
    TokenData attributes;

    SyntaxTree * left;
    SyntaxTree * right;
    linkedSTlist * children;

    //SyntaxTree ** aux;
    //int auxLength;
};

//----------------------------------linkeSTlist--------------------------------------------------------------------
class linkedSTlist
{

    friend class SyntaxTree;

public:
    linkedSTlist();
    ~linkedSTlist();
    void push(SyntaxTree * addition);
    SyntaxTree * pop();
    unsigned size();

    SyntaxTree * operator[](unsigned idx);
    // const value_type& operator[](index_type idx) const;

private:
    linkedSTlist(SyntaxTree * node);

private:
    SyntaxTree * data;
    linkedSTlist * next;
};


#endif
