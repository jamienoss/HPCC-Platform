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
#include <string>
#include <vector>

class linkedSTlist;


//----------------------------------ASyntaxTree--------------------------------------------------------------------
class ASyntaxTree : public CInterface
{

public:
    ASyntaxTree * createASyntaxTree();
    ASyntaxTree * createASyntaxTree(TokenData & token);
    ASyntaxTree * createASyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok);
    ASyntaxTree * createASyntaxTree(TokenData & parentTok, ASyntaxTree * leftBranch, TokenData & rightTok);
    ASyntaxTree * createASyntaxTree(TokenData & parentTok, ASyntaxTree * leftBranch, ASyntaxTree * righBranch);
    ASyntaxTree * createASyntaxTree(TokenData & token, ASyntaxTree * tempAux);
    ~ASyntaxTree();

    ASyntaxTree * release();

    bool printTree();
    bool printBranch(unsigned * parentNodeNum, unsigned * nodeNum, Owned<IFileIOStream> & out);
    bool printEdge(unsigned parentNodeNum, unsigned nodeNum, ASyntaxTree * child, Owned<IFileIOStream> & out);
    bool printNode(unsigned * nodeNum, Owned<IFileIOStream> & out);

    void bifurcate(ASyntaxTree * leftBranch, TokenData & rightTok);
    void bifurcate(ASyntaxTree * leftBranch, ASyntaxTree * rightBranch);
    void bifurcate(TokenData & leftTok, TokenData & rightTok);

    void setLeft(TokenData & token);
    void setLeft(ASyntaxTree * node);
    void setRight(TokenData & token);
    void setRight(ASyntaxTree * node);

    void addChild(ASyntaxTree * addition);
    ASyntaxTree ** releaseAux();
    void transferChildren(ASyntaxTree * node);

    const char * getLexeme();
    symbolKind getKind();

    void extractSymbols(std::vector <std::string> & symbolList, symbolKind kind);
    void setSymbolList(std::vector <std::string> & list);
    void printSymbolList();

private:
    ASyntaxTree();
    ASyntaxTree(TokenData & token);

private:
    TokenData attributes;

    ASyntaxTree * left;
    ASyntaxTree * right;
    //linkedSTlist * children;
    CIArrayOf<ASyntaxTree> * children;

    static std::vector <std::string> * symbolList;
};

//----------------------------------linkeSTlist--------------------------------------------------------------------
class linkedSTlist
{

    friend class ASyntaxTree;

public:
    linkedSTlist();
    ~linkedSTlist();
    void push(ASyntaxTree * addition);
    ASyntaxTree * pop();
    unsigned size();

    ASyntaxTree * operator[](unsigned idx);
    // const value_type& operator[](index_type idx) const;

private:
    linkedSTlist(ASyntaxTree * node);

private:
    ASyntaxTree * data;
    linkedSTlist * next;
};


#endif
