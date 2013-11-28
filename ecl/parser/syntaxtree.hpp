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

//----------------------------------SyntaxTree--------------------------------------------------------------------
class SyntaxTree
{

public:
    SyntaxTree();
    SyntaxTree(TokenData node);
    SyntaxTree(TokenData parent, TokenData leftTok, TokenData rightTok);
    ~SyntaxTree();
    bool printTree();
    bool printBranch(int * parentNodeNum, int * nodeNum);
    bool printEdge(int parentNodeNum, int nodeNum);
    bool printNode(int * nodeNum);

    SyntaxTree * setRight(TokenData rightTok);
    void bifurcate(SyntaxTree * leftBranch, TokenData rightTok);
    void bifurcate(SyntaxTree * leftBranch, SyntaxTree * rightBranch);
    void bifurcate(TokenData leftTok, TokenData rightTok);

private:
    TokenData attributes;
    SyntaxTree * left;
    SyntaxTree * right;
    SyntaxTree * aux;
};

#endif
