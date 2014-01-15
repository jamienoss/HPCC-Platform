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

#include "asyntaxtree.hpp"
#include "jstring.hpp"
#include <iostream>
#include <cstring>

std::vector <std::string> * ASyntaxTree::symbolList = NULL;

//----------------------------------ASyntaxTree--------------------------------------------------------------------
ASyntaxTree * ASyntaxTree::createASyntaxTree()
{
    return new ASyntaxTree();
}

ASyntaxTree * ASyntaxTree::createASyntaxTree(TokenData & token)
{
    return new ASyntaxTree(token);
}


ASyntaxTree * ASyntaxTree::createASyntaxTree(TokenData & parentTok, TokenData & leftTok, TokenData & rightTok)
{
    ASyntaxTree * temp = new ASyntaxTree(parentTok);
    temp->setLeft(leftTok);
    temp->setRight(rightTok);
    return temp;
}

ASyntaxTree * ASyntaxTree::createASyntaxTree(TokenData & parentTok, ASyntaxTree * leftBranch, TokenData & rightTok)
{
    ASyntaxTree * temp = new ASyntaxTree(parentTok);
    temp->setLeft(leftBranch);
    temp->setRight(rightTok);
    return temp;
}

ASyntaxTree * ASyntaxTree::createASyntaxTree(TokenData & parentTok, ASyntaxTree * leftBranch, ASyntaxTree * rightBranch)
{
    ASyntaxTree * temp =  new ASyntaxTree(parentTok);
    temp->setLeft(leftBranch);
    temp->setRight(rightBranch);
    return temp;
}


ASyntaxTree * ASyntaxTree::createASyntaxTree(TokenData & token, ASyntaxTree * tempAux)
{
    ASyntaxTree * temp = new ASyntaxTree(token);
    temp->transferChildren(tempAux);
    return temp;
}


ASyntaxTree::ASyntaxTree()
{
	attributes.lineNumber = 0;
    left = NULL;
    right = NULL;
    children = NULL;
    attributes.attributeKind = none;
}

ASyntaxTree::ASyntaxTree(TokenData & token)
{
	attributes.cpy(token);
	left = NULL;
	right = NULL;
    children = NULL;
}

ASyntaxTree::~ASyntaxTree()
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

ASyntaxTree * ASyntaxTree::release()
{
	ASyntaxTree * temp = this;
//	this = NULL;
	return temp;
}

void ASyntaxTree::setLeft(ASyntaxTree * node)
{
    left = node;
    node = NULL;
}

void ASyntaxTree::setRight(ASyntaxTree * node)
{
    right = node;
    node = NULL;
}

void ASyntaxTree::setLeft(TokenData & token)
{
    left = createASyntaxTree(token);
}

void ASyntaxTree::setRight(TokenData & token)
{
    right = createASyntaxTree(token);
}

void ASyntaxTree::bifurcate(ASyntaxTree * leftBranch, TokenData & rightTok)
{
	left = leftBranch;
	setRight(rightTok);
}

void ASyntaxTree::bifurcate(TokenData & leftTok, TokenData & rightTok)
{
	setLeft(leftTok);
	setRight(rightTok);
}

void ASyntaxTree::bifurcate(ASyntaxTree * leftBranch, ASyntaxTree * rightBranch)
{
	left = leftBranch;
	right = rightBranch;
}

bool ASyntaxTree::printTree()
{
	int ioStat;
	unsigned n = symbolList->size();
	unsigned parentNodeNum = n+1, nodeNum = n+1; // shifted beyond those reserved for nonTerminalDefKind
	StringBuffer str;
	Owned<IFile> treeFile = createIFile("grammarAnalysis.dot");
	Owned<IFileIO> io = treeFile->open(IFOcreaterw);
	Owned<IFileIOStream> out = createIOStream(io);

	str = ("graph \"Abstract Syntax Tree\"\n{\nordering=out\n");
	out->write(str.length(), str.str());

	ioStat = printBranch(& parentNodeNum, & nodeNum, out);

	str = "}\n";
    out->write(str.length(), str.str());
    io->close();
	return ioStat;
}

bool  ASyntaxTree::printBranch(unsigned * parentNodeNum, unsigned * nodeNum, Owned<IFileIOStream> & out)
{
	bool ioStatL;
	bool ioStatR;
	unsigned parentNodeNumm = *nodeNum;

	printNode(nodeNum, out);

	if (left)
	{
		printEdge(parentNodeNumm, *nodeNum, left, out);
		ioStatL = left->printBranch(parentNodeNum, nodeNum, out);
	}

    if(children)
    {
        ForEachItemIn(i,*children)
        {
            printEdge(parentNodeNumm, *nodeNum, &children->item(i), out);
            children->item(i).printBranch(parentNodeNum, nodeNum, out);
        }
    }

	if (right)
	{
		printEdge(parentNodeNumm, *nodeNum, right, out);
		ioStatR = right->printBranch(parentNodeNum, nodeNum, out);
	}

	return !(ioStatL && ioStatR);
}

bool ASyntaxTree::printEdge(unsigned parentNodeNum, unsigned nodeNum, ASyntaxTree * child, Owned<IFileIOStream> & out)
{
    if(attributes.attributeKind == none)
        return true;

    int tempParentNodeNum = (int)parentNodeNum;
    int tempNodeNum = (int)nodeNum;

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

    StringBuffer str;
    if(tempParentNodeNum == tempNodeNum)
        str.append(tempParentNodeNum).append(" -- ").append(tempNodeNum).append(" [style = dashed]\n");
    else
        str.append(tempParentNodeNum).append(" -- ").append(tempNodeNum).append(" [style = solid]\n");
    out->write(str.length(), str.str());

	return true;
}

bool ASyntaxTree::printNode(unsigned * nodeNum, Owned<IFileIOStream> & out)
{
    symbolKind kind = attributes.attributeKind;
    StringBuffer str;

    switch(kind)
    {
    case none :
    case nonTerminalKind : return true;
    case nonTerminalDefKind :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(attributes.lexeme))
            {
                str.append(i).append(" [label = \"");
                break;
            }
        }
        break;
    }
    default :
    {
        str.append(*nodeNum).append(" [label = \"");
        (*nodeNum)++;
    }
    }

	switch(kind){
	case integerKind : str.append(attributes.integer); break;
	case realKind : str.append(attributes.real); break;
	case lexemeKind:
	case terminalKind :
    case nonTerminalKind :
    case nonTerminalDefKind :
    case productionKind : str.append(attributes.lexeme); break;
	default : str.append("KIND not yet defined!"); break;
	}

	str.append("\\nLine: ").append(attributes.lineNumber).append("\"");
	switch(kind)
	{
	case nonTerminalDefKind : str.append("style=filled, color=\"0.25,0.5,1\"]\n"); break;//green
	case terminalKind : str.append("style=filled, color=\"0,0.5,1\"]\n"); break;//red
    case productionKind : str.append("style=filled, color=\"0.66,0.5,1\", fontcolor=white]\n"); break;//blue
	default : str.append("]\n");
	}

	out->write(str.length(), str.str());
	return true;
}

void ASyntaxTree::addChild(ASyntaxTree * addition)
{
    if(!children)
        children = new CIArrayOf<ASyntaxTree>;

    children->append(*addition);
    addition = NULL;
}

void ASyntaxTree::transferChildren(ASyntaxTree * node) // come up with a better name!!!
{
    children = node->children;
    node->children = NULL;
    delete node;
}

void ASyntaxTree::extractSymbols(std::vector <std::string> & symbolList, symbolKind kind)
{
    if(left)
        left->extractSymbols(symbolList, kind);
    if(right)
        right->extractSymbols(symbolList, kind);

    if(children)
    {
        ForEachItemIn(i,*children)
            children->item(i).extractSymbols(symbolList, kind);
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

const char * ASyntaxTree::getLexeme()
{
    return attributes.lexeme;
}


void ASyntaxTree::setSymbolList(std::vector <std::string> & list)
{
    symbolList = &list;
}

void ASyntaxTree::printSymbolList()
{
    unsigned n = symbolList->size();
    StringBuffer str;
    Owned<IFile> treeFile = createIFile("symbolList.txt");
    Owned<IFileIO> io = treeFile->open(IFOcreaterw);
    Owned<IFileIOStream> out = createIOStream(io);

    for (unsigned i = 0; i < n; ++i)
    {
        str.append(((*symbolList)[i]).c_str()).append("\n");
    }
    out->write(str.length(), str.str());
    io->close();
}

symbolKind ASyntaxTree::getKind()
{
    return attributes.attributeKind;
}

//----------------------------------linkedSTlist--------------------------------------------------------------------

linkedSTlist::linkedSTlist()
{
    data = NULL;
    next = NULL;
}

linkedSTlist::linkedSTlist(ASyntaxTree * node)
{
    data = node;
    next = NULL;
}

ASyntaxTree * linkedSTlist::pop()
{
   linkedSTlist * temp = this;
   linkedSTlist * tempB4 = NULL;
   ASyntaxTree * toReturn;

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

void linkedSTlist::push(ASyntaxTree * node)
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

ASyntaxTree * linkedSTlist::operator[](unsigned idx)
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
