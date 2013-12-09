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
#include <iostream>
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
    return new SyntaxTree(parentTok, leftTok, rightTok);
}

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch, TokenData & rightTok)
{
    return new SyntaxTree(parentTok, leftBranch, rightTok);
}

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & token, SyntaxTree * tempAux)
{
    return new SyntaxTree(token, tempAux);
}


SyntaxTree::SyntaxTree()
{
	attributes.lineNumber = 0;
    left = NULL;
    right = NULL;
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData & token)
{
	attributes.cpy(token);
	left = NULL;
	right = NULL;
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok)
{
	attributes.cpy(parentTok);
	setLeft(leftTok);
	setRight(rightTok);
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData & parentTok, SyntaxTree * leftBranch, TokenData & rightTok)
{
	attributes.cpy(parentTok);
	setLeft(leftBranch);
	setRight(rightTok);
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData & token, SyntaxTree * tempAux)
{
	attributes.cpy(token);
	left = NULL;
	right = NULL;
	auxLength = tempAux->getAuxLength();
	aux = tempAux->releaseAux();
	delete tempAux;
}

SyntaxTree::~SyntaxTree()
{
     if (left)
         delete left;
     if (right)
         delete right;
     if (aux) {
         for ( int i = 0; i < auxLength; ++i)
         {
             delete aux[i];
         }
         delete[] aux;
     }

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
}

void SyntaxTree::setRight(SyntaxTree * node)
{
    right = node;
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

	std::cout << "graph \"Abstract Syntax Tree\"\n{\n";
	ioStat = printBranch(& parentNodeNum, & nodeNum);
	std::cout << "}\n";
	return ioStat;
}

bool  SyntaxTree::printBranch(unsigned * parentNodeNum, unsigned * nodeNum)
{
	bool ioStatL;
	bool ioStatR;
	unsigned parentNodeNumm = *nodeNum;

	printNode(nodeNum);

	if (left) {
		printEdge(parentNodeNumm, *nodeNum);
		ioStatL = left->printBranch(parentNodeNum, nodeNum);
	}
	if (right) {
		printEdge(parentNodeNumm, *nodeNum);
		ioStatR = right->printBranch(parentNodeNum, nodeNum);
	}

	if (aux) {
		for (int i = 0; i < auxLength; ++i)
		{
			printEdge(parentNodeNumm, *nodeNum);
			aux[i]->printBranch(parentNodeNum, nodeNum);
		}
	}

	return !(ioStatL && ioStatR);
}

bool SyntaxTree::printEdge(unsigned parentNodeNum, unsigned nodeNum)
{
	//StringBuffer text;
	//text.append(parentNodeNum).append(" -- ").append(nodeNum).append(" [style = solid]\n");
	std::cout << parentNodeNum << " -- " << nodeNum << " [style = solid]\n";
	return true;
}

bool SyntaxTree::printNode(unsigned * nodeNum)
{
	std::cout << *nodeNum << " [label = \"";

	symbolKind kind = attributes.attributeKind;
	switch(kind){
	case integerKind : std::cout << attributes.integer; break;
	case realKind : std::cout << attributes.real; break;
	case lexemeKind : std::cout << attributes.lexeme; break;
	default : std::cout << "KIND not yet defined!"; break;
	}

	std::cout << "\\nLine: " << attributes.lineNumber << "\"]\n";
	(*nodeNum)++;
	return true;
}

void SyntaxTree::add2Aux(SyntaxTree * addition) //MORE: Should maybe use vectors here, talk to Gavin.
{
	if (!aux) {
		aux = new SyntaxTree * [1];
	} else	{
		SyntaxTree ** temp;
		temp = new SyntaxTree * [auxLength + 1];
		for(int i = 0; i < auxLength; ++i)
		{
			temp[i] = aux[i];
		}
		delete[] aux; //MORE: maybe recycle these?
		aux = temp;
		temp = NULL;
	}

	aux[auxLength++] = addition;//->release();
	//addition = NULL;
}

SyntaxTree ** SyntaxTree::releaseAux()
{
    SyntaxTree ** temp = aux;
    aux = NULL;
    return temp;
}

unsigned SyntaxTree::getAuxLength()
{
	return auxLength;
}

void SyntaxTree::takeAux(SyntaxTree * node) // come up with a better name!!!
{
	if(aux) {
		std::cout << "hmmm, aux is already pointing to something\n";
		return;
	}

	aux = node->releaseAux();
}

void SyntaxTree::transferChildren(SyntaxTree * node) // come up with a better name!!!
{
    if(aux) {
        std::cout << "hmmm, aux is already pointing to something. Maybe you meant to addto?\n";
        return;
    }

    auxLength = node->getAuxLength();
    aux = node->releaseAux();
    delete node;
}

bool SyntaxTree::isAux()
{
	return aux ? true : false;
}
