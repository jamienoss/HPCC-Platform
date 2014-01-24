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

#include "parserdata.hpp"
#include"jlib.hpp"
#include <vector>
#include <string>

class print
{
public :
    int id;
    int indent;
    StringBuffer * str;

    print() { indent = 0; id = 0; str = NULL; }
    print(int _indent, StringBuffer * _str) { indent = _indent; id = 0; str = _str; }
    ~print() {}

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
    //virtual void printXml(print * printer) = 0;

    virtual ISyntaxTree * queryChild(unsigned i) = 0;

    virtual void addChild(ISyntaxTree * addition) = 0;
    //virtual void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out) = 0;
    virtual void extractSymbols(std::vector <std::string> & symbolList, TokenKind & kind) = 0;
};

typedef IArrayOf<ISyntaxTree> SyntaxTreeArray;

/// The following code can now be moved and hidden ---
class SyntaxTree : public CInterfaceOf<ISyntaxTree>
{
public:
    static ISyntaxTree * createSyntaxTree();
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos);
    ~SyntaxTree();

//Implementation of ISyntaxTree
    virtual void printTree();
    //virtual void printXml(print * printer);
    void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out);

    virtual ISyntaxTree * queryChild(unsigned i);
    virtual void addChild(ISyntaxTree * addition);

    //virtual TokenKind getKind();
    virtual const ECLlocation & queryPosition() const { return pos; }

protected:
    virtual void printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx);
    virtual void printNode(unsigned * nodeNum,  IIOStream * out);
    void printNode(unsigned * nodeNum, IIOStream * out, const char * text, const char * colour);

    virtual void extractSymbols(std::vector <std::string> & symbolList, TokenKind & kind);

    SyntaxTree * queryPrivateChild(unsigned i);

public: //make protected again
   /* TokenKind token;
   union
    {
        float real;
        IIdAtom * name;
    };
    */
    ECLlocation pos;

    SyntaxTreeArray children;

protected:
    SyntaxTree();
    SyntaxTree(ECLlocation _pos);
};


//MORE: the following below might need parameterless constructors and 'create' wrappers
//--------------------------------------------------------------------------------------------------------
class PuncSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const TokenKind & _token);

//private:
    PuncSyntaxTree(ECLlocation _pos, TokenKind _token);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);
    //virtual void printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out);

public: //private
    TokenKind value;
};

//--------------------------------------------------------------------------------------------------------

class IntSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const int & constant);

//private:
    IntSyntaxTree(ECLlocation _pos, int constant);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);

public: //private
    int value;
};

//--------------------------------------------------------------------------------------------------------
class RealSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const double & constant);

//private:
    RealSyntaxTree(ECLlocation _pos, double constant);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);

public: //private
    double value;
};

//--------------------------------------------------------------------------------------------------------

class BoolSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, const bool & constant);

//private:
    BoolSyntaxTree(ECLlocation _pos, bool constant);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);

public: //private
    bool value;
};

//--------------------------------------------------------------------------------------------------------
class IdSyntaxTree : public SyntaxTree
{
public:
    static ISyntaxTree * createSyntaxTree(const ECLlocation & _pos, IIdAtom * _name);

//private:
    IdSyntaxTree(ECLlocation _pos, IIdAtom * _name);
    virtual void printNode(unsigned * nodeNum, IIOStream * out);

public: //private
    IIdAtom * name;
};

//--------------------------------------------------------------------------------------------------------

















inline ISyntaxTree * createSyntaxTree(){ return SyntaxTree::createSyntaxTree(); }
inline ISyntaxTree * createSyntaxTree(const ECLlocation & _pos){ return SyntaxTree::createSyntaxTree(_pos); }

inline ISyntaxTree * createPuncSyntaxTree(const ECLlocation & _pos, const TokenKind & _token)
{
    return PuncSyntaxTree::createSyntaxTree(_pos, _token);
}

inline ISyntaxTree * createIdSyntaxTree(const ECLlocation & _pos, IIdAtom * _name)
{
    return IdSyntaxTree::createSyntaxTree(_pos, _name);
}

inline ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, const bool & constant)
{
    return BoolSyntaxTree::createSyntaxTree(_pos, constant);
}

inline ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, const int & constant)
{
    return IntSyntaxTree::createSyntaxTree(_pos, constant);
}

inline ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, const double & constant)
{
    return RealSyntaxTree::createSyntaxTree(_pos, constant);
}
#endif
