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


//----------------------------------SyntaxTree--------------------------------------------------------------------
//typedef unsigned short TokenKind;

interface ISyntaxTree : public IInterface
{
    virtual TokenKind getKind() = 0;
    virtual const ECLlocation & queryPosition() const = 0;
    virtual bool printTree() = 0;
    virtual void printXml(StringBuffer & out) = 0;
    virtual ISyntaxTree * queryChild(unsigned i) = 0;

    virtual void setLeft(ISyntaxTree * node) = 0;
    virtual void setRight(ISyntaxTree * node) = 0;
    virtual void addChild(ISyntaxTree * addition) = 0;
    virtual void transferChildren(ISyntaxTree * node) = 0;
    virtual bool printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out) = 0;

};

typedef IArrayOf<ISyntaxTree> SyntaxTreeArray;

/// The following code can now be moved and hidden ---
class SyntaxTree : public CInterfaceOf<ISyntaxTree>
{
public:
    static ISyntaxTree * createSyntaxTree();
    static ISyntaxTree * createSyntaxTree(TokenKind token, const ECLlocation & pos);
    static ISyntaxTree * createSyntaxTree(TokenData & token);
    static ISyntaxTree * createSyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok);
    static ISyntaxTree * createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch);
    static ISyntaxTree * createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch, TokenData & rightTok);
    static ISyntaxTree * createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch, ISyntaxTree * righBranch);
    ~SyntaxTree();

//Implementation of ISyntaxTree
    virtual bool printTree();
    virtual void printXml(StringBuffer & out);
    virtual ISyntaxTree * queryChild(unsigned i);

    virtual void addChild(ISyntaxTree * addition);
    virtual void transferChildren(ISyntaxTree * node);

    virtual TokenKind getKind();
    virtual const ECLlocation & queryPosition() const { return pos; }
    virtual void setLeft(ISyntaxTree * node);
    virtual void setRight(ISyntaxTree * node);
    virtual bool printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out);

protected:
    SyntaxTree();
    SyntaxTree(TokenData & token);
    SyntaxTree(TokenKind token, const ECLlocation & pos);

    bool printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out);
    virtual bool printNode(unsigned * nodeNum,  IIOStream * out);
    bool printNode(unsigned * nodeNum, IIOStream * out, const char * text, const char * colour);

    void setLeft(TokenData & token);
    void setRight(TokenData & token);

    SyntaxTree * queryPrivateChild(unsigned i);

public:
    TokenKind token;
    union
    {
        float real;
        IIdAtom * name;
    };
    ECLlocation pos;

    Owned<ISyntaxTree> left;
    Owned<ISyntaxTree> right;
    SyntaxTreeArray children;
};

class IntegerSyntaxTree : public SyntaxTree
{
public:
    IntegerSyntaxTree(TokenData & token);

    virtual bool printNode(unsigned * nodeNum, IIOStream * out);
    virtual bool printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out);


public:
    int value;
};

#endif
