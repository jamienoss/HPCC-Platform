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

class Printer
{
public :
    int id;
    int indent;
    StringBuffer * str;

    Printer() { indent = 0; id = 0; str = NULL; }
    Printer(int _indent, StringBuffer * _str) { indent = _indent; id = 0; str = _str; }
    ~Printer() {}

    StringBuffer & indentation()
    {
        for (unsigned short i = 0; i < indent; ++i)
            str->append("  ");
        return *str;
    }
    inline void tabIncrease() { ++indent; ++id; }
    inline void tabDecrease() { --indent; }
};

//----------------------------------SyntaxTree--------------------------------------------------------------------
interface ISyntaxTree : public IInterface
{
    //virtual TokenKind getKind() = 0;
    virtual const ECLlocation & queryPosition() const = 0;
    virtual void printTree() = 0;
    virtual void printXml(Printer * print) = 0;
    virtual void appendSTvalue(StringBuffer & str) = 0;

    virtual ISyntaxTree * queryChild(unsigned i) = 0;

    virtual void addChild(ISyntaxTree * addition) = 0;
    //virtual void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out) = 0;
    virtual void extractSymbols(std::vector <std::string> & symbolList, TokenKind & kind) = 0;
};

typedef IArrayOf<ISyntaxTree> SyntaxTreeArray;

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
    virtual void addChild(ISyntaxTree * addition);

    //virtual TokenKind getKind();
    virtual const ECLlocation & queryPosition() const { return pos; }
    virtual void appendSTvalue(StringBuffer & str);

protected:
    virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);
    virtual void printNode(unsigned * nodeNum,  IIOStream * out);
    void printNode(unsigned * nodeNum, IIOStream * out, const char * text, const char * colour);

    virtual void extractSymbols(std::vector <std::string> & symbolList, TokenKind & kind);

    SyntaxTree * queryPrivateChild(unsigned i);

protected:
    ECLlocation pos;
    SyntaxTreeArray children;

protected:
    SyntaxTree();
    SyntaxTree(ECLlocation _pos);
};

/// The following code can now be moved and hidden ---
//MORE: the following below might need parameterless constructors and 'create' wrappers
//--------------------------------------------------------------------------------------------------------
class PuncSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const TokenKind & _token);

private:
    PuncSyntaxTree(ECLlocation _pos, TokenKind _token);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void appendSTvalue(StringBuffer & str);

private:
    TokenKind value;
};

//--------------------------------------------------------------------------------------------------------
class ConstantSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const int & constant);
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const double & constant);
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const bool & constant);

private:
    ConstantSyntaxTree(ECLlocation _pos, const int & constant);
    ConstantSyntaxTree(ECLlocation _pos, const double & constant);
    ConstantSyntaxTree(ECLlocation _pos, const bool & constant);

    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void appendSTvalue(StringBuffer & str);

private:
    OwnedIValue value;
};
//--------------------------------------------------------------------------------------------------------
class IdSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, IIdAtom * _name);

private:
    IdSyntaxTree(ECLlocation _pos, IIdAtom * _name);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    virtual void appendSTvalue(StringBuffer & str);

private:
    IIdAtom * name;
};
//--------------------------------------------------------------------------------------------------------

inline ISyntaxTree * createSyntaxTree(){ return SyntaxTree::createSyntaxTree(); }
inline ISyntaxTree * createSyntaxTree(const ECLlocation & _pos){ return SyntaxTree::createSyntaxTree(_pos); }

ISyntaxTree * createPuncSyntaxTree(const ECLlocation & _pos, const TokenKind & _token);
ISyntaxTree * createIdSyntaxTree(const ECLlocation & _pos, IIdAtom * _name);
ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, const bool & constant);
ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, const int & constant);
ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, const double & constant);

#endif
