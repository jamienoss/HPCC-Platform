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
#include <iostream>

//----------------------------------SyntaxTree--------------------------------------------------------------------
SyntaxTree::SyntaxTree()
{
	attributes.lineNumber = 0;
    left = NULL;
    right = NULL;
    aux = NULL;
}

SyntaxTree::SyntaxTree(TokenData node)
{
	attributes = node;
	left = NULL;
	right = NULL;
    aux = NULL;
}

SyntaxTree::SyntaxTree(TokenData parent, TokenData leftTok, TokenData rightTok)
{
	attributes = parent;
	left = new SyntaxTree(leftTok);
	right = new SyntaxTree(rightTok);
    aux = NULL;
}

SyntaxTree::~SyntaxTree()
{
     if(left)
         delete left;
     if(right)
         delete right;
     if(aux)
         delete aux;

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
	int parentNodeNum = 0, nodeNum = 0;

	std::cout << "graph \"Abstract Syntax Tree\"\n{\n";
	ioStat = printBranch(& parentNodeNum, & nodeNum);
	std::cout << "}\n";
	return ioStat;
}

bool  SyntaxTree::printBranch(int * parentNodeNum, int * nodeNum)
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

	return !(ioStatL && ioStatR);
}

bool SyntaxTree::printEdge(int parentNodeNum, int nodeNum)
{
	std::cout << parentNodeNum << " -- " << nodeNum << " [style = solid]\n";
	return true;
}

bool SyntaxTree::printNode(int * nodeNum)
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
