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

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & node)
{
    return new SyntaxTree(node);
}

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parent, TokenData & leftTok, TokenData & rightTok)
{
    return new SyntaxTree(parent, leftTok, rightTok);
}

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & parent, SyntaxTree * leftBranch, TokenData & rightTok)
{
    return new SyntaxTree(parent, leftBranch, rightTok);
}

SyntaxTree * SyntaxTree::createSyntaxTree(TokenData & node, SyntaxTree * tempAux)
{
    return new SyntaxTree(node, tempAux);
}

SyntaxTree::SyntaxTree()
{
	attributes.lineNumber = 0;
    left = NULL;
    right = NULL;
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData & node)
{
	attributes.cpy(node);
	left = NULL;
	right = NULL;
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData & parent, TokenData & leftTok, TokenData & rightTok)
{
	attributes.cpy(parent);
	left = createSyntaxTree(leftTok);
	right = createSyntaxTree(rightTok);
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData & parent, SyntaxTree * leftBranch, TokenData & rightTok)
{
	attributes.cpy(parent);
	left = leftBranch;
	right = createSyntaxTree(rightTok);
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData & node, SyntaxTree * tempAux)
{
	attributes.cpy(node);
	left = NULL;
	right = NULL;
	auxLength = tempAux->getAuxLength();
	aux = tempAux->releaseAux();
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

SyntaxTree ** SyntaxTree::releaseAux()
{
	SyntaxTree ** temp = aux;
	aux = NULL;
	return temp;
}

void SyntaxTree::bifurcate(SyntaxTree * leftBranch, TokenData rightTok)
{
	left = leftBranch;
	right = createSyntaxTree(rightTok);
}

void SyntaxTree::bifurcate(TokenData leftTok, TokenData rightTok)
{
	left = createSyntaxTree(leftTok);
	right = createSyntaxTree(rightTok);
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
		if (!(aux = createSyntaxTree * [1]))
	} else	{
		SyntaxTree ** temp;
		if (!(temp = createSyntaxTree * [auxLength + 1]))
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

bool SyntaxTree::isAux() const
{
	return aux ? true : false;
}
