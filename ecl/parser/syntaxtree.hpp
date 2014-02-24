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

#include "defvalue.hpp"
#include "parserdata.hpp"
#include"jlib.hpp"
#include <vector>
#include <string>
#include <iostream>

class ISyntaxTree;

//----------------------------------IdTableItem--------------------------------------------------------------------
interface IIdTableItem : public IInterface
{
public :
    virtual const char * getIdName() = 0;
};

class IdTableItem : public CInterfaceOf<IIdTableItem>
{
public :
    IdTableItem();
    IdTableItem(const IIdAtom & id) { symbol = createIdAtom(id); }
    ~IdTableItem();

    virtual const char * getIdName();

protected :
    IIdAtom * symbol;
    ISyntaxTree * node;
};
typedef  IArrayOf<IIdTableItem> IdTable;

//----------------------------------Printer--------------------------------------------------------------------
class Printer
{
public :
    int id;
    int indentation;
    StringBuffer * str;

    Printer() { indentation = 0; id = 0; str = NULL; }
    Printer(int _indentation, StringBuffer * _str) { indentation = _indentation; id = 0; str = _str; }
    ~Printer() {}

    StringBuffer & indent()
    {
        for (unsigned short i = 0; i < indentation; ++i)
            str->append("  ");
        return *str;
    }
    inline void tabIncrease() { ++indentation; ++id; }
    inline void tabDecrease() { --indentation; }
};

typedef IArrayOf<ISyntaxTree> SyntaxTreeArray;
//----------------------------------SyntaxTree--------------------------------------------------------------------
interface ISyntaxTree : public IInterface
{
    virtual TokenKind getKind() = 0;
    virtual const ECLlocation & queryPosition() const = 0;
    virtual void printTree() = 0;
    virtual void printXml(Printer * print) = 0;
    virtual void appendSTvalue(StringBuffer & str) = 0;

    virtual ISyntaxTree * queryChild(unsigned i) = 0;
    virtual SyntaxTreeArray & getChildren() = 0;

    virtual void addChild(ISyntaxTree * addition) = 0;
    virtual void transferChildren(ISyntaxTree * addition) = 0;

    //virtual void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out) = 0;
    virtual void createIdNameList(IdTable & symbolList, TokenKind & _kind) = 0;
    virtual void setIdNameList(IdTable * symbolList) = 0;
    virtual void printIdNameList() = 0;

};

class SyntaxTree : public CInterfaceOf<ISyntaxTree>
{
public:
    static ISyntaxTree * createSyntaxTree();
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos);
    ~SyntaxTree();

//Implementation of ISyntaxTree
    virtual void printTree();
    virtual void printXml(Printer * print);
    void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out);

    virtual ISyntaxTree * queryChild(unsigned i);
    virtual SyntaxTreeArray & getChildren();

    virtual void addChild(ISyntaxTree * addition);
    virtual void transferChildren(ISyntaxTree * addition);

    virtual TokenKind getKind() { return 0; }
    virtual const ECLlocation & queryPosition() const { return pos; }
    virtual void appendSTvalue(StringBuffer & str);

protected:
    virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);
    virtual void printNode(unsigned * nodeNum,  IIOStream * out);
    void printNode(unsigned * nodeNum, IIOStream * out, const char * text, const char * colour);

    virtual void createIdNameList(IdTable & symbolList, TokenKind & _kind);
    virtual void setIdNameList(IdTable * symbolList) {};
    virtual void printIdNameList() {};

    SyntaxTree * queryPrivateChild(unsigned i);

protected:
    ECLlocation pos;
    SyntaxTreeArray children;

protected:
    SyntaxTree();
    SyntaxTree(const ECLlocation & _pos);
};

/// The following code can now be moved and hidden ---
//MORE: the following below might need parameterless constructors and 'create' wrappers
//--------------------------------------------------------------------------------------------------------
class PuncSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const TokenKind & _token);

protected:
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void appendSTvalue(StringBuffer & str);
    PuncSyntaxTree(ECLlocation _pos, TokenKind _token);
    TokenKind value;
};

//--------------------------------------------------------------------------------------------------------
class ConstantSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, IValue * constant);


protected:
    ConstantSyntaxTree(ECLlocation _pos, IValue * constant);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void appendSTvalue(StringBuffer & str);

protected:
    OwnedIValue value;
};
//--------------------------------------------------------------------------------------------------------
class IdSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, IIdAtom * _id);
    ~IdSyntaxTree();

protected:
    IdSyntaxTree(ECLlocation _pos, IIdAtom * _id);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void appendSTvalue(StringBuffer & str);

protected:
    IIdAtom * id;
};
//--------------------------------------------------------------------------------------------------------

inline ISyntaxTree * createSyntaxTree(){ return SyntaxTree::createSyntaxTree(); }
inline ISyntaxTree * createSyntaxTree(const ECLlocation & _pos){ return SyntaxTree::createSyntaxTree(_pos); }

ISyntaxTree * createPuncSyntaxTree(const ECLlocation & _pos, const TokenKind & _token);
ISyntaxTree * createIdSyntaxTree(const ECLlocation & _pos, IIdAtom * _id);
ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, IValue * constant);

#endif
