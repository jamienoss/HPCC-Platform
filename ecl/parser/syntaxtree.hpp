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

class SyntaxTree;

class ISyntaxTree
{
public :
   virtual SyntaxTree * createSyntaxTree() = 0;
   virtual SyntaxTree * createSyntaxTree(TokenData & node) = 0;
   virtual SyntaxTree * createSyntaxTree(TokenData & parent, TokenData & leftTok, TokenData & rightTok) = 0;
   virtual SyntaxTree * createSyntaxTree(TokenData & parent, SyntaxTree * leftBranch, TokenData & rightTok) = 0;
   virtual SyntaxTree * createSyntaxTree(TokenData & node, SyntaxTree * tempAux) = 0;

   virtual bool printTree() const = 0;
   virtual void bifurcate(SyntaxTree * leftBranch, TokenData rightTok) = 0;
   virtual void bifurcate(SyntaxTree * leftBranch, SyntaxTree * rightBranch) = 0;
   virtual void bifurcate(TokenData leftTok, TokenData rightTok) = 0;
};


//----------------------------------SyntaxTree--------------------------------------------------------------------
class SyntaxTree : public ISyntaxTree
{

public:


    ~SyntaxTree();

    SyntaxTree * release();

    bool printTree() const;
    bool printBranch(unsigned * parentNodeNum, unsigned * nodeNum);
    bool printEdge(unsigned parentNodeNum, unsigned nodeNum);
    bool printNode(unsigned * nodeNum);

    void bifurcate(SyntaxTree * leftBranch, TokenData rightTok);
    void bifurcate(SyntaxTree * leftBranch, SyntaxTree * rightBranch);
    void bifurcate(TokenData leftTok, TokenData rightTok);
    void add2Aux(SyntaxTree * addition);
    SyntaxTree ** releaseAux();
    void takeAux(SyntaxTree * node);
    bool isAux();


    unsigned getAuxLength();

protected:
    SyntaxTree * createSyntaxTree();
    SyntaxTree * createSyntaxTree(TokenData & node);
    SyntaxTree * createSyntaxTree(TokenData & parent, TokenData & leftTok, TokenData & rightTok);
    SyntaxTree * createSyntaxTree(TokenData & parent, SyntaxTree * leftBranch, TokenData & rightTok);
    SyntaxTree * createSyntaxTree(TokenData & node, SyntaxTree * tempAux);

private:
    SyntaxTree();
    SyntaxTree(TokenData & node);
    SyntaxTree(TokenData & parent, TokenData & leftTok, TokenData & rightTok);
    SyntaxTree(TokenData & parent, SyntaxTree * leftBranch, TokenData & rightTok);
    SyntaxTree(TokenData & node, SyntaxTree * tempAux);

    TokenData attributes;

    ISyntaxTree * left;
    ISyntaxTree * right;

    SyntaxTree ** aux;
    int auxLength;
};

#endif
