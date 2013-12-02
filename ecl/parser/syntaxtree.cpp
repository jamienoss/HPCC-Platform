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

//----------------------------------SyntaxTree--------------------------------------------------------------------
SyntaxTree::SyntaxTree()
{
	attributes.lineNumber = 0;
    left = NULL;
    right = NULL;
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData node)
{
	attributes = node;
	left = NULL;
	right = NULL;
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData parent, TokenData leftTok, TokenData rightTok)
{
	attributes = parent;
	left = new SyntaxTree(leftTok);
	right = new SyntaxTree(rightTok);
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData parent, SyntaxTree * leftBranch, TokenData rightTok)
{
	attributes = parent;
	left = leftBranch;
	right = new SyntaxTree(rightTok);
    aux = NULL;
    auxLength = 0;
}

SyntaxTree::SyntaxTree(TokenData node, SyntaxTree * tempAux)
{
	attributes = node;
	left = NULL;
	right = NULL;
	auxLength = tempAux->getAuxLength();
	aux = tempAux->releaseAux();
	//delete tempAux;
}

SyntaxTree::~SyntaxTree()
{
     if(left)
         delete left;
     if(right)
         delete right;
     if(aux)
     {
         for ( int i = 0; i < auxLength; ++i)
         {
             delete aux[i];
         }
         delete[] aux;
     }
}

SyntaxTree ** SyntaxTree::releaseAux()
{
	return aux;
	aux = NULL;
}

void SyntaxTree::bifurcate(SyntaxTree * leftBranch, TokenData rightTok)
{
	left = leftBranch;
	right = new SyntaxTree(rightTok);
}

void SyntaxTree::bifurcate(TokenData leftTok, TokenData rightTok)
{
	left = new SyntaxTree(leftTok);
	right = new SyntaxTree(rightTok);
}

void SyntaxTree::bifurcate(SyntaxTree * leftBranch, SyntaxTree * rightBranch)
{
	left = leftBranch;
	right = rightBranch;
}

SyntaxTree * SyntaxTree::setRight(TokenData rightTok)
{
	right = new SyntaxTree(rightTok);
	return this;
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
	int parentNodeNumm = *nodeNum;

	printNode(nodeNum);

	if(left)
	{
		printEdge(parentNodeNumm, *nodeNum);
		ioStatL = left->printBranch(parentNodeNum, nodeNum);
	}
	if(right)
	{
		printEdge(parentNodeNumm, *nodeNum);
		ioStatR = right->printBranch(parentNodeNum, nodeNum);
	}

	if(aux)
	{
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
	if(!aux)
	{
		if(!(aux = new (std::nothrow) SyntaxTree * [1]))
		{
			std::cout << "oops didn't allocate!\n";
		}
	}
	else
	{
		SyntaxTree ** temp;
		if(!(temp = new (std::nothrow) SyntaxTree * [auxLength + 1]))
		{
			std::cout << "oops didn't allocate!\n";
		}
		for(int i = 0; i < auxLength; ++i)
		{
			temp[i] = aux[i];
		}
		delete[] aux; //MORE: maybe recycle these?
		aux = temp;
		temp = NULL;
	}

	aux[auxLength++] = addition;
}

unsigned SyntaxTree::getAuxLength()
{
	return auxLength;
}
