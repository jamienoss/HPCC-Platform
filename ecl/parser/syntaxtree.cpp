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
#include "eclgram.h"

//----------------------------------SyntaxTree--------------------------------------------------------------------

ISyntaxTree * SyntaxTree::createSyntaxTree()
{
    return new SyntaxTree();
}

ISyntaxTree * SyntaxTree::createSyntaxTree(TokenData & token)
{
    if (token.tokenKind == INTEGER)
        return new IntegerSyntaxTree(token);
    return new SyntaxTree(token);
}


ISyntaxTree * SyntaxTree::createSyntaxTree(TokenKind token, const ECLlocation & pos)
{
    return new SyntaxTree(token, pos);
}

ISyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok)
{
    SyntaxTree * temp = new SyntaxTree(parentTok);
    temp->setLeft(leftTok);
    temp->setRight(rightTok);
    return temp;
}

ISyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch, TokenData & rightTok)
{
    SyntaxTree * temp = new SyntaxTree(parentTok);
    temp->setLeft(leftBranch);
    temp->setRight(rightTok);
    return temp;
}

ISyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch)
{
    SyntaxTree * temp = new SyntaxTree(parentTok);
    temp->setLeft(leftBranch);
    return temp;
}

ISyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parentTok, ISyntaxTree * leftBranch, ISyntaxTree * rightBranch)
{
    SyntaxTree * temp =  new SyntaxTree(parentTok);
    temp->setLeft(leftBranch);
    temp->setRight(rightBranch);
    return temp;
}


SyntaxTree::SyntaxTree()
{
    token = 0;
    right = NULL;
}

SyntaxTree::SyntaxTree(TokenKind _token, const ECLlocation & _pos)
{
    token = _token;
    pos.set(_pos);
    right = NULL;
}

SyntaxTree::SyntaxTree(TokenData & tok)
{
    switch (tok.tokenKind) {
    case ID: name = tok.name; break;
    case REAL: real = tok.real; break;
    }

    token = tok.tokenKind;
    pos.set(tok.pos);
    right = NULL;
}

SyntaxTree::~SyntaxTree()
{
     if (right)
         delete right;
}

void SyntaxTree::setLeft(ISyntaxTree * node)
{
    left.setown(node);
}

void SyntaxTree::setRight(ISyntaxTree * node)
{
    right = node;
}

void SyntaxTree::setLeft(TokenData & token)
{
    left.setown(createSyntaxTree(token));
}

void SyntaxTree::setRight(TokenData & token)
{
    right = createSyntaxTree(token);
}


SyntaxTree * SyntaxTree::queryPrivateChild(unsigned i)
{
    //This is valid because all implementations of ISyntaxTree are implemented by the SyntaxTree class
    return static_cast<SyntaxTree *>(queryChild(i));
}


bool SyntaxTree::printTree()
{
    int ioStat;
    unsigned parentNodeNum = 0, nodeNum = 0;
    StringBuffer str;
    Owned<IFile> treeFile = createIFile(((std::string)pos.sourcePath->str()).append(".dot").c_str());
    Owned<IFileIO> io = treeFile->open(IFOcreaterw);
    Owned<IIOStream> out = createIOStream(io);

    str = "graph \"Abstract Syntax Tree\"\n{\n";
    out->write(str.length(), str.str());

    ioStat = printBranch(& parentNodeNum, & nodeNum, out);

    str = "}\n";
    out->write(str.length(), str.str());
    io->close();
    return ioStat;
}

bool  SyntaxTree::printBranch(unsigned * parentNodeNum, unsigned * nodeNum, IIOStream * out)
{
    bool ioStatL;
    bool ioStatR;
    unsigned parentNodeNumm = *nodeNum;

    printNode(nodeNum, out);

    if (left)
    {
        printEdge(parentNodeNumm, *nodeNum, out);
        ioStatL = (static_cast<SyntaxTree *>(left.get()))->printBranch(parentNodeNum, nodeNum, out); // yuk!
    }

    ForEachItemIn(i,children)
    {
       printEdge(parentNodeNumm, *nodeNum, out);
       queryPrivateChild(i)->printBranch(parentNodeNum, nodeNum, out);
    }

    if (right)
    {
        printEdge(parentNodeNumm, *nodeNum, out);
        ioStatR = (static_cast<SyntaxTree *>(right))->printBranch(parentNodeNum, nodeNum, out);
    }

    return !(ioStatL && ioStatR);
}

bool SyntaxTree::printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out)
{
    StringBuffer str;
    str.append(parentNodeNum).append(" -- ").append(nodeNum).append(" [style = solid]\n");
    out->write(str.length(), str.str());
    return true;
}

bool SyntaxTree::printNode(unsigned * nodeNum, IIOStream * out, const char * text, const char * colour)
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
    return true;
}

bool SyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
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

    return printNode(nodeNum, out, text, colour);
}

void SyntaxTree::printXml(StringBuffer & out)
{
    const char * type = "?";
    out.append("<st kind=\'").append(type).append("\' ");

    if (children.ordinality())
    {
        out.append(">").newline();
        ForEachItemIn(i, children)
            children.item(i).printXml(out);
        out.append("</st>");
    }
    else
        out.append("/>").newline();
}

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

void SyntaxTree::transferChildren(ISyntaxTree * _node) // come up with a better name!!!
{
    SyntaxTree * node = static_cast<SyntaxTree *>(_node);   // justification is that interface is being used to implement an opaque type
    ForEachItemIn(i,node->children)
        children.append(*LINK(&node->children.item(i)));

/*
    if(aux) {
        std::cout << "hmmm, aux is already pointing to something. Maybe you meant to use add2Aux?\n";
        return;
    }

    auxLength = node->getAuxLength();
    aux = node->releaseAux();
    delete node;
    */
}

TokenKind SyntaxTree::getKind()
{
    return token;
}

//----------------------------------SyntaxTree--------------------------------------------------------------------

IntegerSyntaxTree::IntegerSyntaxTree(TokenData & tok) : SyntaxTree(tok)
{
    value = tok.integer;
}


bool IntegerSyntaxTree::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    text.append(value);
    return SyntaxTree::printNode(nodeNum, out, text, "\"0.66,0.5,1\"");
}
