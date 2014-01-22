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
    virtual void printTree() = 0;
    virtual void printXml(StringBuffer & out) = 0;
    virtual ISyntaxTree * queryChild(unsigned i) = 0;

    virtual void addChild(ISyntaxTree * addition) = 0;
    virtual void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out) = 0;
};

typedef IArrayOf<ISyntaxTree> SyntaxTreeArray;

/// The following code can now be moved and hidden ---
class SyntaxTree : public CInterfaceOf<ISyntaxTree>
{
public:
    static ISyntaxTree * createSyntaxTree();
    static ISyntaxTree * createSyntaxTree(TokenKind token, const ECLlocation & pos);
    static ISyntaxTree * createSyntaxTree(TokenData & token);
    ~SyntaxTree();

//Implementation of ISyntaxTree
    virtual void printTree();
    virtual void printXml(StringBuffer & out);
    virtual void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out);

    virtual ISyntaxTree * queryChild(unsigned i);
    virtual void addChild(ISyntaxTree * addition);

    virtual TokenKind getKind();
    virtual const ECLlocation & queryPosition() const { return pos; }

protected:
    SyntaxTree();
    SyntaxTree(TokenData & token);
    SyntaxTree(TokenKind token, const ECLlocation & pos);

    void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out);
    virtual void printNode(unsigned * nodeNum,  IIOStream * out);
    void printNode(unsigned * nodeNum, IIOStream * out, const char * text, const char * colour);

    SyntaxTree * queryPrivateChild(unsigned i);

public:
    TokenKind token;
    union
    {
        float real;
        IIdAtom * name;
    };
    ECLlocation pos;

    SyntaxTreeArray children;
};

class IntegerSyntaxTree : public SyntaxTree
{
public:
    IntegerSyntaxTree(TokenData & token);

    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out);


public:
    int value;
};

inline ISyntaxTree * createSyntaxTree() { return SyntaxTree::createSyntaxTree(); }
inline ISyntaxTree * createSyntaxTree(TokenKind token, const ECLlocation & pos) { return SyntaxTree::createSyntaxTree(token, pos); }
inline ISyntaxTree * createSyntaxTree(TokenData & token) { return SyntaxTree::createSyntaxTree(token); }

#endif
