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

#include "syntaxtree.hpp"
#include "jstring.hpp"
#include "jlib.hpp"
#include "jfile.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include "eclgram.h" // needed for correct mapping of YYTokenName


//----------------------------------SyntaxTree--------------------------------------------------------------------
ISyntaxTree * SyntaxTree::createSyntaxTree() { return new SyntaxTree(); }
ISyntaxTree * SyntaxTree::createSyntaxTree(const ECLlocation & _pos) { return new SyntaxTree(_pos); }

SyntaxTree::SyntaxTree() { pos.clear(); }
SyntaxTree::SyntaxTree(const ECLlocation & _pos) { pos.set(_pos); }
SyntaxTree::~SyntaxTree() {}
//-------------------------------------------------------------------------------------------------------------------

SyntaxTree * SyntaxTree::queryPrivateChild(unsigned i)
{
    //This is valid because all implementations of ISyntaxTree are implemented by the SyntaxTree class
    return static_cast<SyntaxTree *>(queryChild(i));
}

void SyntaxTree::printTree()
{
    bool XML = 0;//NOTE/MORE: setup to always write both?

    unsigned parentNodeNum = 0, nodeNum = 0;
    StringBuffer str;
    StringBuffer fileName;

    fileName.append(pos.sourcePath->str());

    if(XML)
        fileName.append(".xgmml");
    else
        fileName.append(".dot");

    Owned<IFile> treeFile = createIFile(fileName.str());
    Owned<IFileIO> io = treeFile->open(IFOcreaterw);
    Owned<IIOStream> out = createIOStream(io);

    if(XML)
    {
        Printer * print = new Printer(0, str);
        print->queryStr().append("<graph>").newline();
        printXml(print);
        //printGEXF(printer);
        print->queryStr().append("</graph>");

        out->write(print->queryStr().length(), print->queryStr().str());
        io->close();
        delete print;
        return;
    }

    //printing dot
    //str = "graph \"Abstract Syntax Tree\"\n{\n";
    str = ("graph \"Abstract Syntax Tree\"\n{\nordering=out\n");
    out->write(str.length(), str.str());

    printBranch(& parentNodeNum, & nodeNum, out);

    str = "}\n";
    out->write(str.length(), str.str());
    io->close();

    StringBuffer runDot = "dot -Tpdf -O ";
    runDot.append(fileName);
    system(runDot.str());
}

void SyntaxTree::printXml(Printer * print)
{

    print->indent().append("<node id=\"").append(print->queryId()).append("\" label=\"");
    appendSTvalue(print->queryStr());

    //appendParserTokenText(*print->str, token);
    print->queryStr().append("\" "); // add extra details here

    if (children.ordinality())
    {
        print->queryStr().append(">").newline();
        ForEachItemIn(i, children)
        {
            print->tabIncrease();
            children.item(i).printXml(print);
            print->tabDecrease();
        }
        print->indent().append("</node>").newline();
    }
    else
    {
        print->queryStr().append("/>").newline();
    }
}

void  SyntaxTree::printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out)
{
    unsigned parentNodeNumm = *nodeNum;

    printNode(nodeNum, out);

    ForEachItemIn(i,children)
    {
       printEdge(parentNodeNumm, *nodeNum, out, i);
       queryPrivateChild(i)->printBranch(parentNodeNum, nodeNum, out);
    }
}

void SyntaxTree::printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx)
{
    StringBuffer str;
    str.append(parentNodeNum).append(" -- ").append(nodeNum).append(" [style = solid]\n");
    out->write(str.length(), str.str());
}

void SyntaxTree::printNode(unsigned * nodeNum, IIOStream * out, const char * text, const char * colour)
{
    StringBuffer str;
    str.append(*nodeNum).append(" [label = \"");
    str.append(text);
    str.append("\\nLine: ").append(pos.lineno);
    str.append("\\nCol: ").append(pos.column);
    str.append("\\nPos: ").append(pos.position).append("\" style=filled, color=");
    str.append(colour);
    str.append("]\n");
    out->write(str.length(), str.str());

    (*nodeNum)++;
}

void SyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    text.append("Empty Node!!!");
    printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}

void SyntaxTree::addChild(ISyntaxTree * addition)
{
    children.append(*LINK(addition));
}

void SyntaxTree::transferChildren(ISyntaxTree * addition)
{
    ForEachItemIn(i,addition->queryChildren())
	{
        children.append(*addition->getChild(i));
	}
}

const SyntaxTreeArray & SyntaxTree::queryChildren() const
{
	return children;
}

ISyntaxTree * SyntaxTree::queryChild(unsigned i)
{
    if (children.isItem(i))
        return & children.item(i);
    return NULL;
}

ISyntaxTree * SyntaxTree::getChild(unsigned i)
{
    if (children.isItem(i))
        return  LINK(&children.item(i));
    return NULL;
}

void SyntaxTree::appendSTvalue(StringBuffer & str)
{
    str.append("Empty tree node!!!");
}

void SyntaxTree::createIdNameList(IdTable & symbolList, TokenKind _kind)
{
    if(children.ordinality())
    {
       ForEachItemIn(i, children)
           children.item(i).createIdNameList(symbolList, _kind);
    }
}

ISyntaxTree * SyntaxTree::walkTree(ITreeWalker & walker)
{
    if(walker.test(this))
        return LINK(this);

    ISyntaxTree * found = NULL;
    if(children.ordinality())
    {
        ForEachItemIn(i, children)
        {
            found = children.item(i).walkTree(walker);
            if(found)
                return found;
        }
    }
    return found;
}
//----------------------------------ConstantSyntaxTree--------------------------------------------------------------------
ISyntaxTree * ConstantSyntaxTree::createSyntaxTree(const ECLlocation & _pos, IValue * constant)
{
    return new ConstantSyntaxTree(_pos, constant);
}

ConstantSyntaxTree::ConstantSyntaxTree(ECLlocation _pos, IValue * constant) : SyntaxTree(_pos)
{
    value.set(constant);
}

void ConstantSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    appendSTvalue(text);
    SyntaxTree::printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}

void ConstantSyntaxTree::appendSTvalue(StringBuffer & str)
{

    //return;

    switch(value->getTypeCode())
    {
    case type_int : str.append(value->getIntValue());break;
    case type_decimal :
    case type_real : str.append(value->getRealValue());break;
    case type_boolean : str.append(value->getBoolValue());break;
    //default : str.append(value);
    }
}
//----------------------------------PunctuationSyntaxTree--------------------------------------------------------------------
ISyntaxTree * PuncSyntaxTree::createSyntaxTree(const ECLlocation & _pos, TokenKind constant)
{
    return new PuncSyntaxTree(_pos, constant);
}

PuncSyntaxTree::PuncSyntaxTree(ECLlocation _pos, TokenKind constant) : SyntaxTree(_pos)
{
    value = constant;
}

void PuncSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    appendParserTokenText(text, value);
    //text.append(value);
    if(value < 256)
        SyntaxTree::printNode(nodeNum, out, text, "\"0.25,0.5,1\"");
    else
        SyntaxTree::printNode(nodeNum, out, text, "\"1.0,0.5,1\"");
}

void PuncSyntaxTree::appendSTvalue(StringBuffer & str)
{
    appendParserTokenText(str, value);
}

//----------------------------------IdentifierSyntaxTree--------------------------------------------------------------------
ISyntaxTree * IdSyntaxTree::createSyntaxTree(const ECLlocation & _pos, IIdAtom * _id)
{
    return new IdSyntaxTree(_pos, _id);
}

IdSyntaxTree::IdSyntaxTree(ECLlocation _pos, IIdAtom * _id) : SyntaxTree(_pos)
{
    id = LINK(_id);//createIdAtom(tokenText, txtLen); or make owned and link
}

IdSyntaxTree::~IdSyntaxTree() { id->Release(); }

void IdSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    text.append(id->str());
    SyntaxTree::printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}

void IdSyntaxTree::appendSTvalue(StringBuffer & str)
{
    str.append(id->str());
}
//------------------------------------------------------------------------------------------------------

ISyntaxTree * createPuncSyntaxTree(const ECLlocation & _pos, TokenKind _token)
{
    return PuncSyntaxTree::createSyntaxTree(_pos, _token);
}

ISyntaxTree * createIdSyntaxTree(const ECLlocation & _pos, IIdAtom * _id)
{
    return IdSyntaxTree::createSyntaxTree(_pos, _id);
}

ISyntaxTree * createConstSyntaxTree(const ECLlocation & _pos, IValue * constant)
{
    return ConstantSyntaxTree::createSyntaxTree(_pos, constant);
}
//------------------------------------------------------------------------------------------------------
IdTableItem::IdTableItem() { symbol = NULL; }
IdTableItem::IdTableItem(const IIdAtom & id) { symbol = createIdAtom(id); }
IdTableItem::IdTableItem(ISyntaxTree * _node) { symbol = NULL; node.set(_node); }
IdTableItem::IdTableItem(const IIdAtom & id, ISyntaxTree * _node) { symbol = createIdAtom(id); node.set(_node); }

IdTableItem::~IdTableItem()
{
    symbol->Release();
//Release(*node);
}
const char * IdTableItem::getIdName()
{
    return symbol->str();
}
//------------------------------------------------------------------------------------------------------
