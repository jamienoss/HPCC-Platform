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
SyntaxTree * SyntaxTree::createSyntaxTree()
{
    return new SyntaxTree();
}

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & token)
{
    return new SyntaxTree(token);
}


SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok)
{
    SyntaxTree * temp = new SyntaxTree(parentTok);
    temp->setLeft(leftTok);
    temp->setRight(rightTok);
    return temp;
}

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch, TokenData & rightTok)
{
    SyntaxTree * temp = new SyntaxTree(parentTok);
    temp->setLeft(leftBranch);
    temp->setRight(rightTok);
    return temp;
}

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch, SyntaxTree * rightBranch)
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

SyntaxTree::SyntaxTree(TokenData & tok)
{
    switch (tok.tokenKind) {
    case ID: name = tok.name; break;
    case INTEGER: integer = tok.integer; break;
    case REAL: real = tok.real; break;
    }

    token = tok.tokenKind;
    pos.set(*tok.pos);
    delete tok.pos;
	right = NULL;
}

SyntaxTree::~SyntaxTree()
{
     if (right)
         delete right;
}

void SyntaxTree::setLeft(SyntaxTree * node)
{
    left.setown(node);
}

void SyntaxTree::setRight(SyntaxTree * node)
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



bool SyntaxTree::printTree()
{
	int ioStat;
	unsigned parentNodeNum = 0, nodeNum = 0;
	StringBuffer str;
    Owned<IFile> treeFile = createIFile(((std::string)pos.sourcePath->str()).append(".dot").c_str());
    Owned<IFileIO> io = treeFile->open(IFOcreaterw);
    Owned<IFileIOStream> out = createIOStream(io);

	str = "graph \"Abstract Syntax Tree\"\n{\n";
	out->write(str.length(), str.str());

	ioStat = printBranch(& parentNodeNum, & nodeNum, out);

	str = "}\n";
	out->write(str.length(), str.str());
	io->close();
	return ioStat;
}

bool  SyntaxTree::printBranch(unsigned * parentNodeNum, unsigned * nodeNum, Owned<IFileIOStream> & out)
{
	bool ioStatL;
	bool ioStatR;
	unsigned parentNodeNumm = *nodeNum;

	printNode(nodeNum, out);

	if (left)
	{
		printEdge(parentNodeNumm, *nodeNum, out);
		ioStatL = left->printBranch(parentNodeNum, nodeNum, out);
	}

    ForEachItemIn(i,children)
    {
       printEdge(parentNodeNumm, *nodeNum, out);
       children.item(i).printBranch(parentNodeNum, nodeNum, out);
    }

	if (right)
	{
		printEdge(parentNodeNumm, *nodeNum, out);
		ioStatR = right->printBranch(parentNodeNum, nodeNum, out);
	}

	return !(ioStatL && ioStatR);
}

bool SyntaxTree::printEdge(unsigned parentNodeNum, unsigned nodeNum, Owned<IFileIOStream> & out)
{
    StringBuffer str;
    str.append(parentNodeNum).append(" -- ").append(nodeNum).append(" [style = solid]\n");
    out->write(str.length(), str.str());
	return true;
}

bool SyntaxTree::printNode(unsigned * nodeNum, Owned<IFileIOStream> & out)
{

    StringBuffer str;
    str.append(*nodeNum).append(" [label = \"");

    TokenKind kind = token;
	switch(kind){
    case INTEGER : str.append(integer); break;
    case REAL: str.append(real); break;
    case ID : str.append(name->str()); break;
    default: appendParserTokenText(str, kind); break;
	}

    str.append("\\nLine: ").append(pos.lineno);
    str.append("\\nCol: ").append(pos.column);
    str.append("\\nPos: ").append(pos.position).append("\" style=filled, color=");

    switch(kind)
	{
    case INTEGER:
    case REAL:
        str.append("\"0.66,0.5,1\"");
        break;
    default:
        str.append("\"0.25,0.5,1\"");
        break;
	}
	str.append("]\n");
	out->write(str.length(), str.str());

	(*nodeNum)++;
	return true;
}

void SyntaxTree::addChild(SyntaxTree * addition) //MORE: Should maybe use vectors here, talk to Gavin.
{
    children.append(*addition);
    addition = NULL;
}

void SyntaxTree::transferChildren(SyntaxTree * node) // come up with a better name!!!
{
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
