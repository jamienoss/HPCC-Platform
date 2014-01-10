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

std::vector <std::string> * SyntaxTree::symbolList = NULL;

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
	attributes.lineNumber = 0;
    left = NULL;
    right = NULL;
    children = NULL;
    attributes.attributeKind = none;
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

     switch(attributes.attributeKind)
     {
     case lexemeKind :
     case terminalKind :
     case nonTerminalKind :
     case productionKind :
        delete[] attributes.lexeme; break;
     }
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
	unsigned n = symbolList->size();
	unsigned parentNodeNum = n+1, nodeNum = n+1; // shifted beyond those reserved for nonTerminalDefKind

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

	if (left)
	{
		printEdge(parentNodeNumm, *nodeNum, left);
		ioStatL = left->printBranch(parentNodeNum, nodeNum);
	}

    if(children)
    {
        linkedSTlist * temp = children;
        while(temp->next)
        {
            printEdge(parentNodeNumm, *nodeNum, temp->data);
            temp->data->printBranch(parentNodeNum, nodeNum);
            temp = temp->next;
        }
        // print last node
         printEdge(parentNodeNumm, *nodeNum, temp->data);
         temp->data->printBranch(parentNodeNum, nodeNum);
    }

	if (right)
	{
		printEdge(parentNodeNumm, *nodeNum, right);
		ioStatR = right->printBranch(parentNodeNum, nodeNum);
	}

	return !(ioStatL && ioStatR);
}

bool SyntaxTree::printEdge(unsigned parentNodeNum, unsigned nodeNum, SyntaxTree * child)
{
    int tempParentNodeNum = (int)parentNodeNum;
    int tempNodeNum = (int)nodeNum;

    if(attributes.attributeKind == none)
        return true;

    switch(attributes.attributeKind)
    {
    case nonTerminalKind :
    case nonTerminalDefKind :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(attributes.lexeme))
            {
                tempParentNodeNum = i;
                break;
            }
        }
    }
    }

    switch(child->attributes.attributeKind)
    {
    case nonTerminalKind :
    case nonTerminalDefKind :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(child->attributes.lexeme))
            {
                tempNodeNum = i;
                break;
            }
        }
    }
    }
	std::cout << tempParentNodeNum << " -- " << tempNodeNum << " [style = solid]\n";
	return true;
}

bool SyntaxTree::printNode(unsigned * nodeNum)
{
    symbolKind kind = attributes.attributeKind;

    if(kind == none)
        return true;

    switch(kind)
    {
    case nonTerminalKind : return true;
    case nonTerminalDefKind :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(attributes.lexeme))
            {
                std::cout << i << " [label = \"";
                break;
            }
        }
        break;
    }
    default :
    {
        std::cout << *nodeNum << " [label = \"";
        (*nodeNum)++;
    }
    }

	switch(kind){
	case integerKind : std::cout << attributes.integer; break;
	case realKind : std::cout << attributes.real; break;
	case lexemeKind:
	case terminalKind :
    case nonTerminalKind :
    case nonTerminalDefKind :
    case productionKind : std::cout << attributes.lexeme; break;
	default : std::cout << "KIND not yet defined!"; break;
	}

	std::cout << "\\nLine: " << attributes.lineNumber << "\"]\n";
	return true;
}

void SyntaxTree::addChild(SyntaxTree * addition) //MORE: Should maybe use vectors here, talk to Gavin.
{
    if(!children)
            children = new linkedSTlist(addition);
        else
            children->push(addition);
        addition = NULL;
}

void SyntaxTree::transferChildren(SyntaxTree * node) // come up with a better name!!!
{
    children = node->children;
    node->children = NULL;
    delete node;
}

void SyntaxTree::extractSymbols(std::vector <std::string> & symbolList, symbolKind kind)
{
    if(left)
        left->extractSymbols(symbolList, kind);
    if(right)
        right->extractSymbols(symbolList, kind);

    if(children)
    {
        linkedSTlist * temp = children;
        while(temp->next)
        {
            temp->data->extractSymbols(symbolList, kind);
            temp = temp->next;
        }
        //last node
        temp->data->extractSymbols(symbolList, kind);
    }

    // add only new symbols
    unsigned m = symbolList.size();
    std::string lexeme;
    symbolKind nodeKind = attributes.attributeKind;
    if(nodeKind == kind)
    {
        switch(nodeKind)
        {
           case terminalKind :
           case nonTerminalKind :
           {
               lexeme = getLexeme();
               for (unsigned i = 0; i < m; ++i)
               {
                   if(!symbolList[i].compare(lexeme))
                       return;
               }
               symbolList.push_back(lexeme);
               break;
           }
           case nonTerminalDefKind :
           {
               lexeme = getLexeme();
               symbolList.push_back(lexeme);
               break;
           }
        }
    }
}

const char * SyntaxTree::getLexeme()
{
    return attributes.lexeme;
}


void SyntaxTree::setSymbolList(std::vector <std::string> & list)
{
    symbolList = &list;
}

void SyntaxTree::printSymbolList()
{
    unsigned n = symbolList->size();
    for (unsigned i = 0; i < n; ++i)
    {
        std::cout << (*symbolList)[i] << "\n";
    }
}

symbolKind SyntaxTree::getKind()
{
    return attributes.attributeKind;
}

//----------------------------------linkedSTlist--------------------------------------------------------------------

linkedSTlist::linkedSTlist()
{
    data = NULL;
    next = NULL;
}

linkedSTlist::linkedSTlist(SyntaxTree * node)
{
    data = node;
    next = NULL;
}

SyntaxTree * linkedSTlist::pop()
{
   linkedSTlist * temp = this;
   linkedSTlist * tempB4 = NULL;
   SyntaxTree * toReturn;

   while(temp->next)
   {
       tempB4 = temp;
       temp = temp->next;
   }
   toReturn = temp->data;
   if(tempB4)
       tempB4->next = NULL;
   delete temp;
   return toReturn;
}

linkedSTlist::~linkedSTlist()
{
    if(next)
        delete next;
}

void linkedSTlist::push(SyntaxTree * node)
{
    if(next)
       next->push(node);
    else
        next = new linkedSTlist(node);
}

unsigned linkedSTlist::size()
{
    if(!this)
        return 0;

    unsigned count = 0;
    linkedSTlist * temp = this;
    while(temp->next)
    {
        count++;
        temp = temp->next;
    }
    return count=1;
}

SyntaxTree * linkedSTlist::operator[](unsigned idx)
{
    if(idx < 0 || idx > size())
        std::cout << "error: subscript out of bounds\n";
        return NULL;

    linkedSTlist* temp = this;
    for ( unsigned i = 0; i < idx; ++i)
    {
        temp = temp->next;
    }
    return temp->data;
}
