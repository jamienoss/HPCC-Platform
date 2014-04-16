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
    virtual ISyntaxTree * getNode() = 0;
};

class IdTableItem : public CInterfaceOf<IIdTableItem>
{
public :
    IdTableItem();
    IdTableItem(IIdAtom * id);
    IdTableItem(ISyntaxTree * _node);
    IdTableItem(IIdAtom * id, ISyntaxTree * _node);

    ~IdTableItem();

    virtual const char * getIdName();
    virtual ISyntaxTree * getNode() { return LINK(node); }

protected :
    IIdAtom * symbol;
    Owned<ISyntaxTree> node;
};
typedef  IArrayOf<IIdTableItem> IdTable;

//----------------------------------Printer--------------------------------------------------------------------
class Printer
{
public :
    Printer() { indentation = 0; id = 0; str = NULL; }
    Printer(int _indentation, StringBuffer _str) { indentation = _indentation; id = 0; str = _str; }
    ~Printer() {}

    StringBuffer & indent()
    {
        for (unsigned short i = 0; i < indentation; ++i)
            str.append("  ");
        return str;
    }
    inline void tabIncrease() { ++indentation; ++id; }
    inline void tabDecrease() { --indentation; }
    inline StringBuffer & queryStr() { return str; }
    inline int queryId() { return id; }

protected :
    int id;
    int indentation;
    StringBuffer str;
};

//----------------------------------ITreeWalker--------------------------------------------------------------------
interface ITreeWalker : public IInterface
{
    virtual bool action(ISyntaxTree * node) = 0;
    virtual bool keepWalking() = 0;
};
//-----------------------------------------------------------------------------------------------------------------
class TreeWalker : public CInterfaceOf<ITreeWalker>
{
public :
    TreeWalker(bool _continueWalk) { continueWalk = _continueWalk; }
    TreeWalker() { TreeWalker(false); }
    virtual bool action(ISyntaxTree * node) { return false; }
    virtual bool keepWalking() { return continueWalk; }

protected :
    bool continueWalk;
};
//-----------------------------------------------------------------------------------------------------------------
class NodeFinder : public TreeWalker
{
public :
    NodeFinder(ISyntaxTree * _node2find, bool _continueWalk) { node2find = _node2find; continueWalk = _continueWalk; }
    virtual bool action(ISyntaxTree * node) { return (node == node2find); }
    virtual bool keepWalking() { return continueWalk; }

protected :
    ISyntaxTree * node2find;

};
//-----------------------------------------------------------------------------------------------------------------

typedef IArrayOf<ISyntaxTree> SyntaxTreeArray;
//----------------------------------SyntaxTree--------------------------------------------------------------------
interface ISyntaxTree : public IInterface
{
    virtual TokenKind queryKind() = 0;
    virtual const ECLlocation & queryPosition() const = 0;
    virtual const char * queryIdName() = 0;

    virtual bool alreadyVisited() = 0;
    virtual void printTree() = 0;
    virtual void printXml(Printer * print) = 0;
    virtual void appendSTvalue(StringBuffer & str) = 0;

    virtual ISyntaxTree * queryChild(unsigned i) = 0;
    virtual ISyntaxTree * getChild(unsigned i) = 0;
    virtual const SyntaxTreeArray & queryChildren() const = 0;
    virtual SyntaxTreeArray * getChildren() = 0;
    virtual unsigned numberOfChildren() = 0;
    virtual void addChild(ISyntaxTree * addition) = 0;
    virtual void transferChildren(ISyntaxTree * addition) = 0;

    //virtual void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out) = 0;
    virtual void createIdNameList(IdTable & symbolList, TokenKind _kind) = 0;
    virtual void setIdNameList(IdTable * symbolList) = 0;
    virtual void printIdNameList() = 0;
    virtual IdTable * queryIdTable() = 0;
    virtual IIdTableItem * queryIdTable(aindex_t pos) = 0;

    virtual void setRevisit(bool _revisit) = 0;
    virtual ISyntaxTree * walkTree(ITreeWalker & walker) = 0;
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
    virtual bool alreadyVisited() { return false; }

    virtual ISyntaxTree * queryChild(unsigned i);
    virtual ISyntaxTree * getChild(unsigned i);
    virtual const SyntaxTreeArray & queryChildren() const;
    virtual SyntaxTreeArray * getChildren() { return &children; };
    virtual unsigned numberOfChildren() { return children.ordinality(); }
    virtual void addChild(ISyntaxTree * addition);
    virtual void transferChildren(ISyntaxTree * addition);

    virtual TokenKind queryKind() { return 0; }
    virtual const ECLlocation & queryPosition() const { return pos; }
    virtual const char * queryIdName() { return NULL; }
    virtual void appendSTvalue(StringBuffer & str);

    virtual void setRevisit(bool _revisit);
    virtual ISyntaxTree * walkTree(ITreeWalker & walker);


protected:
    virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);
    virtual void printNode(unsigned * nodeNum,  IIOStream * out);
    void printNode(unsigned * nodeNum, IIOStream * out, const char * text, const char * colour);

    virtual void createIdNameList(IdTable & symbolList, TokenKind _kind);
    virtual void setIdNameList(IdTable * symbolList) {};
    virtual void printIdNameList() {};
    virtual IdTable * queryIdTable() { return NULL; }
    virtual IIdTableItem * queryIdTable(aindex_t pos) { return NULL; }

    SyntaxTree * queryPrivateChild(unsigned i);

protected:
    ECLlocation pos;
    SyntaxTreeArray children;
    bool revisit;

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
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, TokenKind _token);
    virtual TokenKind queryKind() { return value; };

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
    virtual const char * queryIdName() { return id->str(); }
    virtual const char * value2str() { return queryIdName(); }

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

ISyntaxTree * createPuncSyntaxTree(const ECLlocation & _pos, TokenKind _token);
ISyntaxTree * createIdSyntaxTree(const ECLlocation & _pos, IIdAtom * _id);
ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, IValue * constant);

#endif
