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
#include "bisongram.h"

std::vector <std::string> * AnalyserST::symbolList = NULL;

//----------------------------------AnalyserST--------------------------------------------------------------------
ISyntaxTree * AnalyserST::createSyntaxTree(TokenData & tok)
{
    return new AnalyserST(tok);
}

ISyntaxTree * AnalyserST::createSyntaxTree(TokenKind & _token, const ECLlocation & _pos)
{
    return new AnalyserST(_token, _pos);
}

AnalyserST::AnalyserST(TokenData & tok) : SyntaxTree(tok)
{
    symbolList = NULL;
}

AnalyserST::AnalyserST(TokenKind & _token, const ECLlocation & _pos) : SyntaxTree(_token, _pos)
{
    symbolList = NULL;
}

void AnalyserST::printTree()
{
	unsigned n = symbolList->size();
	unsigned parentNodeNum = n+1, nodeNum = n+1; // shifted beyond those reserved for nonTerminalDefKind
	StringBuffer str;
	Owned<IFile> treeFile = createIFile("grammarAnalysis.dot");
	Owned<IFileIO> io = treeFile->open(IFOcreaterw);
	Owned<IFileIOStream> out = createIOStream(io);

	str = ("graph \"Abstract Syntax Tree\"\n{\nordering=out\n");
	out->write(str.length(), str.str());

	printBranch(& parentNodeNum, & nodeNum, out);

	str = "}\n";
    out->write(str.length(), str.str());
    io->close();
}


void AnalyserST::printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx)
{
    //if(attributes.attributeKind == none)
      //  return true;

    if (!children.isItem(childIndx))
        return;

    int tempParentNodeNum = (int)parentNodeNum;
    int tempNodeNum = (int)nodeNum;

    switch(token)
    {
    case NONTERMINAL :
    case 300 :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(name->str()))
            {
                tempParentNodeNum = i;
                break;
            }
        }
    }
    }

    switch(queryPrivateChild(childIndx)->token)
    {
    case NONTERMINAL :
    case 300 :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(queryPrivateChild(childIndx)->name->str()))
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
}

void AnalyserST::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer str;

    switch(token)
    {
    //case none :
    case NONTERMINAL : return;
    case 300 :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(name->str()))
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

	/*switch(kind){
	case integerKind : str.append(attributes.integer); break;
	case realKind : str.append(attributes.real); break;
	case lexemeKind:
	case terminalKind :
    case nonTerminalKind :
    case nonTerminalDefKind :
    case productionKind : str.append(attributes.lexeme); break;
	default : str.append("KIND not yet defined!"); break;
	}*/


	str.append(name->str()).append("\\nLine: ").append(pos.lineno).append("\"");
	switch(token)
	{
	case 300 : str.append("style=filled, color=\"0.25,0.5,1\"]\n"); break;//green
	case TERMINAL : str.append("style=filled, color=\"0,0.5,1\"]\n"); break;//red
    case CODE : str.append("style=filled, color=\"0.66,0.5,1\", fontcolor=white]\n"); break;//blue
	default : str.append("]\n");
	}

	out->write(str.length(), str.str());
}

void AnalyserST::extractSymbols(std::vector <std::string> & symbolList, TokenKind & kind)
{
    if(children.ordinality())
    {
        ForEachItemIn(i,children)
            children.item(i).extractSymbols(symbolList, kind);
    }

    // add only new symbols
    unsigned m = symbolList.size();
    std::string lexeme;
    if(token == kind)
    {
        switch(token)
        {
           case TERMINAL :
           case NONTERMINAL :
           {
               lexeme = name->str();
               for (unsigned i = 0; i < m; ++i)
               {
                   if(!symbolList[i].compare(lexeme))
                       return;
               }
               symbolList.push_back(lexeme);
               break;
           }
           case 300 :
           {
               lexeme = name->str();
               symbolList.push_back(lexeme);
               break;
           }
        }
    }
}


void AnalyserST::setSymbolList(std::vector <std::string> & list)
{
    symbolList = &list;
}

void AnalyserST::printSymbolList()
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
