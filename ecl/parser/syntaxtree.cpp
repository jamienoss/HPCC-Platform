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


SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & token, SyntaxTree * tempAux)
{
    SyntaxTree * temp = new SyntaxTree(token);
    temp->transferChildren(tempAux);
    return temp;
}


SyntaxTree::SyntaxTree()
{
    attributes.attributeKind = none;
    attributes.pos = NULL;
    left = NULL;
    right = NULL;
    children = NULL;
}

SyntaxTree::SyntaxTree(TokenData & token)
{
	attributes.cpy(token);
	left = NULL;
	right = NULL;
    children = NULL;
}

SyntaxTree::~SyntaxTree()
{
     if (left)
         delete left;
     if (right)
         delete right;

     if(children)
         delete children;

     if (attributes.attributeKind == lexemeKind)
        delete[] attributes.lexeme;

}

SyntaxTree * SyntaxTree::release()
{
	SyntaxTree * temp = this;
//	this = NULL;
	return temp;
}

void SyntaxTree::setLeft(SyntaxTree * node)
{
    left = node;
    node = NULL;
}

void SyntaxTree::setRight(SyntaxTree * node)
{
    right = node;
    node = NULL;
}

void SyntaxTree::setLeft(TokenData & token)
{
    left = createSyntaxTree(token);
}

void SyntaxTree::setRight(TokenData & token)
{
    right = createSyntaxTree(token);
}

void SyntaxTree::bifurcate(SyntaxTree * leftBranch, TokenData & rightTok)
{
	left = leftBranch;
	setRight(rightTok);
}

void SyntaxTree::bifurcate(TokenData & leftTok, TokenData & rightTok)
{
	setLeft(leftTok);
	setRight(rightTok);
}

void SyntaxTree::bifurcate(SyntaxTree * leftBranch, SyntaxTree * rightBranch)
{
	left = leftBranch;
	right = rightBranch;
}





bool SyntaxTree::printTree()
{
	int ioStat;
	unsigned parentNodeNum = 0, nodeNum = 0;
	StringBuffer str;
	Owned<IFile> treeFile = createIFile(((std::string)attributes.pos->sourcePath->str()).append(".dot").c_str());
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

	if(children)
	{
	    ForEachItemIn(i,*children)
	            {
	               printEdge(parentNodeNumm, *nodeNum, out);
	               children->item(i).printBranch(parentNodeNum, nodeNum, out);
	            }
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

	symbolKind kind = attributes.attributeKind;
	switch(kind){
	case integerKind : str.append(attributes.integer); break;
	case realKind : str.append(attributes.real); break;
	case lexemeKind : str.append(attributes.lexeme); break;
	default : str.append("KIND not yet defined!"); break;
	}

	str.append("\\nLine: ").append(attributes.pos->lineno);
	str.append("\\nCol: ").append(attributes.pos->column);
	str.append("\\nPos: ").append(attributes.pos->position).append("\" style=filled, color=");
	switch(kind)
	{
    case integerKind : str.append("\"0.66,0.5,1\""); break;
	case lexemeKind : str.append("\"0.25,0.5,1\""); break;
	}
	str.append("]\n");
	out->write(str.length(), str.str());

	(*nodeNum)++;
	return true;
}

void SyntaxTree::addChild(SyntaxTree * addition) //MORE: Should maybe use vectors here, talk to Gavin.
{
    if(!children)
            children = new CIArrayOf<SyntaxTree>;

    children->append(*addition);
    addition = NULL;
}

void SyntaxTree::transferChildren(SyntaxTree * node) // come up with a better name!!!
{
    children = node->children;
    node->children = NULL;
    delete node;
    return;

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

symbolKind SyntaxTree::getKind()
{
    return attributes.attributeKind;
}
