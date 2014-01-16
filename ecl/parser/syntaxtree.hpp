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
#include"jlib.hpp"

class linkedSTlist;

//----------------------------------SyntaxTree--------------------------------------------------------------------
typedef unsigned short TokenKind;

interface ISyntaxTree : public IInterface
{
    virtual TokenKind getKind() = 0;
};

class SyntaxTree;
typedef CIArrayOf<SyntaxTree> SyntaxTreeArray;

class SyntaxTree : public CInterface
{
public:
    static SyntaxTree * createSyntaxTree();
    static SyntaxTree * createSyntaxTree(TokenKind token, const ECLlocation & pos);
    static SyntaxTree * createSyntaxTree(TokenData & token);
    static SyntaxTree * createSyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok);
    static SyntaxTree * createSyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch);
    static SyntaxTree * createSyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch, TokenData & rightTok);
    static SyntaxTree * createSyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch, SyntaxTree * righBranch);
    ~SyntaxTree();

    bool printTree();
    bool printBranch(unsigned * parentNodeNum, unsigned * nodeNum, Owned<IFileIOStream> & out);
    bool printEdge(unsigned parentNodeNum, unsigned nodeNum, Owned<IFileIOStream> & out);
    bool printNode(unsigned * nodeNum,  Owned<IFileIOStream> & out);

    void addChild(SyntaxTree * addition);
    void transferChildren(SyntaxTree * node);

    virtual TokenKind getKind();
    virtual const ECLlocation & queryPosition() const { return pos; }

private:
    SyntaxTree();
    SyntaxTree(TokenData & token);
    SyntaxTree(TokenKind token, const ECLlocation & pos);

    void setLeft(TokenData & token);
    void setLeft(SyntaxTree * node);
    void setRight(TokenData & token);
    void setRight(SyntaxTree * node);

private:
    TokenKind token;
    union
    {
        int integer;
        float real;
        IIdAtom * name;
    };
    ECLlocation pos;

    Owned<SyntaxTree> left;
    SyntaxTree * right;
    SyntaxTreeArray children;
};

#endif
