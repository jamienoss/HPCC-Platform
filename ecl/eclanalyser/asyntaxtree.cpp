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
#include "bisongram.h"

ISyntaxTree * createAnalyserPuncST(const ECLlocation & _pos, const TokenKind & _kind)
{
    return AnalyserPuncST::createSyntaxTree(_pos, _kind);
}

ISyntaxTree * createAnalyserIdST(const ECLlocation & _pos, IIdAtom * _id, const TokenKind & _kind)
{
    return AnalyserIdST::createSyntaxTree(_pos, _id, _kind);
}

ISyntaxTree * createAnalyserStringST(const ECLlocation & _pos, const StringBuffer & _text, const TokenKind & _kind)
{
    return AnalyserStringST::createSyntaxTree(_pos, _text, _kind);
}
//----------------------------------AnalyserST--------------------------------------------------------------------
IdTable * AnalyserSymbols::idNameList = NULL;
AnalyserSymbols::AnalyserSymbols() { /*symbolList = NULL;*/ }

/*void AnalyserIdST::printTree()
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

*/
/*void AnalyserIdST::printEdge(unsigned parentNodeNum, unsigned nodeNum, IIOStream * out, unsigned childIndx)
{
    //if(attributes.attributeKind == none)
      //  return true;

    if (!children.isItem(childIndx))
        return;

    int tempParentNodeNum = (int)parentNodeNum;
    int tempNodeNum = (int)nodeNum;

    switch(kind)
    {
    case NONTERMINAL :
    case 300 :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(id->str()))
            {
                tempParentNodeNum = i;
                break;
            }
        }
    }
    }

    switch(queryPrivateChild(childIndx)->kind)
    {
    case NONTERMINAL :
    case 300 :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(queryPrivateChild(childIndx)->id->str()))
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

void AnalyserIdST::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer str;

    switch(kind)
    {
    //case none :
    case NONTERMINAL : return;
    case 300 :
    {
        unsigned n = symbolList->size();
        for (unsigned i = 0; i < n; ++i)
        {
            if(!(*symbolList)[i].compare(id->str()))
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

	str.append(id->str()).append("\\nLine: ").append(pos.lineno).append("\"");
	switch(kind)
	{
	case 300 : str.append("style=filled, color=\"0.25,0.5,1\"]\n"); break;//green
	case TERMINAL : str.append("style=filled, color=\"0,0.5,1\"]\n"); break;//red
    case CODE : str.append("style=filled, color=\"0.66,0.5,1\", fontcolor=white]\n"); break;//blue
	default : str.append("]\n");
	}

	out->write(str.length(), str.str());
}
*/

void AnalyserSymbols::setIdNameList(IdTable * list)
{
    idNameList = list;
}

void AnalyserSymbols::printIdNameList()
{
    StringBuffer str;
    Owned<IFile> treeFile = createIFile("symbolList.txt");
    Owned<IFileIO> io = treeFile->open(IFOcreaterw);
    Owned<IFileIOStream> out = createIOStream(io);

    if(idNameList->ordinality())
    {
        ForEachItemIn(i, *idNameList)
        {
            str.append(idNameList->item(i).getIdName()).newline();
        }
    }
    out->write(str.length(), str.str());
    io->close();
}

extern void analyserAppendParserTokenText(StringBuffer & target, unsigned tok);

//-----------------------------------------------------------------------------------------------------------------------

ISyntaxTree * AnalyserPuncST::createSyntaxTree(const ECLlocation & _pos, const TokenKind & _kind)
{
    return new AnalyserPuncST(_pos, _kind);
}

AnalyserPuncST::AnalyserPuncST(const ECLlocation & _pos, const TokenKind & _kind) : AnalyserSymbols(), PuncSyntaxTree(_pos, _kind) {}

void AnalyserPuncST::printNode(unsigned * nodeNum, IIOStream * out)
{
    StringBuffer text;
    analyserAppendParserTokenText(text, value);
    //text.append(value);
    if(value < 256)
        SyntaxTree::printNode(nodeNum, out, text, "\"0.25,0.5,1\"");
    else
        SyntaxTree::printNode(nodeNum, out, text, "\"1.0,0.5,1\"");
}

void AnalyserPuncST::appendSTvalue(StringBuffer & str)
{
    analyserAppendParserTokenText(str, value);
}
//-----------------------------------------------------------------------------------------------------------------------
ISyntaxTree * AnalyserIdST::createSyntaxTree(const ECLlocation & _pos, IIdAtom * _id, const TokenKind & _kind)
{
    return new AnalyserIdST(_pos, _id, _kind);
}

AnalyserIdST::AnalyserIdST(const ECLlocation & _pos, IIdAtom * _id, const TokenKind & _kind) : AnalyserSymbols(), IdSyntaxTree(_pos, _id)
{
    kind = _kind;
}

void AnalyserIdST::createIdNameList(IdTable & symbolList, TokenKind & _kind)
{
    if(children.ordinality())
    {
       ForEachItemIn(i, children)
           children.item(i).createIdNameList(symbolList, _kind);
    }
    // add only new symbols
    TokenKind kind = getKind();
    if(kind == _kind)
    {
       String * idName = new String(id->str());
       IdTableItem * idItem = new IdTableItem(*id);
       switch(kind)
       {
       case TERMINAL :
       case NONTERMINAL :
       {
           ForEachItemIn(i, symbolList)
           {
               if(!idName->compareTo(symbolList.item(i).getIdName()))
                   return;
           }
           symbolList.append(*LINK(idItem)); break;
       }
       case NONTERMINALDEF :
           symbolList.append(*LINK(idItem)); break;
       }
   }
}

//-----------------------------------------------------------------------------------------------------------------------
ISyntaxTree * AnalyserStringST::createSyntaxTree(const ECLlocation & _pos, const StringBuffer & _text, const TokenKind & _kind)
{
    return new AnalyserStringST(_pos, _text, _kind);
}

AnalyserStringST::AnalyserStringST(const ECLlocation & _pos, const StringBuffer & _text, const TokenKind & _kind) : AnalyserSymbols(), SyntaxTree(_pos)
{
    kind = _kind;
    text = _text.str();
}

void AnalyserStringST::printNode(unsigned * nodeNum, IIOStream * out)
{
    SyntaxTree::printNode(nodeNum, out, text.str(), "\"1.0,0.5,1\"");
}

//-----------------------------------------------------------------------------------------------------------------------
