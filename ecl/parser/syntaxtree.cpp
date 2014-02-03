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

SyntaxTree::SyntaxTree(ECLlocation _pos) { pos.set(_pos); }

SyntaxTree::~SyntaxTree() {}
//-------------------------------------------------------------------------------------------------------------------


SyntaxTree * SyntaxTree::queryPrivateChild(unsigned i)
{
    //This is valid because all implementations of ISyntaxTree are implemented by the SyntaxTree class
    return static_cast<SyntaxTree *>(queryChild(i));
}

void SyntaxTree::printTree()
{
    bool XML = 0;

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
        print * printer = new print(0, &str);
        printer->str->append("<graph>").newline();
        //printXml(printer);
        //printGEXF(printer);
        printer->str->append("</graph>");

        out->write(printer->str->length(), printer->str->str());
        io->close();
        delete printer;
        return;
    }

    //printing dot
    str = "graph \"Abstract Syntax Tree\"\n{\n";
    out->write(str.length(), str.str());

    printBranch(& parentNodeNum, & nodeNum, out);

    str = "}\n";
    out->write(str.length(), str.str());
    io->close();
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
    /*StringBuffer text;
    switch(token){
    case REAL: text.append(real); break;
    case ID : text.append(name->str()); break;
    default: appendParserTokenText(text, token); break;
    }

    const char * colour = NULL;
    switch(token)
    {
    case INTEGER:
    case REAL:
        colour = "\"0.66,0.5,1\"";
        break;
    default:
        colour = "\"0.25,0.5,1\"";
        break;
    }

    printNode(nodeNum, out, text, colour);
}

/*void SyntaxTree::printXml(print * printer)
{

    printer->indentation().append("<node id=\"").append(printer->id).append("\" label=\"");
    appendParserTokenText(*printer->str, token);
    printer->str->append("\" "); // add extra details here

    if (children.ordinality())
    {
        printer->str->append(">").newline();
        ForEachItemIn(i, children)
        {
            printer->tabIncrease();
            children.item(i).printXml(printer);
            printer->tabDecrease();
        }
        printer->indentation().append("</node>").newline();
    }
    else
    {
        printer->str->append("/>").newline();
    }
}*/

void SyntaxTree::addChild(ISyntaxTree * addition) //MORE: Should maybe use vectors here, talk to Gavin.
{
    children.append(*addition);
    addition = NULL;
}

ISyntaxTree * SyntaxTree::queryChild(unsigned i)
{
    if (children.isItem(i))
        return & children.item(i);
    return NULL;
}

/*TokenKind SyntaxTree::getKind()
{
    return token;
}*/

void SyntaxTree::extractSymbols(std::vector <std::string> & symbolList, TokenKind & kind) {}







//----------------------------------IntegerSyntaxTree--------------------------------------------------------------------
ISyntaxTree * IntSyntaxTree::createSyntaxTree(const ECLlocation & _pos, const int & constant)
{
    return new IntSyntaxTree(_pos, constant);
}

IntSyntaxTree::IntSyntaxTree(ECLlocation _pos, int constant) : SyntaxTree(_pos)
{
    value = constant;
}

void IntSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    text.append(value);
    SyntaxTree::printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}
//----------------------------------RealSyntaxTree--------------------------------------------------------------------
ISyntaxTree * RealSyntaxTree::createSyntaxTree(const ECLlocation & _pos, const double & constant)
{
    return new RealSyntaxTree(_pos, constant);
}

RealSyntaxTree::RealSyntaxTree(ECLlocation _pos, double constant) : SyntaxTree(_pos)
{
    value = constant;
}

void RealSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    text.append(value);
    SyntaxTree::printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}
//----------------------------------BooleanSyntaxTree--------------------------------------------------------------------
ISyntaxTree * BoolSyntaxTree::createSyntaxTree(const ECLlocation & _pos, const bool & constant)
{
    return new BoolSyntaxTree(_pos, constant);
}

BoolSyntaxTree::BoolSyntaxTree(ECLlocation _pos, bool constant) : SyntaxTree(_pos)
{
    value = constant;
}

void BoolSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    text.append(value);
    SyntaxTree::printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}
//----------------------------------PunctuationSyntaxTree--------------------------------------------------------------------
ISyntaxTree * PuncSyntaxTree::createSyntaxTree(const ECLlocation & _pos, const TokenKind & constant)
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
    SyntaxTree::printNode(nodeNum, out, text, "\"0.25,0.5,1\"");
}
//----------------------------------IdentifierSyntaxTree--------------------------------------------------------------------
ISyntaxTree * IdSyntaxTree::createSyntaxTree(const ECLlocation & _pos, IIdAtom * _name)
{
    return new IdSyntaxTree(_pos, _name);
}

IdSyntaxTree::IdSyntaxTree(ECLlocation _pos, IIdAtom * _name) : SyntaxTree(_pos)
{
    name = _name;//createIdAtom(tokenText, txtLen); or make owned and link
}

void IdSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    text.append(name->str());
    SyntaxTree::printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}
//------------------------------------------------------------------------------------------------------




/*void IntegerSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    text.append(value);
    SyntaxTree::printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}

void  IntegerSyntaxTree::printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out)
{
    unsigned parentNodeNumm = *nodeNum;

    printNode(nodeNum, out);

    ForEachItemIn(i,children)
    {
       printEdge(parentNodeNumm, *nodeNum, out, i);
       queryPrivateChild(i)->printBranch(parentNodeNum, nodeNum, out);
    }
}*/
